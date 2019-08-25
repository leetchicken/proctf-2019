package util

import (
	"crypto/rand"
	"fmt"
	"io/ioutil"
	"os"
	"path/filepath"
)

func LoadKeyOrGenerate(filename string, keySize int) ([]byte, error) {
	data, err := ioutil.ReadFile(filename)
	if os.IsNotExist(err) {
		key := make([]byte, keySize)
		_, err := rand.Read(key)
		if err != nil {
			return nil, err
		}
		dir, _ := filepath.Split(filename)
		if err = os.MkdirAll(dir, os.ModePerm); err != nil {
			return nil, fmt.Errorf("failed to create directory: %s", err)
		}
		if err = ioutil.WriteFile(filename, key, 0600); err != nil {
			return nil, fmt.Errorf("failed to write key: %s", err)
		}
		return key, nil
	}
	if len(data) != keySize {
		return nil, fmt.Errorf("Loaded key of size %d from '%s', expect key of size %d", len(data), filename, keySize)
	}
	return data, nil
}
