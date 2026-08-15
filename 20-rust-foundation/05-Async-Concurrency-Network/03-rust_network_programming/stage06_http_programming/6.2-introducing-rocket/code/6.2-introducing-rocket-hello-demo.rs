//! §6.2：最小 Rocket 服务（`127.0.0.1:18082`）。
//! 书中完整示例还含 diesel / Tera；本 demo 只演示路由宏 + `#[launch]`。
//!
//! 运行后访问：http://127.0.0.1:18082/ 与 http://127.0.0.1:18082/api/ping

#[macro_use]
extern crate rocket;

use rocket::serde::{json::Json, Serialize};

#[derive(Serialize)]
struct Ping {
    ok: bool,
    section: &'static str,
}

#[get("/")]
fn index() -> &'static str {
    "Hello from 6.2 Rocket demo — see 6.2-introducing-rocket.md"
}

#[get("/api/ping")]
fn ping() -> Json<Ping> {
    Json(Ping {
        ok: true,
        section: "6.2",
    })
}

#[launch]
fn rocket() -> _ {
    let config = rocket::Config {
        port: 18082,
        ..rocket::Config::default()
    };
    rocket::custom(config).mount("/", routes![index, ping])
}
