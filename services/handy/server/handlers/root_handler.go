package handlers

import (
	"errors"
	"fmt"
	"net/http"
)

type rootHandler struct {
}

func (h *rootHandler) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	if r.Method == http.MethodGet {
		h.handleGet(w, r)
	} else {
		HandleError(w, fmt.Errorf("invalid verb for profile picture handler: %s", r.Method), http.StatusBadRequest)
	}
}

func (h *rootHandler) handleGet(w http.ResponseWriter, r *http.Request) {
	HandleError(w, errors.New("not implemented"), http.StatusNotImplemented)
}

func NewRootHandler() *rootHandler {
	return &rootHandler{}
}
