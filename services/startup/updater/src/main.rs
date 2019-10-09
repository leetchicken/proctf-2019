use std::time;
use std::collections::HashMap;

use serde::{Serialize, Deserialize};

use tokio::{
    prelude::*,
    net::{TcpListener, TcpStream},
    codec,
    timer,
    fs
};

const LISTEN_ADDR: &str = "0.0.0.0:3255";
const TIMEOUT: time::Duration = time::Duration::from_secs(15);
const CHUNK_SIZE: usize = 1024;


#[derive(Serialize, Deserialize, Debug)]
struct Manifest {
    links: Vec<FileLink>,
}


#[derive(Serialize, Deserialize, Debug)]
struct FileLink {
    url: String,
    checksum: String
}


#[derive(Debug)]
struct ClientError (String);

impl std::fmt::Display for ClientError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let ClientError(msg) = self;
        write!(f, "{}", msg)
    }
}

impl From<&str> for ClientError {
    fn from(error: &str) -> Self {
        ClientError(error.to_string())
    }
}

impl From<codec::LinesCodecError> for ClientError {
    fn from(_error: codec::LinesCodecError) -> Self {
        ClientError("LinesCodecError".into())
    }
}


struct AsyncUnbufferedWriteStream {
    stream: Box<dyn AsyncWrite+Unpin+Send+Sync>,
}

impl AsyncUnbufferedWriteStream {
    fn new(stream: Box<dyn AsyncWrite+Unpin+Send+Sync>) -> AsyncUnbufferedWriteStream {
        AsyncUnbufferedWriteStream {
            stream: Box::new(stream)
        }
    }

    async fn write_all(&mut self, buf: &[u8]) -> Result<(), std::io::Error> {
        self.stream.write_all(buf).await?;
        self.stream.flush().await
    }
}


struct AsyncBufferedReadStream {
    stream: Box<dyn AsyncRead+Unpin+Send+Sync>,
    buf: [u8; CHUNK_SIZE],
    avail: usize
}

impl AsyncBufferedReadStream {
    fn new(stream: Box<dyn AsyncRead+Unpin+Send+Sync>) -> AsyncBufferedReadStream {
        AsyncBufferedReadStream{
            stream: stream, buf: [0; CHUNK_SIZE], avail: 0
        }
    }

    async fn fullfill_buffer(&mut self) -> Result<(), Box<dyn std::error::Error>>{
        self.avail += self.stream.read(&mut self.buf).await?;
        Ok(())
    }

    async fn read_line(&mut self) -> String {
        if self.avail == 0 {
            if self.fullfill_buffer().await.is_err() {
                return String::new()
            };
        }

        return match self.buf[..self.avail].splitn_mut(2, |b| *b == b'\n').next() {
            Some(line) => {
                let ret = String::from_utf8(line.to_vec()).unwrap_or_default();
                let mut next_chunk = line.len();

                while next_chunk < CHUNK_SIZE && self.buf[next_chunk] == b'\n' {
                    next_chunk += 1;
                }
                for (n, i) in (next_chunk..CHUNK_SIZE).enumerate() {
                    self.buf[n] = self.buf[i];
                }
                self.avail -= next_chunk;
                ret
            },
            None => String::new()
        }
    }

    async fn read(&mut self) -> Vec<u8> {
        if self.avail == 0 {
            if self.fullfill_buffer().await.is_err() {
                return vec![]
            };
        }
        let ret = self.buf[..self.avail].to_vec();
        self.avail -= ret.len();
        ret
    }
}


// protocol->pair of streams
type ProcessMap = HashMap<String, (AsyncUnbufferedWriteStream, AsyncBufferedReadStream)>;

struct ExternalDownloader {
    processes: Vec<tokio::net::process::Child>,
    process_channels: ProcessMap
}

impl ExternalDownloader {
    fn new() -> ExternalDownloader{
        ExternalDownloader {
            processes: vec![],
            process_channels: ProcessMap::new()
        }
    }

    fn get_link_protocol(link: &str) -> Option<&str> {
        if !link.contains("://") {
            return None
        }
        link.split("://").next()
    }

    async fn download(&mut self, link: &str) -> Result<String, String> {
        let downloader_name = match Self::get_link_protocol(link) {
            Some("http") => "transport/http",
            Some("http2") => "transport/http2",
            Some("debug") => "/bin/cat",
            _ => return Err("Unknown protocol".into())
        };

        if !self.process_channels.contains_key(downloader_name) {
            let mut cmd = tokio::net::process::Command::new(downloader_name);
            cmd.stdin(std::process::Stdio::piped()).stdout(std::process::Stdio::piped());
            match cmd.spawn() {
                Ok(mut child) => {
                    match (child.stdin().take(), child.stdout().take()) {
                        (Some(stdin), Some(stdout)) => {
                            self.processes.push(child);
                            
                            let write_stream = AsyncUnbufferedWriteStream::new(Box::new(stdin));
                            let read_stream = AsyncBufferedReadStream::new(Box::new(stdout));
                            self.process_channels.insert(downloader_name.into(),
                                                        (write_stream, read_stream));
                        }
                        _ => return Err("Process standard streams problem".into())
                    }
                }
                _ => return Err("Failed to run process".into())
            }
        }

        let (write_stream, read_stream) = self.process_channels.get_mut(downloader_name).unwrap();

        if write_stream.write_all(b"qqq").await.is_err() {
            return Err("Failed to write data to process".into());
        }
        println!("{}", read_stream.read_line().await);

        Ok(String::new())
    }

    async fn cleanup(&mut self) -> bool {
        let mut all_ok = true;
        while let Some(mut p) = self.processes.pop() {
            all_ok &= p.kill().is_ok();
            all_ok &= p.wait_with_output().await.is_ok();
        }
        all_ok
    }
}



fn check_data(host_port: String, signature: String, manifest: String ) -> Result<Manifest, String> {
    return Err("BAY".into());
}

fn is_signature_valid(raw_manifest: &str, signature: &str) -> bool {
    true
}

fn is_hostport_valid(hostport: &str) -> bool {
    let v: Vec<&str> = hostport.splitn(2, ':').collect();
    if v.len() != 2 {
        return false;
    }

    for c in v[0].chars() {
        match c {
            'a'..='z' | 'A'..='Z' | '0'..='9' | '.' | '-' => continue,
            _ => return false
        };
    }
    for c in v[1].chars() {
        match c {
            '0'..='9' => continue,
            _ => return false
        };
    }
    true
}

async fn handle_client(stream: tokio::net::TcpStream) -> Result<String, ClientError> {
    let mut lines = codec::Framed::new(stream, codec::LinesCodec::new());

    lines.send("Upgrade service ready".into()).await?;
    lines.send("Enter address of the mirror server (host:port):".into()).await?;
    
    let hostport = match lines.next().await {
        Some(Ok(h)) if is_hostport_valid(&h) => h,
        _ => {
            lines.send("Bad host or port".into()).await?;
            return Err("Bad host or port".into());
        }
    };
    
    lines.send("Enter manifest (JSON):".into()).await?;
    let raw_manifest = match lines.next().await {
        Some(Ok(r)) => r,
        _ => {
            lines.send("Bad manifest".into()).await?;
            return Err("Bad manifest".into());
        }
    };

    let manifest: Manifest = match serde_json::from_str(&raw_manifest) {
        Ok(obj) => obj,
        Err(_) => {
            lines.send("Bad manifest".into()).await?;
            return Err("Bad manifest".into())
        }
    };

    lines.send("Enter manifest signature (32 hex):".into()).await?;
    let signature = match lines.next().await {
        Some(Ok(s)) => s,
        _ => {
            lines.send("No signature".into()).await?;
            return Err("No signature".into());
        }
    };

    if !is_signature_valid(&raw_manifest, &signature) {
        lines.send("Bad signature".into()).await?;
        return Err("Bad signature".into());        
    }

    lines.send("Signature is ok, downloading".into()).await?;

    let mut downloader = ExternalDownloader::new();
    for link in manifest.links {
        let url = link.url.replacen("mirror", &hostport, 1);
        println!("{:?}", downloader.download(&url).await);
    }
    downloader.cleanup().await;

    Ok("done".into())
}


async fn handle_client_wrapper(stream: tokio::net::TcpStream) {
    match handle_client(stream).await {
        Ok(s) => println!("OK: {}", s),
        Err(s) => println!("ERR: {}", s)
    };
}



async fn command() {
    let mut downloader = ExternalDownloader::new();
    println!("{:?}", downloader.download("debug://fafads").await);
    timer::delay_for(time::Duration::from_millis(1000)).await;
    downloader.cleanup().await;
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>>{
    tokio::spawn(async move { command().await });

    let mut listener = TcpListener::bind(LISTEN_ADDR).await?;

    loop {
        let (stream, addr) = match listener.accept().await {
            Ok((s, a)) => (s, a),
            Err(_) => continue,
        };
        println!("Connection from {}", addr);

        tokio::spawn(async move {
            let res = handle_client_wrapper(stream).timeout(TIMEOUT).await;
            if res.is_err() {
               println!("client timed out");
            }
        });
    }
}
