//! Rust HashMap wrapped in an unsafe C API
//!
//! Generate `rust_hash_map.h` from this using:
//!
//!     cbindgen --lang c rust_hash_map.rs
//!

#![crate_type = "staticlib"]

use std::collections::HashMap;
use std::ffi::{CStr, CString};
use std::os::raw::c_char;
use std::ptr;

pub struct IntHash {
    map: HashMap<i64, i64>,
}

pub struct StrHash {
    map: HashMap<CString, i64>,
}

#[no_mangle]
pub extern "C" fn new_int_hash() -> *mut IntHash {
    let hash = Box::new(IntHash {
        map: HashMap::new(),
    });
    Box::leak(hash)
}

#[no_mangle]
pub extern "C" fn insert_int_into_hash(hash: *mut IntHash, key: i64, value: i64) {
    unsafe {
        (*hash).map.insert(key, value);
    }
}

#[no_mangle]
pub extern "C" fn delete_int_from_hash(hash: *mut IntHash, key: i64) {
    unsafe {
        (*hash).map.remove(&key);
    }
}

#[no_mangle]
pub extern "C" fn lookup_int(hash: *const IntHash, key: i64) -> *const i64 {
    unsafe {
        match (*hash).map.get(&key) {
            Some(value) => value,
            None => ptr::null(),
        }
    }
}

#[no_mangle]
pub extern "C" fn new_str_hash() -> *mut StrHash {
    let hash = Box::new(StrHash {
        map: HashMap::new(),
    });
    Box::leak(hash)
}

#[no_mangle]
pub extern "C" fn insert_str_into_hash(hash: *mut StrHash, key: *const c_char, value: i64) {
    unsafe {
        (*hash).map.insert(CStr::from_ptr(key).to_owned(), value);
    }
}

#[no_mangle]
pub extern "C" fn delete_str_from_hash(hash: *mut StrHash, key: *const c_char) {
    unsafe {
        (*hash).map.remove(CStr::from_ptr(key));
    }
}
