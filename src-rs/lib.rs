use std::os::raw::c_char;

#[no_mangle]
pub extern "C" fn rust_function() -> *const c_char {
    b"hoge\n".as_ptr() as *const c_char
}

#[cfg(test)]
mod tests {
    #[test]
    fn it_works() {
        assert_eq!(2 + 2, 4);
    }
}
