//! 4.8 Polling Sockets：`mio::Poll` + `Token` + 非阻塞 `TcpListener`。

use std::time::Duration;

use mio::net::TcpListener;
use mio::{Events, Interest, Poll, Token};

const LISTENER: Token = Token(0);

fn main() -> std::io::Result<()> {
    let mut poll = Poll::new()?;
    let mut events = Events::with_capacity(8);

    let mut server = TcpListener::bind("127.0.0.1:0".parse().unwrap())?;
    poll.registry()
        .register(&mut server, LISTENER, Interest::READABLE)?;

    let local = server.local_addr()?;
    println!("listening on {local} (nc 127.0.0.1 {})", local.port());

    poll.poll(&mut events, Some(Duration::from_millis(500)))?;
    println!("poll: {} event(s)", events.iter().count());
    Ok(())
}
