use std::{
    ffi::{CString},
    os::raw::c_char,
};

use once_cell::sync::Lazy;

static MSG: Lazy<CString> = Lazy::new(|| CString::new("hoge").unwrap());

#[no_mangle]
pub extern "C" fn rust_function() -> *const c_char {
    MSG.as_ptr()
}

#[cfg(test)]
mod tests {
    #[test]
    fn it_works() {
        assert_eq!(2 + 2, 4);
    }
}
