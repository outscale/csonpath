use std::ffi::{c_char, c_int, c_void};

#[repr(C)]
pub struct Csonpath {
    pub compile_error: *const c_char,
    pub return_empty_array: c_int,
    pub regex_cnt: c_int,
    pub regexs: *mut c_void,
    /* The C struct has a flexible array member `char data[]` here.
     * Rust never allocates this struct or indexes into `data`, so it is
     * intentionally omitted. */
}

#[repr(C)]
pub union CsonpathChildInfoData {
    pub idx: c_int,
    pub key: *const c_char,
}

#[repr(C)]
pub struct CsonpathChildInfo {
    pub type_: c_int,
    pub data: CsonpathChildInfoData,
}

extern "C" {
    pub fn csonpath_new(path: *const c_char) -> *mut Csonpath;
    pub fn csonpath_new_ex(path: *const c_char, flags: c_int) -> *mut Csonpath;
    pub fn csonpath_set_path(cp: *mut Csonpath, path: *const c_char) -> *mut Csonpath;
    pub fn csonpath_destroy(cp: *mut Csonpath);
    pub fn csonpath_find_first(cp: *mut Csonpath, origin: *mut c_void) -> *mut c_void;
    pub fn csonpath_find_all(cp: *mut Csonpath, origin: *mut c_void) -> *mut c_void;
    pub fn csonpath_remove(cp: *mut Csonpath, origin: *mut c_void) -> c_int;
    pub fn csonpath_update_or_create(
        cp: *mut Csonpath,
        origin: *mut c_void,
        new_val: *mut c_void,
    ) -> c_int;
    pub fn csonpath_callback(
        cp: *mut Csonpath,
        origin: *mut c_void,
        callback: *mut c_void,
        udata: *mut c_void,
    ) -> c_int;
    pub fn csonpath_update_or_create_callback(
        cp: *mut Csonpath,
        origin: *mut c_void,
        callback: *mut c_void,
        udata: *mut c_void,
    ) -> c_int;
}
