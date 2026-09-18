pub mod error;
pub mod error_capture;
pub mod ffi;
pub mod backend_serde;

pub use error_capture::{clear_last_error, last_error};

use std::ffi::{c_int, c_void, CStr, CString};
use ffi::*;
use serde_json::Value;
use error::CsonpathError;

pub use ffi::CsonpathChildInfo;

pub const CSONPATH_CHILD_NONE: c_int = 0;
pub const CSONPATH_CHILD_INTEGER: c_int = 1;
pub const CSONPATH_CHILD_STR: c_int = 2;

pub const CSONPATH_AUTO_ROOT: c_int = 1;
pub const CSONPATH_NO_DESTROY: c_int = 1 << 1;
pub const CSONPATH_RETURN_EMPTY_ARRAY: c_int = 1 << 2;

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
    extra_roots: Option<Box<Value>>,
}

impl CsonPath {
    pub fn from_raw(raw: *mut Csonpath) -> Self {
        CsonPath { raw, extra_roots: None }
    }

    pub fn raw_ptr(&self) -> *mut Csonpath {
        self.raw
    }

    pub fn new(path: &str) -> Result<Self, CsonpathError> {
        Self::new_with_flags(path, 0)
    }

    pub fn new_with_flags(path: &str, flags: i32) -> Result<Self, CsonpathError> {
        let cp = Self::new_lenient_with_flags(path, flags);
        if let Some(err) = cp.compile_error() {
            return Err(CsonpathError::CompileError(err));
        }
        Ok(cp)
    }

    pub fn new_lenient(path: &str) -> Self {
        Self::new_lenient_with_flags(path, 0)
    }

    pub fn new_lenient_with_flags(path: &str, flags: i32) -> Self {
        let c_path = CString::new(path).unwrap_or_else(|_| CString::new("").unwrap());
        let raw = unsafe { csonpath_new_ex(c_path.as_ptr(), flags | CSONPATH_NO_DESTROY) };
        CsonPath { raw, extra_roots: None }
    }

    pub fn compile_error(&self) -> Option<String> {
        unsafe {
            let ce = (*self.raw).compile_error;
            if ce.is_null() {
                None
            } else {
                Some(std::ffi::CStr::from_ptr(ce).to_string_lossy().into_owned())
            }
        }
    }

    pub fn is_valid(&self) -> bool {
        self.compile_error().is_none()
    }

    pub fn set_extra_roots(&mut self, roots: Vec<Value>) {
        unsafe { (*self.raw).extra_roots = std::ptr::null_mut(); }
        self.extra_roots = None;
        let boxed = Box::new(Value::Array(roots));
        let raw_ptr = Box::into_raw(boxed);
        unsafe { (*self.raw).extra_roots = raw_ptr as *mut c_void; }
        self.extra_roots = Some(unsafe { Box::from_raw(raw_ptr) });
    }

    pub fn clear_extra_roots(&mut self) {
        unsafe { (*self.raw).extra_roots = std::ptr::null_mut(); }
        self.extra_roots = None;
    }

    pub fn builder(path: &str) -> CsonPathBuilder {
        CsonPathBuilder::new(path)
    }

    fn runtime_error_if_set(&self) -> Result<(), CsonpathError> {
        if let Some(err) = last_error() {
            Err(CsonpathError::RuntimeError(err))
        } else {
            Ok(())
        }
    }

    pub fn find_first_lenient(&self, value: &Value) -> Result<Value, CsonpathError> {
        clear_last_error();
        let raw_result = unsafe {
            csonpath_find_first(self.raw, value as *const Value as *mut c_void)
        };
        if raw_result.is_null() {
            self.runtime_error_if_set()?;
            Ok(Value::Null)
        } else {
            // raw_result points INSIDE the original tree — clone, do not free
            Ok(unsafe { (*(raw_result as *mut Value)).clone() })
        }
    }

    pub fn find_first(&self, value: &Value) -> Result<Option<Value>, CsonpathError> {
        match self.find_first_lenient(value)? {
            Value::Null => Ok(None),
            v => Ok(Some(v)),
        }
    }

    pub fn set_return_empty_array(&mut self, value: bool) {
        unsafe {
            if value {
                (*self.raw).flags |= CSONPATH_RETURN_EMPTY_ARRAY;
            } else {
                (*self.raw).flags &= !CSONPATH_RETURN_EMPTY_ARRAY;
            }
        }
    }

    pub fn find_all_lenient(&self, value: &Value) -> Result<Value, CsonpathError> {
        clear_last_error();
        let raw_result = unsafe {
            csonpath_find_all(self.raw, value as *const Value as *mut c_void)
        };
        if raw_result.is_null() {
            self.runtime_error_if_set()?;
            return Ok(Value::Null);
        }
        unsafe { Ok(*Box::from_raw(raw_result as *mut Value)) }
    }

    pub fn find_all(&self, value: &Value) -> Result<Option<Value>, CsonpathError> {
        match self.find_all_lenient(value)? {
            Value::Null => Ok(None),
            v => Ok(Some(v)),
        }
    }

    pub fn remove(&self, value: &mut Value) -> Result<usize, CsonpathError> {
        clear_last_error();
        let count = unsafe {
            csonpath_remove(self.raw, value as *mut Value as *mut c_void)
        };
        if count >= 0 {
            Ok(count as usize)
        } else {
            self.runtime_error_if_set()?;
            Err(CsonpathError::CompileError("remove failed".into()))
        }
    }

    pub fn update_or_create(&self, value: &mut Value, new_val: Value) -> Result<usize, CsonpathError> {
        let new_ptr = Box::into_raw(Box::new(new_val));
        clear_last_error();
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
            Ok(rc as usize)
        } else {
            self.runtime_error_if_set()?;
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

        clear_last_error();
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
            self.runtime_error_if_set()?;
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
        // extra_roots is owned by Rust (self.extra_roots); clear the C pointer
        // before destroying so the core never touches freed memory.
        unsafe { (*self.raw).extra_roots = std::ptr::null_mut(); }
        unsafe { csonpath_destroy(self.raw); }
    }
}

pub struct CsonPathBuilder {
    path: String,
    flags: i32,
    extra_roots: Option<Vec<Value>>,
}

impl CsonPathBuilder {
    pub fn new(path: &str) -> Self {
        CsonPathBuilder {
            path: path.to_string(),
            flags: 0,
            extra_roots: None,
        }
    }

    pub fn auto_root(mut self, value: bool) -> Self {
        if value {
            self.flags |= CSONPATH_AUTO_ROOT;
        } else {
            self.flags &= !CSONPATH_AUTO_ROOT;
        }
        self
    }

    pub fn return_empty_array(mut self, value: bool) -> Self {
        if value {
            self.flags |= CSONPATH_RETURN_EMPTY_ARRAY;
        } else {
            self.flags &= !CSONPATH_RETURN_EMPTY_ARRAY;
        }
        self
    }

    pub fn extra_roots(mut self, roots: Vec<Value>) -> Self {
        self.extra_roots = Some(roots);
        self
    }

    pub fn build(mut self) -> Result<CsonPath, CsonpathError> {
        let mut cp = CsonPath::new_lenient_with_flags(&self.path, self.flags);
        if let Some(err) = cp.compile_error() {
            return Err(CsonpathError::CompileError(err));
        }
        if let Some(roots) = self.extra_roots.take() {
            cp.set_extra_roots(roots);
        }
        Ok(cp)
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
        assert_eq!(r, Some(json!([1, 2])));
    }

    #[test]
    fn test_recursive_descent() {
        let data = json!({"a": {"title": "A"}, "b": {"title": "B"}});
        let cp = CsonPath::new("$..title").unwrap();
        let r = cp.find_all(&data).unwrap();
        assert_eq!(r.unwrap().as_array().unwrap().len(), 2);
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
        assert_eq!(r, Some(json!(["Expensive"])));
    }

    #[test]
    fn test_filter_ne() {
        let data = json!({"items": [{"x": 1}, {"x": 2}, {"x": 1}]});
        let cp = CsonPath::new("$.items[?(@.x != 1)]").unwrap();
        let r = cp.find_all(&data).unwrap();
        assert_eq!(r.unwrap().as_array().unwrap().len(), 1);
    }

    #[test]
    fn test_filter_eq_str() {
        let data = json!({"items": [{"name": "foo"}, {"name": "bar"}]});
        let cp = CsonPath::new("$.items[?(@.name == 'foo')]").unwrap();
        let r = cp.find_all(&data).unwrap();
        assert_eq!(r, Some(json!([{"name": "foo"}])));
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
        let count = cp.update_or_create(&mut data, json!(42)).unwrap();
        assert_eq!(count, 0);
        assert_eq!(data["a"]["b"], json!(42));
    }

    #[test]
    fn test_update_or_create_create_nested_obj() {
        /* Creates the whole chain $.a.b.c: the C core clones each fresh
         * container into its parent (do_incref=1) then frees the temporary
         * with CSONPATH_REMOVE and keeps using it as context.  Must not keep
         * a dangling pointer into the freed temporary. */
        let mut data = json!({});
        let cp = CsonPath::new("$.a.b.c").unwrap();
        let count = cp.update_or_create(&mut data, json!(42)).unwrap();
        assert_eq!(count, 0);
        assert_eq!(data, json!({"a": {"b": {"c": 42}}}));
    }

    #[test]
    fn test_update_or_create_create_single_obj() {
        let mut data = json!({});
        let cp = CsonPath::new("$.a.b").unwrap();
        let count = cp.update_or_create(&mut data, json!(42)).unwrap();
        assert_eq!(count, 0);
        assert_eq!(data, json!({"a": {"b": 42}}));
    }

    #[test]
    fn test_update_or_create_create_after_array_pad() {
        /* Integer lookups: the fresh object is appended at the padded array
         * slot (index 5) while the freed temporary is still used as context. */
        let mut data = json!({"a": [1, 2, 3]});
        let cp = CsonPath::new("$.a[5].b").unwrap();
        let count = cp.update_or_create(&mut data, json!(42)).unwrap();
        assert_eq!(count, 0);
        assert_eq!(data["a"], json!([1, 2, 3, null, null, {"b": 42}]));
    }

    #[test]
    fn test_update_or_create_huge_array_index_rejected() {
        /* Creating at a humongous index pads the array with null in a tight
         * loop (unbounded CPU + memory, same DoS as the Python backend).  The
         * backend must reject absurd gaps instead of allocating them. */
        let mut data = json!({"a": [1]});
        let cp = CsonPath::new("$.a[100000000]").unwrap();
        assert!(cp.update_or_create(&mut data, json!(42)).is_err());
    }

    #[test]
    fn test_empty_result() {
        let data = json!({"a": []});
        let cp = CsonPath::new("$.a[*]").unwrap();
        let r = cp.find_all(&data).unwrap();
        assert_eq!(r, None);

        let mut cp = CsonPath::new("$.a[*]").unwrap();
        cp.set_return_empty_array(true);
        let r = cp.find_all(&data).unwrap();
        assert_eq!(r, Some(json!([])));
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
        assert_eq!(r, None);
    }

    #[test]
    fn test_union() {
        let data = json!({"a": 1, "b": 2, "c": 3});
        let cp = CsonPath::new("$.['a','b']").unwrap();
        let r = cp.find_all(&data).unwrap();
        assert_eq!(r, Some(json!([1, 2])));
    }

    #[test]
    fn test_subpath() {
        let data = json!({"a": "b", "b": 42});
        let cp = CsonPath::new("$[$.a]").unwrap();
        let r = cp.find_all(&data).unwrap();
        assert_eq!(r, Some(json!([42])));
    }

    #[test]
    fn test_subpath_missing_prefix_does_not_crash() {
        /* The subpath $.nope resolves to nothing, so the walker feeds the
         * type-check accessors a NULL node.  rust_is_num/str dereference the
         * pointer unconditionally (json-c/Python checks are NULL-safe). */
        let data = json!({"a": "b", "b": 42});
        let cp = CsonPath::new("$[$.nope]").unwrap();
        // With runtime-error capture, the core reports the missing subpath
        // instead of silently returning Null; the important property here is
        // still that we do not panic or abort.
        assert!(matches!(
            cp.find_all(&data),
            Err(CsonpathError::RuntimeError(_))
        ));
    }

    #[test]
    fn test_remove_all_array_no_panic() {
        /* removing a whole array in one pass: rust_remove_child shrinks the
         * Vec (Vec::remove) while rust_array_iter_next still indexes it with
         * the length captured at iteration start.  Shrinking mid-iteration can
         * move the uncapped &a[i.idx] past the end -> index-out-of-bounds
         * panic. */
        let mut data = json!({"a": [1, 2, 3]});
        let cp = CsonPath::new("$.a.*").unwrap();
        let n = cp.remove(&mut data).unwrap();
        assert_eq!(n, 3);
        assert_eq!(data, json!({"a": []}));
    }

    #[test]
    fn test_filter_string_with_nul_no_panic() {
        /* a JSON string containing an embedded NUL (e.g. "\u0000") must not
         * make rust_get_str panic: CString::new() rejects interior NULs, so
         * the unwrap aborts the whole process through the extern "C" FFI
         * boundary.  json-c returns the raw char* and strcmp simply stops at
         * the first NUL; truncating to the first NUL reproduces that.
         * (the '>=' filter is used because '==' goes through rust_equal_str,
         * a direct Rust comparison that never touches GET_STR). */
        let data = json!([{"name": "fo\u{0}o"}]);
        let cp = CsonPath::new("$[?(@.name >= 'fo')]").unwrap();
        let r = cp.find_all(&data).unwrap();
        assert_eq!(r, Some(json!([{"name": "fo\u{0}o"}])));
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

    #[test]
    fn test_extra_roots_find_first() {
        let data = json!({"a": {"a": "oh"}});
        let mut cp = CsonPath::new("$.a[$1.b]").unwrap();
        cp.set_extra_roots(vec![json!({"b": "a"})]);
        let r = cp.find_first(&data).unwrap();
        assert_eq!(r, Some(json!("oh")));
    }

    #[test]
    fn test_extra_roots_find_all() {
        let data = json!({"items": {"a": 1, "b": 2}});
        let cp = CsonPath::builder("$.items[$1.key]")
            .extra_roots(vec![json!({"key": "a"})])
            .build()
            .unwrap();
        let r = cp.find_all(&data).unwrap();
        assert_eq!(r, Some(json!([1])));
    }

    #[test]
    fn test_extra_roots_replace() {
        let data = json!({"a": {"a": "oh", "b": "eh"}});
        let mut cp = CsonPath::new("$.a[$1.b]").unwrap();
        cp.set_extra_roots(vec![json!({"b": "a"})]);
        let r = cp.find_first(&data).unwrap();
        assert_eq!(r, Some(json!("oh")));

        cp.set_extra_roots(vec![json!({"b": "b"})]);
        let r = cp.find_first(&data).unwrap();
        assert_eq!(r, Some(json!("eh")));

        cp.clear_extra_roots();
        // Extra root index 0 is now out of bounds -> runtime error.
        assert!(matches!(
            cp.find_first(&data),
            Err(CsonpathError::RuntimeError(_))
        ));
    }

    #[test]
    fn test_auto_root() {
        let data = json!({"root": {"hello": "world"}});
        let cp = CsonPath::builder(".root.hello").auto_root(true).build().unwrap();
        let r = cp.find_first(&data).unwrap();
        assert_eq!(r, Some(json!("world")));
    }

    #[test]
    fn test_new_lenient_valid() {
        let cp = CsonPath::new_lenient("$.a");
        assert!(cp.is_valid());
        assert_eq!(cp.compile_error(), None);
    }

    #[test]
    fn test_new_lenient_invalid() {
        let cp = CsonPath::new_lenient("$.a[?!");
        assert!(!cp.is_valid());
        let err = cp.compile_error().expect("expected compile error");
        assert!(err.contains("column")); // message has column + explanation
    }

    #[test]
    fn test_exec_on_invalid_path_returns_runtime_error() {
        // The C core marks broken compiled paths with CSONPATH_INST_BROKEN
        // and reports the failure as a runtime error captured from the core.
        let cp = CsonPath::new_lenient("$.a[?!");
        let data = json!({"a": 1});
        assert!(matches!(
            cp.find_first(&data),
            Err(CsonpathError::RuntimeError(_))
        ));
        assert!(matches!(
            cp.find_all(&data),
            Err(CsonpathError::RuntimeError(_))
        ));
    }

    #[test]
    fn test_chain_compile_exec() {
        let data = json!({"items": [1, 2, 3]});
        let r = CsonPath::new("$.items[*]")
            .unwrap()
            .find_all(&data)
            .unwrap();
        assert_eq!(r, Some(json!([1, 2, 3])));
    }

    #[test]
    fn test_new_with_flags_basic() {
        let data = json!({"a": 1});
        let cp = CsonPath::new_with_flags("$.a", 0).unwrap();
        assert_eq!(cp.find_first(&data).unwrap(), Some(json!(1)));
    }

    #[test]
    fn test_new_with_flags_auto_root() {
        let data = json!({"root": {"hello": "world"}});
        let cp = CsonPath::new_with_flags(".root.hello", CSONPATH_AUTO_ROOT).unwrap();
        assert_eq!(cp.find_first(&data).unwrap(), Some(json!("world")));
    }

    #[test]
    fn test_new_with_flags_return_empty_array() {
        let data = json!({"a": []});
        let cp = CsonPath::new_with_flags("$.a[*]", CSONPATH_RETURN_EMPTY_ARRAY).unwrap();
        assert_eq!(cp.find_all(&data).unwrap(), Some(json!([])));
    }

    #[test]
    fn test_new_with_flags_invalid() {
        assert!(CsonPath::new_with_flags("$.a[?!", 0).is_err());
    }

    #[test]
    fn test_new_lenient_with_flags() {
        let data = json!({"a": []});
        let cp = CsonPath::new_lenient_with_flags("$.a[*]", CSONPATH_RETURN_EMPTY_ARRAY);
        assert!(cp.is_valid());
        assert_eq!(cp.find_all(&data).unwrap(), Some(json!([])));

        let broken = CsonPath::new_lenient_with_flags("$.a[?!", 0);
        assert!(!broken.is_valid());
    }

    #[test]
    fn test_runtime_error_capture() {
        clear_last_error();
        let cp = CsonPath::new("$1.foo").unwrap();
        let _ = cp.find_first(&json!({"foo": 1}));
        let err = last_error().expect("expected a runtime error");
        assert!(err.contains("extra root index 0 out of bounds"));
    }

    #[test]
    fn test_dollar_key_literal() {
        let data = json!({
            "aaaRefresh": {
                "$": {
                    "outCookie": "abc123",
                    "outRefreshPeriod": "600"
                }
            }
        });
        let cp = CsonPath::new("$.aaaRefresh[\"$\"].outCookie").unwrap();
        assert_eq!(cp.find_first(&data).unwrap(), Some(json!("abc123")));
    }
}
