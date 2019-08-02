package handlers

import (
	"errors"
	"context"
	"net/http"

	"handy/server/backends"
)

func NewRootHandler(us *backends.UserStorage) InternalHandler {
	return func (context.Context, http.ResponseWriter, *http.Request) {
		panic(errors.New("not implemented"))
	}
}