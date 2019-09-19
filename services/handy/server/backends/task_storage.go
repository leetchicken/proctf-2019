package backends

import (
	"context"
	"fmt"

	"go.mongodb.org/mongo-driver/bson"
	"go.mongodb.org/mongo-driver/mongo"

	"handy/server/data"
	"handy/server/util"
)

const (
	maxDescriptionLength = 100 * 1024
	maxTitleLength       = 1024
	uuidLength           = 36
)

type TaskStorage struct {
	tasks *mongo.Collection
	users *mongo.Collection
}

func NewTaskStorage(db *mongo.Database) (*TaskStorage, error) {
	return &TaskStorage{
		tasks: db.Collection("tasks"),
		users: db.Collection("users"),
	}, nil
}

func (s *TaskStorage) CreateTask(ti *data.TaskInfo) error {
	if err := util.CheckFieldLength(ti.MasterId, uuidLength, uuidLength); err != nil {
		return fmt.Errorf("invalid MasterID field: %s", err)
	}
	if err := util.CheckFieldLength(ti.RequesterId, uuidLength, uuidLength); err != nil {
		return fmt.Errorf("invalid RequesterId field: %s", err)
	}
	if err := util.CheckFieldLength(ti.Title, 1, maxTitleLength); err != nil {
		return fmt.Errorf("invalid Title field: %s", err)
	}
	if err := util.CheckFieldLength(ti.Description, 1, maxDescriptionLength); err != nil {
		return fmt.Errorf("invalid Description field: %s", err)
	}
	if ti.MasterId == ti.RequesterId {
		return fmt.Errorf("user and master cannot be the same")
	}

	filter := &bson.D{{"isMaster", true}, {"id", ti.MasterId}}
	cnt, err := s.users.CountDocuments(context.Background(), filter)
	if err != nil {
		return fmt.Errorf("failed to search for a master: %s", err)
	}
	if cnt == 0 {
		return fmt.Errorf("master with id %s doesn't exist", ti.MasterId)
	}

	doc, err := protbufToBson(ti)
	if err != nil {
		return fmt.Errorf("failed to marshal user: %s", err)
	}
	if _, err := s.tasks.InsertOne(context.Background(), doc); err != nil {
		return fmt.Errorf("failed to create a task: %s", err)
	}
	return nil
}

func (s *TaskStorage) RetrieveTasksFromUser(id string) ([]*data.TaskInfo, error) {
	if err := util.CheckFieldLength(id, uuidLength, uuidLength); err != nil {
		return nil, fmt.Errorf("invalid ID field: %s", err)
	}

	filter := &bson.D{{"requesterId", id}}
	return s.retrieveTasks(filter)
}

func (s *TaskStorage) RetrieveTasksToUser(id string) ([]*data.TaskInfo, error) {
	if err := util.CheckFieldLength(id, uuidLength, uuidLength); err != nil {
		return nil, fmt.Errorf("invalid ID field: %s", err)
	}

	filter := &bson.D{{"masterId", id}}
	return s.retrieveTasks(filter)
}

func (s *TaskStorage) retrieveTasks(filter *bson.D) ([]*data.TaskInfo, error) {
	ctx := context.Background()
	cur, err := s.tasks.Find(ctx, filter)
	if err != nil {
		return nil, fmt.Errorf("failed to retrieve tasks: %s", err)
	}
	defer cur.Close(ctx)

	result := []*data.TaskInfo{}
	for cur.Next(ctx) {
		elem := &bson.D{}
		if err := cur.Decode(elem); err != nil {
			return nil, fmt.Errorf("failed to get over tasks: %s", err)
		}

		ti := &data.TaskInfo{}
		if err = bsonToProtobuf(elem, ti); err != nil {
			return nil, fmt.Errorf("failed to decode protobuf: %s", err)
		}
		result = append(result, ti)
	}
	if err := cur.Err(); err != nil {
		return nil, fmt.Errorf("tasks retrieving error: %s", err)
	}

	return result, nil
}
