use std::num::NonZeroUsize;
use std::sync::Arc;

use bytes::Bytes;
use lru::LruCache;
use tokio::sync::Mutex;

#[derive(Clone)]
pub struct MemoryCache {
    inner: Arc<Mutex<LruCache<String, Bytes>>>,
}

impl MemoryCache {
    pub fn new(capacity: usize) -> Self {
        let cap = NonZeroUsize::new(capacity.max(1)).expect("capacity");
        Self {
            inner: Arc::new(Mutex::new(LruCache::new(cap))),
        }
    }

    pub async fn get(&self, key: &str) -> Option<Bytes> {
        self.inner.lock().await.get(key).cloned()
    }

    pub async fn insert(&self, key: String, value: Bytes) {
        self.inner.lock().await.put(key, value);
    }
}
