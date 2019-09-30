package backends

import (
	"context"
	"crypto/sha256"
	"encoding/base64"
	"errors"
	"flag"
	"fmt"

	"github.com/google/uuid"
	"go.mongodb.org/mongo-driver/bson"
	"go.mongodb.org/mongo-driver/mongo"

	"handy/server/data"
	"handy/server/util"
)

const (
	userStorageSaltSize = 32

	maxUsernameLength = 256
	maxPasswordLength = 256
)

var (
	UserStorageUserExistsError = errors.New("user already exists")
	UserStorageUserNotFound    = errors.New("user not found")

	userStorageHashingSaltPath = flag.String(
		"user-storage-hashing-salt-path",
		"/etc/handy/user_storage_hashing_salt",
		"Path to the salt used for hashing in UserStorage")
)

type UserStorage struct {
	users *mongo.Collection
	salt  []byte
}

type UserStorageRegistrationInfo struct {
	Username string
	Password string
	IsMaster bool
}

type UserStorageLoginInfo struct {
	Username string
	Password string
}

func NewUserStorage(db *mongo.Database) (*UserStorage, error) {
	users := db.Collection("users")
	err := ensureIndexExists(users, "username")
	if err != nil {
		return nil, fmt.Errorf("failed to ensure index exists: %s", err)
	}
	salt, err := util.LoadKeyOrGenerate(*userStorageHashingSaltPath, userStorageSaltSize)
	if err != nil {
		return nil, fmt.Errorf("failed to load hashing salt: %s", err)
	}

	return &UserStorage{
		users: users,
		salt:  salt,
	}, nil
}

func (s *UserStorage) Register(ri *UserStorageRegistrationInfo) (*data.UserInfo, error) {
	if err := util.CheckFieldLength(ri.Username, 1, maxUsernameLength); err != nil {
		return nil, fmt.Errorf("invalid Username field: %s", err)
	}
	if err := util.CheckFieldLength(ri.Password, 1, maxPasswordLength); err != nil {
		return nil, fmt.Errorf("invalid Password field: %s", err)
	}

	cnt, err := s.users.CountDocuments(context.Background(), bson.D{{"username", ri.Username}})
	if err != nil {
		return nil, fmt.Errorf("failed to search for a user: %s", err)
	}
	if cnt > 0 {
		return nil, UserStorageUserExistsError
	}

	id, _ := uuid.New().MarshalText()
	ui := &data.UserInfo{
		Id:           string(id),
		Username:     ri.Username,
		PasswordHash: s.hashPassword(ri.Password),
		IsMaster:     ri.IsMaster,
	}
	doc, err := protbufToBson(ui)
	if err != nil {
		return nil, fmt.Errorf("failed to marshal user: %s", err)
	}
	_, err = s.users.InsertOne(context.Background(), doc)
	if err != nil {
		return nil, fmt.Errorf("failed to create user: %s", err)
	}
	return ui, nil
}

func (s *UserStorage) Login(li *UserStorageLoginInfo) (*data.UserInfo, error) {
	if err := util.CheckFieldLength(li.Username, 1, maxUsernameLength); err != nil {
		return nil, fmt.Errorf("invalid Username field: %s", err)
	}
	if err := util.CheckFieldLength(li.Password, 1, maxPasswordLength); err != nil {
		return nil, fmt.Errorf("invalid Password field: %s", err)
	}

	filter := bson.D{{"username", li.Username}, {"passwordHash", s.hashPassword(li.Password)}}

	cnt, err := s.users.CountDocuments(context.Background(), filter)
	if err != nil {
		return nil, fmt.Errorf("failed to search for a user: %s", err)
	}
	if cnt == 0 {
		return nil, UserStorageUserNotFound
	}

	fr := s.users.FindOne(context.Background(), filter)
	if err = fr.Err(); err != nil {
		return nil, fmt.Errorf("failed to find a user: %s", err)
	}
	doc := &bson.D{}
	if err = fr.Decode(doc); err != nil {
		return nil, fmt.Errorf("failed to decode a find response: %s", err)
	}

	ui := &data.UserInfo{}
	if err = bsonToProtobuf(doc, ui); err != nil {
		return nil, fmt.Errorf("failed to decode protobuf: %s", err)
	}
	return ui, nil
}

func (s *UserStorage) GetProfileInfo(id string) (*data.ProfileUserInfo, error) {
	filter := bson.D{{"id", id}}
	cnt, err := s.users.CountDocuments(context.Background(), filter)
	if err != nil {
		return nil, fmt.Errorf("failed to search for a user: %s", err)
	}
	if cnt == 0 {
		return nil, UserStorageUserNotFound
	}

	fr := s.users.FindOne(context.Background(), filter)
	if err = fr.Err(); err != nil {
		return nil, fmt.Errorf("failed to find a user: %s", err)
	}
	doc := &bson.D{}
	if err = fr.Decode(doc); err != nil {
		return nil, fmt.Errorf("failed to decode a find response: %s", err)
	}

	ui := &data.ProfileUserInfo{}
	if err = bsonToProtobuf(doc, ui); err != nil {
		return nil, fmt.Errorf("failed to decode protobuf: %s", err)
	}
	return ui, nil
}

func (s *UserStorage) GetMasters() ([]*data.ProfileUserInfo, error) {
	ctx := context.Background()
	cur, err := s.users.Find(ctx, &bson.D{{"isMaster", true}})
	if err != nil {
		return nil, fmt.Errorf("failed to retrieve masters: %s", err)
	}
	defer cur.Close(ctx)

	result := []*data.ProfileUserInfo{}
	for cur.Next(ctx) {
		elem := &bson.D{}
		if err := cur.Decode(elem); err != nil {
			return nil, fmt.Errorf("failed to get over masters: %s", err)
		}

		pui := &data.ProfileUserInfo{}
		if err = bsonToProtobuf(elem, pui); err != nil {
			return nil, fmt.Errorf("failed to decode protobuf: %s", err)
		}
		result = append(result, pui)
	}
	if err := cur.Err(); err != nil {
		return nil, fmt.Errorf("masters retrieving error: %s", err)
	}

	return result, nil
}

func (s *UserStorage) hashPassword(password string) string {
	msg := []byte(password)
	msg = append(msg, s.salt...)
	hash := sha256.Sum256(msg)
	return base64.StdEncoding.EncodeToString(hash[:])
}
