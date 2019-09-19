package data

import (
	"encoding/binary"
	"fmt"
	"hash/crc32"

	"github.com/golang/protobuf/proto"
)

type CookieInfo struct {
	serializedInfo []byte
	deserialized   bool

	id string
	ui *LocalUserInfo
}

func CookieInfoFromUserInfo(ui *UserInfo) *CookieInfo {
	return &CookieInfo{
		id: ui.Id,
		ui: &LocalUserInfo{
			Username: ui.Username,
			IsMaster: ui.IsMaster,
		},
		deserialized: true,
	}
}

func CookieInfoFromBytes(id string, bytes []byte) *CookieInfo {
	return &CookieInfo{
		id:             id,
		serializedInfo: bytes,
		deserialized:   false,
	}
}

func (ci *CookieInfo) Bytes() ([]byte, error) {
	if ci.serializedInfo == nil {
		if err := ci.serialize(); err != nil {
			return nil, err
		}
	}
	return ci.serializedInfo, nil
}

func (ci *CookieInfo) GetID() string {
	return ci.id
}

func (ci *CookieInfo) GetLocalUserInfo() (*LocalUserInfo, error) {
	if !ci.deserialized {
		if err := ci.deserialize(); err != nil {
			return nil, err
		}
	}
	return ci.ui, nil
}

func (ci *CookieInfo) serialize() error {
	serializedUI, err := proto.Marshal(ci.ui)
	if err != nil {
		return err
	}
	ci.serializedInfo = make([]byte, 4)
	binary.BigEndian.PutUint32(ci.serializedInfo, crc32.ChecksumIEEE(serializedUI))
	ci.serializedInfo = append(ci.serializedInfo, serializedUI...)
	return nil
}

func (ci *CookieInfo) deserialize() error {
	if len(ci.serializedInfo) < 4 {
		return fmt.Errorf("serialize info has %d bytes, while minimum value is 4", len(ci.serializedInfo))
	}
	expectedHash := binary.BigEndian.Uint32(ci.serializedInfo[:4])
	hash := crc32.ChecksumIEEE(ci.serializedInfo[4:])
	if hash != expectedHash {
		return fmt.Errorf("cookie malformed, expected hash %d, found %d", expectedHash, hash)
	}

	ui := &LocalUserInfo{}
	err := proto.Unmarshal(ci.serializedInfo[4:], ui)
	if err != nil {
		return fmt.Errorf("failed to deserialize LocalUserInfo: %s", err)
	}
	ci.ui = ui
	ci.deserialized = true
	return nil
}
