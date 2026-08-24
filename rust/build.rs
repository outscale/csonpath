use std::env;
use std::path::PathBuf;

fn main() {
    let out_dir = PathBuf::from(env::var("OUT_DIR").unwrap());
    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    let root_dir = manifest_dir.parent().unwrap();

    println!("cargo:rerun-if-changed=csonpath_rust_backend.c");
    println!("cargo:rerun-if-changed=csonpath_rust_backend.h");
    println!("cargo:rerun-if-changed={}", root_dir.join("csonpath.h").display());
    println!("cargo:rerun-if-changed={}", root_dir.join("csonpath_do.h").display());

    cc::Build::new()
        .file("csonpath_rust_backend.c")
        .include(root_dir)
        .include(&manifest_dir)
        .flag("-std=c11")
        .flag("-O2")
        .flag("-fno-strict-aliasing")
        .flag("-Wno-unused-parameter")
        .flag("-Wno-unused-function")
        .flag("-Wno-unused-variable")
        .compile("csonpath_rust_backend");

    println!("cargo:rustc-link-lib=static=csonpath_rust_backend");
    println!("cargo:rustc-link-search=native={}", out_dir.display());

    /* The C backend references #[no_mangle] Rust symbols. On macOS the static
     * linker needs to be told to resolve them at runtime. */
    if env::var("CARGO_CFG_TARGET_VENDOR").unwrap_or_default() == "apple" {
        println!("cargo:rustc-link-arg=-Wl,-undefined,dynamic_lookup");
    }
}
