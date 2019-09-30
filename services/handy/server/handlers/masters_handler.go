package handlers

import (
	"errors"
	"fmt"
	"html/template"
	"log"
	"net/http"

	"handy/server/backends"
	"handy/server/data"
	"handy/server/util"
)

type masterHandlerTemplateData struct {
	LocalUserInfo *data.LocalUserInfo
	Masters       []*data.ProfileUserInfo
}

type masterHandler struct {
	us *backends.UserStorage
	t  *template.Template
}

func NewMasterHandler(us *backends.UserStorage, t *template.Template) *masterHandler {
	return &masterHandler{
		us: us,
		t:  t,
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
	_, lui, err := util.RetrieveUserInfo(r)
	if err == util.UserNotPresentError {
		HandleError(w, errors.New("user not authenticated"), http.StatusUnauthorized)
		return
	} else if err != nil {
		HandleError(w, fmt.Errorf("failed to extract user: %s", err), http.StatusBadRequest)
		return
	}

	td := &masterHandlerTemplateData{
		LocalUserInfo: lui,
	}

	td.Masters, err = h.us.GetMasters()
	if err != nil {
		HandleError(w, fmt.Errorf("failed to retrieve masters: %s", err), http.StatusInternalServerError)
		return
	}

	err = h.t.ExecuteTemplate(w, "masters", td)
	if err != nil {
		log.Printf("Template error: %s", err)
	}
}
