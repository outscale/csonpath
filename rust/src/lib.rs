pub mod error;
pub mod ffi;
pub mod backend_serde;

use std::ffi::{c_int, c_void, CStr, CString};
use ffi::*;
use serde_json::Value;
use error::CsonpathError;

pub use ffi::CsonpathChildInfo;

pub const CSONPATH_CHILD_NONE: c_int = 0;
pub const CSONPATH_CHILD_INTEGER: c_int = 1;
pub const CSONPATH_CHILD_STR: c_int = 2;

impl CsonpathChildInfo {
    pub fn is_none(&self) -> bool {
        self.type_ == CSONPATH_CHILD_NONE
    }

    pub fn is_integer(&self) -> bool {
        self.type_ == CSONPATH_CHILD_INTEGER
    }

    pub fn is_string(&self) -> bool {
        self.type_ == CSONPATH_CHILD_STR
    }

    pub fn idx(&self) -> Option<usize> {
        if self.is_integer() {
            Some(unsafe { self.data.idx } as usize)
        } else {
            None
        }
    }

    pub fn key(&self) -> Option<&str> {
        if self.is_string() {
            unsafe {
                let c = CStr::from_ptr(self.data.key);
                Some(std::str::from_utf8_unchecked(c.to_bytes()))
            }
        } else {
            None
        }
    }
}

pub struct CallbackContext<'a> {
    pub parent: *const Value,
    pub child_info: &'a CsonpathChildInfo,
    pub current: &'a mut Value,
}

impl<'a> CallbackContext<'a> {
    pub fn parent_ref(&self) -> Option<&Value> {
        if self.parent == self.current as *const Value {
            None
        } else {
            unsafe { Some(&*self.parent) }
        }
    }
}

struct TrampolineData<'a> {
    f: &'a mut dyn FnMut(CallbackContext) -> Result<(), CsonpathError>,
    error: &'a mut Option<CsonpathError>,
}

extern "C" fn callback_trampoline(
    parent: *mut Value,
    ci: *mut c_void,
    current: *mut Value,
    udata: *mut c_void,
) -> c_int {
    unsafe {
        let data = &mut *(udata as *mut TrampolineData<'_>);
        let context = CallbackContext {
            parent: parent as *const Value,
            child_info: &*(ci as *const CsonpathChildInfo),
            current: &mut *current,
        };
        match std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| (data.f)(context))) {
            Ok(Ok(())) => 0,
            Ok(Err(e)) => {
                *data.error = Some(e);
                -1
            }
            Err(_) => -1,
        }
    }
}

pub struct CsonPath {
    raw: *mut Csonpath,
}

impl CsonPath {
    pub fn from_raw(raw: *mut Csonpath) -> Self {
        CsonPath { raw }
    }

    pub fn raw_ptr(&self) -> *mut Csonpath {
        self.raw
    }

    pub fn new(path: &str) -> Result<Self, CsonpathError> {
        Self::new_with_flags(path, 0)
    }

    fn new_with_flags(path: &str, flags: i32) -> Result<Self, CsonpathError> {
        let c_path = CString::new(path).map_err(|_| CsonpathError::CompileError("nul in path".into()))?;
        let raw = unsafe { csonpath_new_ex(c_path.as_ptr(), flags) };
        if raw.is_null() {
            return Err(CsonpathError::CompileError("null csonpath".into()));
        }
        unsafe {
            let ce = (*raw).compile_error;
            if !ce.is_null() {
                let msg = std::ffi::CStr::from_ptr(ce)
                    .to_string_lossy()
                    .into_owned();
                return Err(CsonpathError::CompileError(msg));
            }
        }
        Ok(CsonPath { raw })
    }

    pub fn find_first(&self, value: &Value) -> Result<Option<Value>, CsonpathError> {
        let raw_result = unsafe {
            csonpath_find_first(self.raw, value as *const Value as *mut c_void)
        };
        if raw_result.is_null() {
            Ok(None)
        } else {
            // raw_result points INSIDE the original tree — clone, do not free
            Ok(Some(unsafe { (*(raw_result as *mut Value)).clone() }))
        }
    }

    pub fn set_return_empty_array(&mut self, value: bool) {
        unsafe {
            (*self.raw).return_empty_array = value as c_int;
        }
    }

    pub fn find_all(&self, value: &Value) -> Result<Value, CsonpathError> {
        let raw_result = unsafe {
            csonpath_find_all(self.raw, value as *const Value as *mut c_void)
        };
        if raw_result.is_null() {
            return Ok(Value::Null);
        }
        unsafe { Ok(*Box::from_raw(raw_result as *mut Value)) }
    }

    pub fn remove(&self, value: &mut Value) -> Result<usize, CsonpathError> {
        let count = unsafe {
            csonpath_remove(self.raw, value as *mut Value as *mut c_void)
        };
        if count >= 0 {
            Ok(count as usize)
        } else {
            Err(CsonpathError::CompileError("remove failed".into()))
        }
    }

    pub fn update_or_create(&self, value: &mut Value, new_val: Value) -> Result<(), CsonpathError> {
        let new_ptr = Box::into_raw(Box::new(new_val));
        let rc = unsafe {
            csonpath_update_or_create(
                self.raw,
                value as *mut Value as *mut c_void,
                new_ptr as *mut c_void
            )
        };
        // The C core always passes do_incref=1, so it cloned new_val.
        // We still own new_ptr and must free it.
        unsafe { let _ = Box::from_raw(new_ptr); }
        if rc >= 0 {
            Ok(())
        } else {
            Err(CsonpathError::CompileError("update_or_create failed".into()))
        }
    }

    fn call_callback<F>(
        &self,
        value: &mut Value,
        mut f: F,
        c_fn: unsafe extern "C" fn(
            cp: *mut Csonpath,
            origin: *mut c_void,
            callback: *mut c_void,
            udata: *mut c_void,
        ) -> c_int,
    ) -> Result<usize, CsonpathError>
    where
        F: FnMut(CallbackContext) -> Result<(), CsonpathError>,
    {
        let mut error: Option<CsonpathError> = None;
        let mut trampoline_data = TrampolineData {
            f: &mut f,
            error: &mut error,
        };

        let count = unsafe {
            c_fn(
                self.raw,
                value as *mut Value as *mut c_void,
                callback_trampoline as *mut c_void,
                &mut trampoline_data as *mut _ as *mut c_void,
            )
        };

        if let Some(e) = error {
            return Err(e);
        }
        if count >= 0 {
            Ok(count as usize)
        } else {
            Err(CsonpathError::CompileError("callback aborted".into()))
        }
    }

    pub fn callback<F>(&self, value: &mut Value, f: F) -> Result<usize, CsonpathError>
    where
        F: FnMut(CallbackContext) -> Result<(), CsonpathError>,
    {
        self.call_callback(value, f, csonpath_callback)
    }

    pub fn update_or_create_callback<F>(
        &self,
        value: &mut Value,
        f: F,
    ) -> Result<usize, CsonpathError>
    where
        F: FnMut(CallbackContext) -> Result<(), CsonpathError>,
    {
        self.call_callback(value, f, csonpath_update_or_create_callback)
    }
}

impl Drop for CsonPath {
    fn drop(&mut self) {
        unsafe { csonpath_destroy(self.raw); }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde_json::json;

    #[test]
    fn test_find_first_simple() {
        let data = json!({"a": "hello"});
        let cp = CsonPath::new("$.a").unwrap();
        let r = cp.find_first(&data).unwrap();
        assert_eq!(r, Some(json!("hello")));
    }

    #[test]
    fn test_find_all_array() {
        let data = json!({"items": [{"x": 1}, {"x": 2}]});
        let cp = CsonPath::new("$.items[*].x").unwrap();
        let r = cp.find_all(&data).unwrap();
        assert_eq!(r, json!([1, 2]));
    }

    #[test]
    fn test_recursive_descent() {
        let data = json!({"a": {"title": "A"}, "b": {"title": "B"}});
        let cp = CsonPath::new("$..title").unwrap();
        let r = cp.find_all(&data).unwrap();
        assert_eq!(r.as_array().unwrap().len(), 2);
    }

    #[test]
    fn test_filter_gt() {
        let data = json!({
            "store": {
                "book": [
                    {"title": "Cheap", "price": 5},
                    {"title": "Expensive", "price": 25}
                ]
            }
        });
        let cp = CsonPath::new("$.store.book[?(@.price > 20)].title").unwrap();
        let r = cp.find_all(&data).unwrap();
        assert_eq!(r, json!(["Expensive"]));
    }

    #[test]
    fn test_filter_ne() {
        let data = json!({"items": [{"x": 1}, {"x": 2}, {"x": 1}]});
        let cp = CsonPath::new("$.items[?(@.x != 1)]").unwrap();
        let r = cp.find_all(&data).unwrap();
        assert_eq!(r.as_array().unwrap().len(), 1);
    }

    #[test]
    fn test_filter_eq_str() {
        let data = json!({"items": [{"name": "foo"}, {"name": "bar"}]});
        let cp = CsonPath::new("$.items[?(@.name == 'foo')]").unwrap();
        let r = cp.find_all(&data).unwrap();
        assert_eq!(r, json!([{"name": "foo"}]));
    }

    #[test]
    fn test_remove_obj() {
        let mut data = json!({"a": 1, "b": 2});
        let cp = CsonPath::new("$.a").unwrap();
        let count = cp.remove(&mut data).unwrap();
        assert_eq!(count, 1);
        assert!(data.get("a").is_none());
    }

    #[test]
    fn test_remove_array() {
        let mut data = json!({"items": ["a", "b", "c"]});
        let cp = CsonPath::new("$.items[1]").unwrap();
        let count = cp.remove(&mut data).unwrap();
        assert_eq!(count, 1);
        assert_eq!(data["items"], json!(["a", "c"]));
    }

    #[test]
    fn test_update_or_create() {
        let mut data = json!({"a": {"b": 1}});
        let cp = CsonPath::new("$.a.b").unwrap();
        cp.update_or_create(&mut data, json!(42)).unwrap();
        assert_eq!(data["a"]["b"], json!(42));
    }

    #[test]
    fn test_empty_result() {
        let data = json!({"a": []});
        let cp = CsonPath::new("$.a[*]").unwrap();
        let r = cp.find_all(&data).unwrap();
        assert_eq!(r, Value::Null);

        let mut cp = CsonPath::new("$.a[*]").unwrap();
        cp.set_return_empty_array(true);
        let r = cp.find_all(&data).unwrap();
        assert_eq!(r, json!([]));
    }

    #[test]
    fn test_input_not_modified() {
        let data = json!({"items": [1, 2, 3]});
        let cp = CsonPath::new("$.items[*]").unwrap();
        let _ = cp.find_all(&data).unwrap();
        assert_eq!(data["items"], json!([1, 2, 3]));
    }

    #[test]
    fn test_invalid_path() {
        assert!(CsonPath::new("").is_err());
    }

    #[test]
    fn test_f64_cast_to_i64() {
        let data = json!({"price": 19.95});
        let cp = CsonPath::new("$[?(@.price > 20)]").unwrap();
        let r = cp.find_all(&data).unwrap();
        assert_eq!(r, Value::Null);
    }

    #[test]
    fn test_union() {
        let data = json!({"a": 1, "b": 2, "c": 3});
        let cp = CsonPath::new("$.['a','b']").unwrap();
        let r = cp.find_all(&data).unwrap();
        assert_eq!(r, json!([1, 2]));
    }

    #[test]
    fn test_subpath() {
        let data = json!({"a": "b", "b": 42});
        let cp = CsonPath::new("$[$.a]").unwrap();
        let r = cp.find_all(&data).unwrap();
        assert_eq!(r, json!([42]));
    }

    #[test]
    fn test_callback_sum() {
        let data = json!({"a": 1, "b": 2, "c": 3});
        let cp = CsonPath::new("$.*").unwrap();
        let mut sum = 0;
        let count = cp.callback(&mut data.clone(), |ctx| {
            if let Some(n) = ctx.current.as_i64() {
                sum += n;
            }
            Ok(())
        }).unwrap();
        assert_eq!(count, 3);
        assert_eq!(sum, 6);
    }

    #[test]
    fn test_callback_modify() {
        let mut data = json!({"a": 1, "b": 2});
        let cp = CsonPath::new("$.*").unwrap();
        let count = cp.callback(&mut data, |ctx| {
            if let Some(n) = ctx.current.as_i64() {
                *ctx.current = json!(n * 10);
            }
            Ok(())
        }).unwrap();
        assert_eq!(count, 2);
        assert_eq!(data["a"], json!(10));
        assert_eq!(data["b"], json!(20));
    }

    #[test]
    fn test_callback_abort() {
        let mut data = json!({"a": 1, "b": 2});
        let cp = CsonPath::new("$.*").unwrap();
        let result = cp.callback(&mut data, |_ctx| {
            Err(CsonpathError::NullResult)
        });
        assert!(result.is_err());
    }

    #[test]
    fn test_update_or_create_callback() {
        let mut data = json!({"a": {"b": 1}});
        let cp = CsonPath::new("$.a.b").unwrap();
        let count = cp.update_or_create_callback(&mut data, |ctx| {
            if let Some(n) = ctx.current.as_i64() {
                *ctx.current = json!(n + 100);
            }
            Ok(())
        }).unwrap();
        assert_eq!(count, 1);
        assert_eq!(data["a"]["b"], json!(101));
    }
}
