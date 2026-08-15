use crate::configuration::Settings;

pub fn run() {
    let settings = Settings::load();
    println!(
        "[startup] bind {}:{} (Actix-web 在此启动，demo 仅打印)",
        settings.host, settings.port
    );
    crate::routes::health_check::log_ready();
}
