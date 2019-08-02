package backends

import (
	"fmt"
	"io"
	"encoding/base64"

	"github.com/google/uuid"
	"github.com/nullrocks/identicon"
)

const (
	maxSize = 1024
)

type AvatarGenerator struct {
	ig *identicon.Generator
}

func NewAvatarGenerator() (*AvatarGenerator, error) {
	ig, err := identicon.New(
		"handy",
		7,
		3,
	)
	if err != nil {
		return nil, err
	}

	return &AvatarGenerator{
		ig: ig,
	}, nil
}

func (g *AvatarGenerator) GenerateAvatar(id uuid.UUID, size int, w io.Writer) error {
	if size < 1 || size > maxSize {
		return fmt.Errorf("Invalid size: %d should be greater than 0 and no more than %d", size, maxSize)
	}
	idBytes, err := id.MarshalBinary()
	if err != nil {
		return err
	}
	encodedID := base64.StdEncoding.EncodeToString(idBytes)
	ii, err := g.ig.Draw(encodedID)
	if err != nil {
		return err
	}
	ii.Png(size, w)
	return nil
}
