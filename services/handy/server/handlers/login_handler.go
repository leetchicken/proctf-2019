package handlers

import (
	"errors"
	"context"
	"net/http"

	"handy/server/backends"
)

func NewLoginHandler(cs *backends.CookieStorage, us *backends.UserStorage) InternalHandler {
	return func (context.Context, http.ResponseWriter, *http.Request) {
		panic(errors.New("not implemented"))
	}
}