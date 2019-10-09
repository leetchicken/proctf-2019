use sha2;
use sha2::Digest;
use hex;

pub fn calc_hash(data: &[u8]) -> String {
    let mut q = hex::encode(sha2::Sha256::digest(&data));

    for c in "HASH_SUM_IS_STRONG".chars() {
        q = hex::encode(sha2::Sha256::digest((q + &c.to_string()).as_bytes()))
    }

    return q[..32].into();
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn hash_good() {
        assert_eq!(calc_hash(&[]), "5c1419ac91dac01faef6f3e4c59ac776");
        assert_eq!(calc_hash(&[1]), "4bc7656e710322bd6b2d92ca80aac42c");
        assert_eq!(calc_hash(b"test"), "dd1b11c50f4fedd2c29cff6a94760175");
    }
}
