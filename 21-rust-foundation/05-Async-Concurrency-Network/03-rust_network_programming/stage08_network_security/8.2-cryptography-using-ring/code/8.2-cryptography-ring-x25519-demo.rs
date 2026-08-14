//! §8.2：`ring` X25519 密钥交换（两端 shared secret 相同）。
use ring::agreement::{self, EphemeralPrivateKey, UnparsedPublicKey, X25519};
use ring::rand::SystemRandom;

fn main() -> Result<(), ring::error::Unspecified> {
    let rng = SystemRandom::new();
    let client_priv = EphemeralPrivateKey::generate(&X25519, &rng)?;
    let server_priv = EphemeralPrivateKey::generate(&X25519, &rng)?;
    let client_pub = client_priv.compute_public_key()?;
    let server_pub = server_priv.compute_public_key()?;

    let client_shared = agreement::agree_ephemeral(
        client_priv,
        &UnparsedPublicKey::new(&X25519, server_pub.as_ref()),
        |key| key.to_vec(),
    )?;

    let server_shared = agreement::agree_ephemeral(
        server_priv,
        &UnparsedPublicKey::new(&X25519, client_pub.as_ref()),
        |key| key.to_vec(),
    )?;

    assert_eq!(client_shared, server_shared);
    println!("X25519 shared secret len: {}", client_shared.len());
    Ok(())
}
