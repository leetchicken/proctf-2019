package backends

import (
	"crypto/aes"
	"crypto/cipher"
	"crypto/sha256"
	"encoding/base64"
	"flag"
	"fmt"

	"github.com/google/uuid"

	"handy/server/data"
	"handy/server/util"
)

const (
	cookieStorageKeySize  = 16
	cookieStorageSaltSize = 32
)

var (
	cookieStorageEncryptionKeyPath = flag.String(
		"cookie-storage-encryption-key-path",
		"/etc/handy/cookie_storage_encryption_key",
		"Path to the key used for encryption in CookieStorage")
	cookieStorageHashingSaltPath = flag.String(
		"cookie-storage-hashing-salt-path",
		"/etc/handy/cookie_storage_hashing_salt",
		"Path to the salt used for hashing in CookieStorage")
)

type CookieStorage struct {
	algo cipher.Block
	salt []byte
}

func NewCookieStorage() (*CookieStorage, error) {
	key, err := util.LoadKeyOrGenerate(*cookieStorageEncryptionKeyPath, cookieStorageKeySize)
	if err != nil {
		return nil, err
	}
	algo, err := aes.NewCipher(key)
	if err != nil {
		return nil, err
	}
	salt, err := util.LoadKeyOrGenerate(*cookieStorageHashingSaltPath, cookieStorageSaltSize)
	if err != nil {
		return nil, err
	}
	return &CookieStorage{
		algo: algo,
		salt: salt,
	}, nil
}

func (s *CookieStorage) CreateCookie(ci *data.CookieInfo) (string, error) {
	pt, err := ci.Bytes()
	if err != nil {
		return "", err
	}
	pt, err = util.Pkcs7Pad(pt, 16)
	if err != nil {
		return "", err
	}

	id, err := uuid.Parse(ci.GetID())
	if err != nil {
		return "", err
	}
	idBytes, err := id.MarshalBinary()
	if err != nil {
		return "", err
	}
	hash := sha256.Sum256(idBytes)
	nonce := hash[:cookieStorageKeySize]

	c := cipher.NewCTR(s.algo, nonce)
	ct := make([]byte, len(pt))
	c.XORKeyStream(ct, pt)
	ct = append(idBytes, ct...)
	ctEnc := base64.StdEncoding.EncodeToString(ct)
	return ctEnc, nil
}

func (s *CookieStorage) UnpackCookie(cookie string) (*data.CookieInfo, error) {
	ct, err := base64.StdEncoding.DecodeString(cookie)
	if err != nil {
		return nil, err
	}
	if len(ct) < 16 {
		return nil, fmt.Errorf("cookie only has %d bytes, should be at least 16 bytes", len(ct))
	}
	id, err := uuid.FromBytes(ct[:16])
	if err != nil {
		return nil, err
	}
	hash := sha256.Sum256(ct[:16])
	nonce := hash[:cookieStorageKeySize]
	ct = ct[16:]

	c := cipher.NewCTR(s.algo, nonce)
	pt := make([]byte, len(ct))
	c.XORKeyStream(pt, ct)
	pt, err = util.Pkcs7Unpad([]byte(pt), 16)
	if err != nil {
		return nil, err
	}

	idStr, err := id.MarshalText()
	if err != nil {
		return nil, err
	}
	return data.CookieInfoFromBytes(string(idStr), pt), nil
}
