use std::path::{Component, Path, PathBuf};
use std::sync::Arc;

use axum::{
    body::Body,
    extract::{Path as AxumPath, State},
    http::{header, StatusCode},
    response::Response,
    routing::get,
    Router,
};
use bytes::Bytes;
use mime_guess::from_path;
use tokio::net::TcpListener;

use crate::cache::MemoryCache;
use crate::config::Config;

#[derive(Clone)]
struct AppState {
    origin: PathBuf,
    cache: MemoryCache,
}

pub async fn run(cfg: Config) -> anyhow::Result<()> {
    tokio::fs::create_dir_all(&cfg.origin).await?;

    let state = AppState {
        origin: cfg.origin,
        cache: MemoryCache::new(cfg.cache_capacity),
    };

    let app = Router::new()
        .route("/health", get(health))
        .route("/{*path}", get(get_object))
        .with_state(state);

    let addr = format!("0.0.0.0:{}", cfg.port);
    let listener = TcpListener::bind(&addr).await?;
    tracing::info!(%addr, "listening");
    axum::serve(listener, app).await?;
    Ok(())
}

async fn health() -> &'static str {
    "ok"
}

async fn get_object(
    State(state): State<AppState>,
    AxumPath(path): AxumPath<String>,
) -> Result<Response, StatusCode> {
    let key = normalize_key(&path).ok_or(StatusCode::BAD_REQUEST)?;

    if let Some(body) = state.cache.get(&key).await {
        tracing::debug!(%key, "cache hit");
        return Ok(cached_response(&key, body));
    }

    let file_path = state.origin.join(&key);
    let data = tokio::fs::read(&file_path).await.map_err(|e| {
        tracing::debug!(%key, error = %e, "origin miss");
        StatusCode::NOT_FOUND
    })?;

    let bytes = Bytes::from(data);
    state.cache.insert(key.clone(), bytes.clone()).await;
    tracing::debug!(%key, "cache fill from origin");
    Ok(cached_response(&key, bytes))
}

fn cached_response(key: &str, body: Bytes) -> Response {
    let mime = from_path(key).first_or_octet_stream();
    Response::builder()
        .status(StatusCode::OK)
        .header(header::CONTENT_TYPE, mime.as_ref())
        .header(header::CACHE_CONTROL, "public, max-age=60")
        .header("X-Cache", "edge")
        .body(Body::from(body))
        .expect("valid response")
}

/// Reject `..` and absolute paths — map URL path to safe relative key.
fn normalize_key(path: &str) -> Option<String> {
    let p = Path::new(path);
    let mut parts = Vec::new();
    for comp in p.components() {
        match comp {
            Component::Normal(s) => parts.push(s.to_string_lossy().into_owned()),
            Component::CurDir => {}
            Component::ParentDir | Component::RootDir | Component::Prefix(_) => return None,
        }
    }
    if parts.is_empty() {
        None
    } else {
        Some(parts.join("/"))
    }
}
