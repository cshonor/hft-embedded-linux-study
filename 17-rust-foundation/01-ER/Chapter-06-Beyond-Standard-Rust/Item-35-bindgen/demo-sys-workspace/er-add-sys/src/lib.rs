//! `-sys` crate：仅 bindgen 生成 + 原始 FFI（unsafe 由调用方负责）

#![allow(non_camel_case_types, non_snake_case, dead_code)]

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
