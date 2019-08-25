package handlers

import (
	"encoding/json"
	"errors"
	"fmt"
	"net/http"

	"handy/server/backends"
	"handy/server/util"
)

type masterHandler struct {
	us *backends.UserStorage
}

func NewMasterHandler(us *backends.UserStorage) *masterHandler {
	return &masterHandler{
		us: us,
	}
}

func (h *masterHandler) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	if r.Method == http.MethodGet {
		h.handleGet(w, r)
	} else {
		HandleError(w, fmt.Errorf("invalid verb for profile handler: %s", r.Method), http.StatusBadRequest)
	}
}

func (h *masterHandler) handleGet(w http.ResponseWriter, r *http.Request) {
	_, _, err := util.RetrieveUserInfo(r)
	if err == util.UserNotPresentError {
		HandleError(w, errors.New("user not authenticated"), http.StatusUnauthorized)
		return
	} else if err != nil {
		HandleError(w, fmt.Errorf("failed to extract user: %s", err), http.StatusBadRequest)
		return
	}

	masters, err := h.us.GetMasters()
	if err != nil {
		HandleError(w, fmt.Errorf("failed to retrieve masters: %s", err), http.StatusInternalServerError)
		return
	}

	// TODO: replace with html
	resultJSON, err := json.Marshal(masters)
	if err != nil {
		HandleError(w, fmt.Errorf("failed to marshal response: %s", err), http.StatusInternalServerError)
		return
	}
	w.Header().Set("Content-Type", "application/json")
	w.Write(resultJSON)
}
