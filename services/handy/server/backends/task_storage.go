package backends

import (
	"errors"

	"go.mongodb.org/mongo-driver/mongo"
	"github.com/google/uuid"
)

type TaskStorage struct {
	tasks *mongo.Collection
}

type TaskInfo struct {

}

func NewTaskStorage(db *mongo.Database) (*TaskStorage, error) {
	return &TaskStorage {
		tasks: db.Collection("tasks"),
	}, nil
}

func (s *TaskStorage) CreateTask(ti *TaskInfo) error {
	return errors.New("not implemented")
}

func (s *TaskStorage) RetrieveTasksFromUser(id uuid.UUID) ([]*TaskInfo, error) {
	return nil, errors.New("not implemented")
}

func (s *TaskStorage) RetrieveTasksToUser(id uuid.UUID) ([]*TaskInfo, error) {
	return nil, errors.New("not implemented")
}
