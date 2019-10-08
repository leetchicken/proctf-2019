use std::io::{self, Write};
use nix::unistd::alarm;

use h2::client;

use hashsum;

use http::{Request, Uri};
use tokio::net::TcpStream;

const MAX_RESP_SIZE: usize = 100_000;

#[tokio::main]
async fn main() -> Result<(), ()> {
    alarm::set(8);

    loop {
        let mut input = String::new();
        let url = match io::stdin().read_line(&mut input) {
            Ok(0) => return Ok(()),
            Err(_) => return Err(()),
            Ok(_) => input.trim()
        };

        println!("Url: {}", url);
        let url = match url.parse::<Uri>() {
            Ok(u) => u,
            Err(_) => return Err(())
        };

        let host_port = match url.authority_part() {
            Some(a) => a.to_string(),
            None => return Err(())
        };

        let tcp = match TcpStream::connect(host_port).await {
            Ok(t) => t,
            Err(_) => return Err(())
        };
        let (mut h2, connection) = match client::handshake(tcp).await {
            Ok((h, c)) => (h, c),
            Err(_) => return Err(())
        };
        tokio::spawn(async move {
            connection.await.unwrap();
        });

        let request = match Request::get(url).body(()) {
            Ok(r) => r,
            Err(_) => return Err(())
        };

        let (response, _) = match h2.send_request(request, true) {
            Ok((h, c)) => (h, c),
            Err(_) => return Err(())
        };

        let (head, mut body) = match response.await {
            Ok(r) => r.into_parts(),
            Err(_) => return Err(())
        };

        let mut release_capacity = body.release_capacity().clone();

        println!("Success: {}", head.status.is_success());

        let mut data : Vec<u8> = vec![];

        while let Some(chunk) = body.data().await {
            match chunk {
                Ok(c) => {
                    if data.len() + c.len() > MAX_RESP_SIZE {
                        return Err(());
                    }
                    data.extend_from_slice(&c);
                    if release_capacity.release_capacity(c.len()).is_err() {
                        return Err(());
                    }
                },
                Err(_) => return Err(())
            }
        }

        println!("Hashsum: {}", hashsum::calc_hash(&data));

        println!("Content-Length: {}", data.len());
        if io::stdout().write_all(&data).is_err() {
            return Err(());
        }

        if io::stdout().flush().is_err() {
            return Err(());
        }
    }
}


