package data

import (
	"fmt"

	"github.com/golang/protobuf/proto"
)

// TODO: add hashing

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
	ci.serializedInfo = serializedUI
	return nil
}

func (ci *CookieInfo) deserialize() error {
	ui := &LocalUserInfo{}
	err := proto.Unmarshal(ci.serializedInfo, ui)
	if err != nil {
		return fmt.Errorf("failed to deserialize LocalUserInfo: %s", err)
	}
	ci.ui = ui
	ci.deserialized = true
	return nil
}
