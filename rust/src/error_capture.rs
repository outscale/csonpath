use std::cell::RefCell;
use std::ffi::CStr;
use std::os::raw::c_char;

thread_local! {
    static LAST_ERROR: RefCell<String> = RefCell::new(String::new());
}

/// Called from C via `CSONPATH_FORMAT_EXCEPTION` to capture runtime error messages
/// instead of writing them to stderr.
#[no_mangle]
pub extern "C" fn rust_log_error(msg: *const c_char) {
    if msg.is_null() {
        return;
    }
    let s = unsafe { CStr::from_ptr(msg).to_string_lossy().into_owned() };
    LAST_ERROR.with(|err| *err.borrow_mut() = s);
}

/// Returns the last runtime error captured from the C core, if any.
pub fn last_error() -> Option<String> {
    LAST_ERROR.with(|err| {
        let s = err.borrow().clone();
        if s.is_empty() {
            None
        } else {
            Some(s)
        }
    })
}

/// Clears the last captured runtime error.
pub fn clear_last_error() {
    LAST_ERROR.with(|err| err.borrow_mut().clear());
}
