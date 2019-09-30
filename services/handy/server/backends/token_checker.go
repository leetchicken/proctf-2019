package backends

import (
	"crypto"
	"crypto/rand"
	"crypto/rsa"
	"crypto/sha256"
	"crypto/x509"
	"encoding/base64"
	"encoding/pem"
	"flag"
	"fmt"
	"io/ioutil"
	"log"
	"sync"
	"time"
)

const (
	tokenSize = 32

	tokenExpirationDelay = 1 * time.Minute
)

var (
	tokenCheckerPublicKeyPath = flag.String(
		"token-checker-public-key-path",
		"/etc/handy/token_checker_public_key",
		"Path to the public key used for token validation.")
)

type TokenChecker struct {
	pub    *rsa.PublicKey
	tokens map[string]time.Time
	mx     *sync.Mutex
}

func NewTokenChecker() (*TokenChecker, error) {
	pub, err := ioutil.ReadFile(*tokenCheckerPublicKeyPath)
	if err != nil {
		return nil, fmt.Errorf("failed to read public key from %s: %s", *tokenCheckerPublicKeyPath, err)
	}
	pubPem, _ := pem.Decode(pub)
	if pubPem == nil {
		return nil, fmt.Errorf("failed to decode public key")
	}

	parsedKey, err := x509.ParsePKIXPublicKey(pubPem.Bytes)
	if err != nil {
		return nil, fmt.Errorf("failed to parse public key: %s", err)
	}

	pubKey, ok := parsedKey.(*rsa.PublicKey)
	if !ok {
		return nil, fmt.Errorf("public key is not in RSA format")
	}

	tc := &TokenChecker{
		pub:    pubKey,
		tokens: make(map[string]time.Time),
		mx:     &sync.Mutex{},
	}

	go func() {
		for true {
			func() {
				tc.mx.Lock()
				defer tc.mx.Unlock()

				toRemove := []string{}
				for k, v := range tc.tokens {
					if time.Now().Sub(v) > tokenExpirationDelay {
						toRemove = append(toRemove, k)
					}
				}
				for _, k := range toRemove {
					delete(tc.tokens, k)
				}
				if len(toRemove) > 0 {
					log.Printf("Removed %d tokens from cache", len(toRemove))
				}
			}()

			time.Sleep(tokenExpirationDelay)
		}
	}()

	return tc, nil
}

func (tc *TokenChecker) IssueToken() string {
	data := make([]byte, tokenSize)
	if _, err := rand.Read(data); err != nil {
		panic(fmt.Errorf("failed to generate token: %s", err))
	}
	token := base64.StdEncoding.EncodeToString(data)

	tc.mx.Lock()
	defer tc.mx.Unlock()
	tc.tokens[token] = time.Now()

	return token
}

func (tc *TokenChecker) CheckToken(token, signature string) error {
	tc.mx.Lock()
	_, ok := tc.tokens[token]
	tc.mx.Unlock()
	if !ok {
		return fmt.Errorf("token %s is expired or was never issued", token)
	}

	data, err := base64.StdEncoding.DecodeString(token)
	if err != nil {
		return fmt.Errorf("failed to decode token: %s", err)
	}
	sig, err := base64.StdEncoding.DecodeString(signature)
	if err != nil {
		return fmt.Errorf("failed to decode signature: %s", err)
	}

	h := sha256.New()
	h.Write(data)
	hashed := h.Sum(nil)

	return rsa.VerifyPSS(tc.pub, crypto.SHA256, hashed, sig, &rsa.PSSOptions{})
}
