//! 加法 feature：`schema` 开启额外 API，不用 `no_schema` 否定名

#[derive(Debug, PartialEq)]
pub struct Record {
    pub data: Vec<u8>,
}

#[cfg(feature = "schema")]
pub fn with_schema_tag(tag: &str, data: Vec<u8>) -> Record {
    let mut payload = tag.as_bytes().to_vec();
    payload.extend(data);
    Record { data: payload }
}

pub fn plain(data: Vec<u8>) -> Record {
    Record { data }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn plain_always() {
        assert_eq!(plain(vec![1]), Record { data: vec![1] });
    }

    #[cfg(feature = "schema")]
    #[test]
    fn schema_extends() {
        let r = with_schema_tag("v1", vec![2]);
        assert!(r.data.starts_with(b"v1"));
    }
}
