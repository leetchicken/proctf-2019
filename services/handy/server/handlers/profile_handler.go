package handlers

import (
	"errors"
	"fmt"
	"html/template"
	"log"
	"net/http"

	"github.com/gorilla/schema"

	"handy/server/backends"
	"handy/server/data"
	"handy/server/util"
)

type profileHandlerTemplateData struct {
	LocalUserInfo *data.LocalUserInfo
	User          *data.ProfileUserInfo
}

type profileForm struct {
	ID string
}

type profileHandler struct {
	us *backends.UserStorage
	t  *template.Template
}

func NewProfileHandler(us *backends.UserStorage, t *template.Template) *profileHandler {
	return &profileHandler{
		us: us,
		t:  t,
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
	_, lui, err := util.RetrieveUserInfo(r)
	if err == util.UserNotPresentError {
		HandleError(w, errors.New("user not authenticated"), http.StatusUnauthorized)
		return
	} else if err != nil {
		HandleError(w, fmt.Errorf("failed to extract user: %s", err), http.StatusBadRequest)
		return
	}

	td := &profileHandlerTemplateData{
		LocalUserInfo: lui,
	}

	if err := r.ParseForm(); err != nil {
		HandleError(w, err, http.StatusBadRequest)
		return
	}

	decoder := schema.NewDecoder()
	pf := &profileForm{}
	if err := decoder.Decode(pf, r.Form); err != nil {
		HandleError(w, fmt.Errorf("failed to parse form: %s", err), http.StatusBadRequest)
		return
	}

	td.User, err = h.us.GetProfileInfo(pf.ID)
	if err != nil {
		HandleError(w, fmt.Errorf("failed to get profile info: %s", err), http.StatusBadRequest)
		return
	}

	err = h.t.ExecuteTemplate(w, "profile", td)
	if err != nil {
		log.Printf("Template error: %s", err)
	}
}
