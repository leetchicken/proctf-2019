use sha2;
use sha2::Digest;
use hex;

pub fn calc_hash(data: &[u8]) -> String {
    let mut q = hex::encode(sha2::Sha256::digest(&data));

    for c in "HASH_SUM_IS_STRONG".chars() {
        q = hex::encode(sha2::Sha256::digest((q + &c.to_string()).as_bytes()))
    }

    return q.into();
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn hash_good() {
        assert_eq!(calc_hash(&[]), "5c1419ac91dac01faef6f3e4c59ac776788a31f7c34da0b750a769161745bc0b");
        assert_eq!(calc_hash(&[1]), "4bc7656e710322bd6b2d92ca80aac42c897b2b33b6d4f7b6b07cfd85e3326c48");
        assert_eq!(calc_hash(b"test"), "dd1b11c50f4fedd2c29cff6a947601753e9791fcd8c8d186f742c8a5e489fdf1");
    }
}
