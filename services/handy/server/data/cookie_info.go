package data

import (
	"fmt"

	"github.com/google/uuid"
	"github.com/golang/protobuf/proto"
)

// TODO: add hashing

type CookieInfo struct {
	serializedInfo []byte
	deserialized bool

	id uuid.UUID
	ui *UserInfo
}

func CookieInfoFromBytes(bytes []byte) *CookieInfo {
	return &CookieInfo {
		serializedInfo: bytes,
		deserialized: false,
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

func (ci *CookieInfo) GetID() (uuid.UUID, error) {
	if !ci.deserialized {
		if err := ci.deserialize(); err != nil {
			return uuid.Nil, err
		}
	}
	return ci.id, nil
}

func (ci *CookieInfo) serialize() error {
	serializedUUID, err := ci.id.MarshalBinary()
	if err != nil {
		return err
	}
	serializedUi, err := proto.Marshal(ci.ui)
	if err != nil {
		return err
	}
	ci.serializedInfo = []byte{}
	ci.serializedInfo = append(ci.serializedInfo, serializedUUID...)
	ci.serializedInfo = append(ci.serializedInfo, serializedUi...)
	return nil
}

func (ci *CookieInfo) deserialize() error {
	if len(ci.serializedInfo) < 16 {
		return fmt.Errorf("serialized CookieInfo has %d bytes, which is less than required 16", len(ci.serializedInfo))
	}
	serializedUUID := ci.serializedInfo[:16]
	id, err := uuid.ParseBytes(serializedUUID)
	if err != nil {
		return fmt.Errorf("failed to deserialize ID: %s", err)
	}
	ci.id = id

	ui := &UserInfo{}
	err = proto.Unmarshal(ci.serializedInfo[16:], ui)
	if err != nil {
		return fmt.Errorf("failed to deserialize UserInfo: %s", err)
	}
	ci.ui = ui
	return nil
}