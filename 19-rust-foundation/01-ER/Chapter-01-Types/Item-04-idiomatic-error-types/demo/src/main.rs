//! 应用层：anyhow 聚合多种错误，带 context 链

use anyhow::{Context, Result};
use item_04_error_types::{parse_port, ConfigError};

fn load_config(port_str: &str) -> Result<u16> {
    parse_port(port_str).context("failed to parse server port")
}

fn main() -> Result<()> {
    match load_config("0") {
        Ok(p) => println!("port = {p}"),
        Err(e) => {
            println!("anyhow chain:\n{e:?}");
            for cause in e.chain() {
                println!("  caused by: {cause}");
            }
        }
    }

    let lib_err = ConfigError::MissingField("host");
    println!("thiserror display: {lib_err}");
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn context_wraps_library_error() {
        let err = load_config("abc").unwrap_err();
        assert!(err.to_string().contains("server port"));
    }
}
