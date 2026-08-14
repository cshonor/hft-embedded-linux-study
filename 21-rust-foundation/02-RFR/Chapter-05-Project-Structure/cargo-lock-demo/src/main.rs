use serde::Serialize;

#[derive(Serialize)]
struct Demo {
    ok: bool,
}

fn main() {
    let _ = Demo { ok: true };
}
