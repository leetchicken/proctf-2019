package backends

import (
	"encoding/hex"
	"crypto/aes"
	"crypto/cipher"
	"encoding/base64"
	"io"
	"crypto/rand"

	"handy/server/data"
	"handy/server/util"
)

type CookieStorage struct {
	algo cipher.Block
}

func NewCookieStorage() (*CookieStorage, error) {
	// TODO: replace with a generator.
	key, _ := hex.DecodeString("7627e63a76759af22442a606a61697e9")
	algo, err := aes.NewCipher(key)
	if err != nil {
		return nil, err
	}
	return &CookieStorage{
		algo: algo,
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

	nonce := make([]byte, 16)
	if _, err := io.ReadFull(rand.Reader, nonce); err != nil {
		return "", err
	}

	c := cipher.NewCTR(s.algo, nonce)
	ct := make([]byte, len(pt))
	c.XORKeyStream(ct, pt)
	ct = append(nonce, ct...)
	ctEnc := base64.StdEncoding.EncodeToString(ct)
	return ctEnc, nil
}

func (s *CookieStorage) UnpackCookie(cookie string) (*data.CookieInfo, error) {
	ct, err := base64.StdEncoding.DecodeString(cookie)
	if err != nil {
		return nil, err
	}
	nonce := ct[:16]
	ct = ct[16:]

	c := cipher.NewCTR(s.algo, nonce)
	pt := make([]byte, len(ct))
	c.XORKeyStream(pt, ct)
	pt, err = util.Pkcs7Unpad([]byte(pt), 16)
	if err != nil {
		return nil, err
	}

	return data.CookieInfoFromBytes(pt), nil
}
