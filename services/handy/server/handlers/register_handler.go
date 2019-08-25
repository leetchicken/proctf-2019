package handlers

import (
	"errors"
	"fmt"
	"log"
	"net/http"

	"github.com/gorilla/schema"

	"handy/server/backends"
)

type registerHandler struct {
	us *backends.UserStorage
}

func NewRegisterHandler(us *backends.UserStorage) *registerHandler {
	return &registerHandler{
		us: us,
	}
}

func (h *registerHandler) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	if r.Method == http.MethodGet {
		h.handleGet(w, r)
	} else if r.Method == http.MethodPost {
		h.handlePost(w, r)
	} else {
		HandleError(w, fmt.Errorf("invalid verb for register handler: %s", r.Method), http.StatusBadRequest)
	}
}

func (h *registerHandler) handleGet(w http.ResponseWriter, r *http.Request) {
	HandleError(w, errors.New("not implemented"), http.StatusNotImplemented)
}

func (h *registerHandler) handlePost(w http.ResponseWriter, r *http.Request) {
	if err := r.ParseForm(); err != nil {
		HandleError(w, err, http.StatusBadRequest)
		return
	}

	decoder := schema.NewDecoder()
	ri := &backends.UserStorageRegistrationInfo{}
	if err := decoder.Decode(ri, r.PostForm); err != nil {
		HandleError(w, fmt.Errorf("failed to parse form: %s", err), http.StatusBadRequest)
		return
	}
	if len(ri.Username) == 0 || len(ri.Password) == 0 {
		HandleError(w, fmt.Errorf("invalid form, username/password cannot be empty"), http.StatusBadRequest)
		return
	}

	// TODO: add PoW

	_, err := h.us.Register(ri)
	if err == backends.UserStorageUserExistsError {
		HandleError(w, fmt.Errorf("user '%s' already exists", ri.Username), http.StatusConflict)
		return
	} else if err != nil {
		HandleError(w, fmt.Errorf("failed to register: %s", err), http.StatusBadRequest)
		return
	}
	log.Printf("Successfully registered a user '%s'", ri.Username)

	// TODO: redirect to login page
}
