package backends

import (
	"fmt"
	"io"

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

type GenerateAvatarInfo struct {
	ID   string
	Size int
}

func (g *AvatarGenerator) GenerateAvatar(gai *GenerateAvatarInfo, w io.Writer) error {
	if gai.Size < 1 || gai.Size > maxSize {
		return fmt.Errorf("Invalid size: %d should be greater than 0 and no more than %d", gai.Size, maxSize)
	}
	ii, err := g.ig.Draw(gai.ID)
	if err != nil {
		return err
	}
	ii.Png(gai.Size, w)
	return nil
}
