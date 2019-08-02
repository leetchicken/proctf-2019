package util

import (
	"bytes"
	"errors"
)

func Pkcs7Pad(b []byte, blockSize int) ([]byte, error) {
	if blockSize <= 0 {
		return nil, errors.New("invalid block size")
	}
	if b == nil || len(b) == 0 {
		return nil, errors.New("data is empty or nil")
	}
	n := blockSize - (len(b) % blockSize)
	pb := make([]byte, len(b)+n)
	copy(pb, b)
	copy(pb[len(b):], bytes.Repeat([]byte{byte(n)}, n))
	return pb, nil
}

func Pkcs7Unpad(b []byte, blockSize int) ([]byte, error) {
	if blockSize <= 0 {
		return nil, errors.New("invalid block size")
	}
	if b == nil || len(b) == 0 {
		return nil, errors.New("data is empty or nil")
	}
	if len(b)%blockSize != 0 {
		return nil, errors.New("invalid padding")
	}
	c := b[len(b)-1]
	n := int(c)
	if n == 0 || n > len(b) {
		return nil, errors.New("invalid padding")
	}
	for i := 0; i < n; i++ {
		if b[len(b)-n+i] != c {
			return nil, errors.New("invalid padding")
		}
	}
	return b[:len(b)-n], nil
}
