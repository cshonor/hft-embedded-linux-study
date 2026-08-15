//! §5.2：用 `lettre` 组装邮件（不发送）。
use lettre::message::SinglePart;
use lettre::Message;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let email = Message::builder()
        .from("crash-report@example.com".parse()?)
        .to("ops@example.com".parse()?)
        .subject("5.2 SMTP demo — CrashReport")
        .singlepart(SinglePart::plain(String::from(
            "Server panic at line 42 (dry-run, not sent)",
        )))?;

    println!("Built lettre Message (dry-run, not sent):");
    println!("{email:?}");
    println!("Next: SmtpTransport::relay(...) + credentials + send.");
    Ok(())
}
