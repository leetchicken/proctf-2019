package handlers

import (
	"errors"
	"fmt"
	"net/http"

	"handy/server/backends"
)

type profileHandler struct {
	us *backends.UserStorage
}

func NewProfileHandler(us *backends.UserStorage) *profileHandler {
	return &profileHandler{
		us: us,
	}
}

func (h *profileHandler) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	if r.Method == http.MethodGet {
		h.handleGet(w, r)
	} else {
		HandleError(w, fmt.Errorf("invalid verb for profile handler: %s", r.Method), http.StatusBadRequest)
	}
}

func (h *profileHandler) handleGet(w http.ResponseWriter, r *http.Request) {
	HandleError(w, errors.New("not implemented"), http.StatusNotImplemented)
}
