//! §5.3：FTP vs TFTP 步骤速查（无 live 服务器）。

fn main() {
    println!("FTP (TCP):");
    println!("  1. connect :21 control");
    println!("  2. USER / PASS");
    println!("  3. PASV or PORT → data channel");
    println!("  4. STOR/RETR on data connection");
    println!();
    println!("TFTP (UDP):");
    println!("  1. RRQ/WRQ datagram");
    println!("  2. DATA blocks + ACK");
    println!("  3. no auth — embedded / PXE");
    println!();
    println!("Crates: rust-ftp, tftp_server — use per book in separate projects.");
}
