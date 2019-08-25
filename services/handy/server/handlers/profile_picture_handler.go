package handlers

import (
	"fmt"
	"net/http"

	"github.com/gorilla/schema"

	"handy/server/backends"
)

type profilePictureHandler struct {
	ag *backends.AvatarGenerator
}

func (h *profilePictureHandler) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	if r.Method == http.MethodGet {
		h.handleGet(w, r)
	} else {
		HandleError(w, fmt.Errorf("invalid verb for profile picture handler: %s", r.Method), http.StatusBadRequest)
	}
}

func (h *profilePictureHandler) handleGet(w http.ResponseWriter, r *http.Request) {
	if err := r.ParseForm(); err != nil {
		HandleError(w, err, http.StatusBadRequest)
		return
	}

	decoder := schema.NewDecoder()
	gai := &backends.GenerateAvatarInfo{}
	if err := decoder.Decode(gai, r.Form); err != nil {
		HandleError(w, fmt.Errorf("failed to parse form: %s", err), http.StatusBadRequest)
		return
	}

	if err := h.ag.GenerateAvatar(gai, w); err != nil {
		HandleError(w, err, http.StatusBadRequest)
	}
}

func NewProfilePictureHandler(ag *backends.AvatarGenerator) *profilePictureHandler {
	return &profilePictureHandler{
		ag: ag,
	}
}
