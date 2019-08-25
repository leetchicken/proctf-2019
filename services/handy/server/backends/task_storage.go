package backends

import (
	"context"
	"fmt"

	"go.mongodb.org/mongo-driver/bson"
	"go.mongodb.org/mongo-driver/mongo"
)

type TaskStorage struct {
	tasks *mongo.Collection
	users *mongo.Collection
}

type TaskInfo struct {
	MasterID    string
	RequesterID string
	Description string
}

func NewTaskStorage(db *mongo.Database) (*TaskStorage, error) {
	return &TaskStorage{
		tasks: db.Collection("tasks"),
		users: db.Collection("users"),
	}, nil
}

func (s *TaskStorage) CreateTask(ti *TaskInfo) error {
	if len(ti.MasterID) == 0 || len(ti.RequesterID) == 0 || len(ti.Description) == 0 {
		return fmt.Errorf("task information contains empty fields: %v", ti)
	}

	filter := &bson.D{{"isMaster", true}, {"id", ti.MasterID}}
	cnt, err := s.users.CountDocuments(context.Background(), filter)
	if err != nil {
		return fmt.Errorf("failed to search for a master: %s", err)
	}
	if cnt == 0 {
		return fmt.Errorf("master with id %s doesn't exist", ti.MasterID)
	}

	if _, err := s.tasks.InsertOne(context.Background(), ti); err != nil {
		return fmt.Errorf("failed to create a task: %s", err)
	}
	return nil
}

func (s *TaskStorage) RetrieveTasksFromUser(id string) ([]*TaskInfo, error) {
	filter := &bson.D{{"requesterID", id}}
	return s.retrieveTasks(filter)
}

func (s *TaskStorage) RetrieveTasksToUser(id string) ([]*TaskInfo, error) {
	filter := &bson.D{{"masterID", id}}
	return s.retrieveTasks(filter)
}

func (s *TaskStorage) retrieveTasks(filter *bson.D) ([]*TaskInfo, error) {
	ctx := context.Background()
	cur, err := s.tasks.Find(ctx, filter)
	if err != nil {
		return nil, fmt.Errorf("failed to retrieve tasks: %s", err)
	}
	defer cur.Close(ctx)

	result := []*TaskInfo{}
	for cur.Next(ctx) {
		elem := &TaskInfo{}
		if err := cur.Decode(elem); err != nil {
			return nil, fmt.Errorf("failed to get over results: %s", err)
		}

		result = append(result, elem)
	}
	if err := cur.Err(); err != nil {
		return nil, fmt.Errorf("tasks retrieving error: %s", err)
	}

	return result, nil
}
