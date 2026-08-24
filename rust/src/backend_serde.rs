use std::ffi::{c_char, c_int, c_void, CStr, CString};
use std::str;
use serde_json::{map::Iter as MapIter, Value};
use crate::ffi::CsonpathChildInfo;

#[cfg(test)]
use std::sync::atomic::{AtomicIsize, Ordering};

#[cfg(test)]
pub static ALLOC_COUNT: AtomicIsize = AtomicIsize::new(0);

/* ===================================================================== */
/*  Lifecycle & memory                                                   */
/* ===================================================================== */

#[no_mangle]
pub extern "C" fn rust_decref(obj: *mut Value) {
    if !obj.is_null() {
        unsafe { let _ = Box::from_raw(obj); }
    }
}

#[no_mangle]
pub extern "C" fn rust_new_object() -> *mut Value {
    Box::into_raw(Box::new(Value::Object(serde_json::Map::new())))
}

#[no_mangle]
pub extern "C" fn rust_new_array() -> *mut Value {
    Box::into_raw(Box::new(Value::Array(vec![])))
}

/* ===================================================================== */
/*  Construction helpers                                                 */
/* ===================================================================== */

#[no_mangle]
pub extern "C" fn rust_array_append(list: *mut Value, el: *mut Value) {
    unsafe {
        let arr = &mut *list;
        let item = Box::from_raw(el);
        if let Value::Array(ref mut a) = arr {
            a.push(*item);
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_array_append_incref(list: *mut Value, el: *mut Value) {
    unsafe {
        let arr = &mut *list;
        if let Value::Array(ref mut a) = arr {
            a.push((*el).clone());
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_array_clear(o: *mut Value) {
    unsafe {
        if let Value::Array(ref mut a) = *o {
            a.clear();
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_obj_clear(o: *mut Value) {
    unsafe {
        if let Value::Object(ref mut m) = *o {
            m.clear();
        }
    }
}

/* ===================================================================== */
/*  Getters (return pointers into the ORIGINAL tree)                     */
/* ===================================================================== */

#[no_mangle]
pub extern "C" fn rust_get(obj: *mut Value, key: *const c_char) -> *mut Value {
    unsafe {
        if let Value::Object(ref m) = *obj {
            let c = CStr::from_ptr(key);
            let k = str::from_utf8_unchecked(c.to_bytes());
            match m.get(k) {
                Some(v) => v as *const Value as *mut Value,
                None => std::ptr::null_mut(),
            }
        } else {
            std::ptr::null_mut()
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_at(arr: *mut Value, idx: c_int) -> *mut Value {
    unsafe {
        if let Value::Array(ref a) = *arr {
            let i = idx as usize;
            if i < a.len() {
                &a[i] as *const Value as *mut Value
            } else {
                std::ptr::null_mut()
            }
        } else {
            std::ptr::null_mut()
        }
    }
}

/* ===================================================================== */
/*  Type checks                                                          */
/* ===================================================================== */

#[no_mangle]
pub extern "C" fn rust_is_obj(o: *mut Value) -> c_int {
    unsafe { if let Value::Object(_) = *o { 1 } else { 0 } }
}

#[no_mangle]
pub extern "C" fn rust_is_array(o: *mut Value) -> c_int {
    unsafe { if let Value::Array(_) = *o { 1 } else { 0 } }
}

#[no_mangle]
pub extern "C" fn rust_is_str(o: *mut Value) -> c_int {
    unsafe { if let Value::String(_) = *o { 1 } else { 0 } }
}

#[no_mangle]
pub extern "C" fn rust_is_num(o: *mut Value) -> c_int {
    unsafe { if let Value::Number(_) = *o { 1 } else { 0 } }
}

#[no_mangle]
pub extern "C" fn rust_is_bool(o: *mut Value) -> c_int {
    unsafe { if let Value::Bool(_) = *o { 1 } else { 0 } }
}

#[no_mangle]
pub extern "C" fn rust_is_null(o: *mut Value) -> c_int {
    if o.is_null() {
        1
    } else {
        unsafe { if let Value::Null = *o { 1 } else { 0 } }
    }
}

#[no_mangle]
pub extern "C" fn rust_get_bool(o: *mut Value) -> c_int {
    unsafe {
        match *o {
            Value::Bool(b) => if b { 1 } else { 0 },
            _ => 0,
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_get_str(o: *mut Value) -> *const c_char {
    unsafe {
        if let Value::String(ref s) = *o {
            let c = CString::new(s.as_str()).unwrap();
            let p = c.as_ptr();
            #[cfg(test)]
            ALLOC_COUNT.fetch_add(1, Ordering::SeqCst);
            std::mem::forget(c);
            p
        } else {
            std::ptr::null()
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_free_str(s: *const c_char) {
    if !s.is_null() {
        unsafe {
            #[cfg(test)]
            ALLOC_COUNT.fetch_sub(1, Ordering::SeqCst);
            let _ = CString::from_raw(s as *mut c_char);
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_get_num(o: *mut Value) -> i64 {
    unsafe {
        match *o {
            Value::Number(ref n) => n.as_i64().unwrap_or(0),
            _ => 0,
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_equal_num(o: *mut Value, v: i64) -> c_int {
    unsafe {
        match *o {
            Value::Number(ref n) => if n.as_i64() == Some(v) { 1 } else { 0 },
            _ => 0,
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_equal_str(o: *mut Value, s: *const c_char) -> c_int {
    unsafe {
        if let Value::String(ref os) = *o {
            let c = CStr::from_ptr(s);
            let cs = str::from_utf8_unchecked(c.to_bytes());
            if os == cs { 1 } else { 0 }
        } else {
            0
        }
    }
}

/* ===================================================================== */
/*  Opaque iterators                                                     */
/* ===================================================================== */

#[repr(C)]
pub struct RustArrayIter {
    arr: *mut Value,
    len: usize,
    idx: usize,
}

#[no_mangle]
pub extern "C" fn rust_array_iter_init(it: *mut RustArrayIter, arr: *mut Value) {
    unsafe {
        (*it).arr = arr;
        (*it).len = match *arr {
            Value::Array(ref a) => a.len(),
            _ => 0,
        };
        (*it).idx = 0;
    }
}

#[no_mangle]
pub extern "C" fn rust_array_iter_next(
    it: *mut RustArrayIter,
    out_el: *mut *mut Value,
    out_idx: *mut usize,
) -> c_int {
    unsafe {
        let i = &mut *it;
        if i.idx >= i.len { return 0; }
        if let Value::Array(ref a) = *i.arr {
            *out_el = &a[i.idx] as *const Value as *mut Value;
            *out_idx = i.idx;
            i.idx += 1;
            return 1;
        }
        0
    }
}

/* Real Map iterator. Invalid if the map is mutated while alive; the C core
 * re-initialises the iterator via need_reloop when mutating. */
#[repr(C)]
pub struct RustObjIter {
    obj: *mut Value,
    iter: *mut c_void,   /* Box<MapIter<'static>> */
}

#[no_mangle]
pub extern "C" fn rust_obj_iter_init(it: *mut RustObjIter, obj: *mut Value) {
    unsafe {
        let it = &mut *it;
        it.obj = obj;
        it.iter = std::ptr::null_mut();
        match *obj {
            Value::Object(ref m) => {
                let iter = m.iter();
                let iter_static: MapIter<'static> = std::mem::transmute(iter);
                it.iter = Box::into_raw(Box::new(iter_static)) as *mut c_void;
            }
            _ => {}
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_obj_iter_next(
    it: *mut RustObjIter,
    out_val: *mut *mut Value,
    out_key: *mut *const c_char,
    out_key_len: *mut usize,
) -> c_int {
    unsafe {
        let i = &mut *it;
        if i.iter.is_null() {
            return 0;
        }
        let iter = &mut *(i.iter as *mut MapIter<'static>);
        if let Some((k, v)) = iter.next() {
            *out_key = k.as_bytes().as_ptr() as *const c_char;
            *out_key_len = k.len();
            *out_val = v as *const Value as *mut Value;
            return 1;
        }
        0
    }
}

#[no_mangle]
pub extern "C" fn rust_obj_iter_free(it: *mut RustObjIter) {
    unsafe {
        if !(*it).iter.is_null() {
            let _ = Box::from_raw((*it).iter as *mut MapIter<'static>);
            (*it).iter = std::ptr::null_mut();
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_obj_iter_cleanup(it: *mut RustObjIter) {
    rust_obj_iter_free(it);
}

/* ===================================================================== */
/*  Mutation helpers                                                     */
/* ===================================================================== */

#[no_mangle]
pub extern "C" fn rust_remove_child(o: *mut Value, ci: *mut c_void) {
    unsafe {
        let info = &*(ci as *const CsonpathChildInfo);
        match info.type_ {
            1 /* CSONPATH_INTEGER */ => {
                if let Value::Array(ref mut a) = *o {
                    let idx = info.data.idx as usize;
                    if idx < a.len() {
                        a.remove(idx);
                    }
                }
            }
            2 /* CSONPATH_STR */ => {
                if let Value::Object(ref mut m) = *o {
                    let c = CStr::from_ptr(info.data.key);
                    let key = str::from_utf8_unchecked(c.to_bytes());
                    m.remove(key);
                }
            }
            _ => {}
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_append_at_int(arr: *mut Value, idx: c_int, el: *mut Value, do_incref: c_int) -> c_int {
    unsafe {
        match *arr {
            Value::Array(ref mut a) => {
                let i = idx as usize;
                while a.len() <= i {
                    a.push(Value::Null);
                }
                if do_incref != 0 {
                    a[i] = (*el).clone();
                } else {
                    a[i] = *Box::from_raw(el);
                }
                0
            }
            Value::Object(ref mut m) => {
                let key = idx.to_string();
                if do_incref != 0 {
                    m.insert(key, (*el).clone());
                } else {
                    m.insert(key, *Box::from_raw(el));
                }
                0
            }
            _ => -1,
        }
    }
}

#[no_mangle]
pub extern "C" fn rust_append_at_str(arr: *mut Value, key: *const c_char, el: *mut Value, do_incref: c_int) -> c_int {
    unsafe {
        if let Value::Object(ref mut m) = *arr {
            let c = CStr::from_ptr(key);
            let k = str::from_utf8_unchecked(c.to_bytes()).to_string();
            if do_incref != 0 {
                m.insert(k, (*el).clone());
            } else {
                m.insert(k, *Box::from_raw(el));
            }
            0
        } else {
            -1
        }
    }
}

/* ===================================================================== */
/*  Misc                                                                 */
/* ===================================================================== */

#[no_mangle]
pub extern "C" fn rust_need_foreach_redo(o: *mut Value) -> c_int {
    unsafe {
        if let Value::Object(_) = *o { 1 } else { 0 }
    }
}

type CallbackTrampoline = unsafe extern "C" fn(
    ctx: *mut Value,
    ci: *mut c_void,
    tmp: *mut Value,
    udata: *mut c_void,
) -> c_int;

#[no_mangle]
pub extern "C" fn rust_call_callback(
    cb: *mut c_void,
    ctx: *mut c_void,
    ci: *mut c_void,
    tmp: *mut c_void,
    udata: *mut c_void,
) -> c_int {
    unsafe {
        let trampoline: CallbackTrampoline = std::mem::transmute(cb);
        trampoline(ctx as *mut Value, ci, tmp as *mut Value, udata)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::sync::atomic::Ordering;

    #[test]
    fn test_get_str_no_leak() {
        ALLOC_COUNT.store(0, Ordering::SeqCst);
        let v = Box::into_raw(Box::new(Value::String("hello world".into())));
        unsafe {
            let before = ALLOC_COUNT.load(Ordering::SeqCst);
            let p1 = rust_get_str(v);
            assert_eq!(ALLOC_COUNT.load(Ordering::SeqCst), before + 1);
            let p2 = rust_get_str(v);
            assert_eq!(ALLOC_COUNT.load(Ordering::SeqCst), before + 2);
            let p3 = rust_get_str(v);
            assert_eq!(ALLOC_COUNT.load(Ordering::SeqCst), before + 3);

            assert!(!p1.is_null());
            assert!(!p2.is_null());
            assert!(!p3.is_null());

            rust_free_str(p1);
            assert_eq!(ALLOC_COUNT.load(Ordering::SeqCst), before + 2);
            rust_free_str(p2);
            assert_eq!(ALLOC_COUNT.load(Ordering::SeqCst), before + 1);
            rust_free_str(p3);
            assert_eq!(ALLOC_COUNT.load(Ordering::SeqCst), before);

            let _ = Box::from_raw(v);
        }
    }
}
