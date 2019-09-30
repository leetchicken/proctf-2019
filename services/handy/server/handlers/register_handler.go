package handlers

import (
	"fmt"
	"html/template"
	"log"
	"net/http"

	"github.com/gorilla/schema"

	"handy/server/backends"
	"handy/server/util"
)

type registerHandlerTemplateData struct {
	Token string
}

type registerHandlerForm struct {
	Username    string
	Password    string
	IsMaster    bool
	Token       string
	SignedToken string
}

type registerHandler struct {
	us *backends.UserStorage
	tc *backends.TokenChecker
	t  *template.Template
}

func NewRegisterHandler(us *backends.UserStorage, tc *backends.TokenChecker, t *template.Template) *registerHandler {
	return &registerHandler{
		us: us,
		tc: tc,
		t:  t,
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
	_, _, err := util.RetrieveUserInfo(r)
	if err == util.UserNotPresentError {
		err = h.t.ExecuteTemplate(w, "register", &registerHandlerTemplateData{
			Token: h.tc.IssueToken(),
		})
		if err != nil {
			log.Printf("Template error: %s", err)
		}
		return
	} else if err != nil {
		HandleError(w, fmt.Errorf("failed to extract user: %s", err), http.StatusBadRequest)
		return
	}
	http.Redirect(w, r, "/", http.StatusSeeOther)
}

func (h *registerHandler) handlePost(w http.ResponseWriter, r *http.Request) {
	if err := r.ParseForm(); err != nil {
		HandleError(w, err, http.StatusBadRequest)
		return
	}

	decoder := schema.NewDecoder()
	form := &registerHandlerForm{}
	if err := decoder.Decode(form, r.PostForm); err != nil {
		HandleError(w, fmt.Errorf("failed to parse form: %s", err), http.StatusBadRequest)
		return
	}

	if form.IsMaster {
		if err := h.tc.CheckToken(form.Token, form.SignedToken); err != nil {
			HandleError(w, fmt.Errorf("failed to check token: %s", err), http.StatusBadRequest)
			return
		}
	}

	_, err := h.us.Register(&backends.UserStorageRegistrationInfo{
		Username: form.Username,
		Password: form.Password,
		IsMaster: form.IsMaster,
	})
	if err == backends.UserStorageUserExistsError {
		HandleError(w, fmt.Errorf("user '%s' already exists", form.Username), http.StatusConflict)
		return
	} else if err != nil {
		HandleError(w, fmt.Errorf("failed to register: %s", err), http.StatusBadRequest)
		return
	}
	log.Printf("Successfully registered a user '%s'", form.Username)
}
