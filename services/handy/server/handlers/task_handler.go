package handlers

import (
	"errors"
	"context"
	"net/http"

	"handy/server/backends"
)

func NewTaskHandler(ts *backends.TaskStorage) InternalHandler {
	return func (context.Context, http.ResponseWriter, *http.Request) {
		panic(errors.New("not implemented"))
	}
}