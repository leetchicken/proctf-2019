package backends

import (
	"errors"

	"go.mongodb.org/mongo-driver/mongo"
	"github.com/google/uuid"
)

type UserStorage struct {
	users *mongo.Collection
}

type UserInfo struct {
}

type ProfileInfo struct {
}

func NewUserStorage(db *mongo.Database) (*UserStorage, error) {
	return &UserStorage {
		users: db.Collection("users"),
	}, nil
}

func (s *UserStorage) Register(ui *UserInfo) error {
	return errors.New("not implemented")
}

func (s *UserStorage) Login(username string, password string) (*UserInfo, error) {
	return nil, errors.New("not implemented")
}

func (s *UserStorage) GetProfileInfo(id uuid.UUID) (*ProfileInfo, error) {
	return nil, errors.New("not implemented")
}
