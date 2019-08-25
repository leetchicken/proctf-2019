package handlers

import (
	"errors"
	"fmt"
	"net/http"

	"github.com/gorilla/schema"

	"handy/server/backends"
	"handy/server/constants"
	"handy/server/data"
)

type loginHandler struct {
	cs *backends.CookieStorage
	us *backends.UserStorage
}

func NewLoginHandler(cs *backends.CookieStorage, us *backends.UserStorage) *loginHandler {
	return &loginHandler{
		cs: cs,
		us: us,
	}
}

func (h *loginHandler) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	if r.Method == http.MethodGet {
		h.handleGet(w, r)
	} else if r.Method == http.MethodPost {
		h.handlePost(w, r)
	} else {
		HandleError(w, fmt.Errorf("invalid verb for login handler: %s", r.Method), http.StatusBadRequest)
	}
}

func (h *loginHandler) handleGet(w http.ResponseWriter, r *http.Request) {
	HandleError(w, errors.New("not implemented"), http.StatusNotImplemented)
}

func (h *loginHandler) handlePost(w http.ResponseWriter, r *http.Request) {
	if err := r.ParseForm(); err != nil {
		HandleError(w, err, http.StatusBadRequest)
		return
	}

	decoder := schema.NewDecoder()
	li := &backends.UserStorageLoginInfo{}
	if err := decoder.Decode(li, r.PostForm); err != nil {
		HandleError(w, fmt.Errorf("failed to parse form: %s", err), http.StatusBadRequest)
		return
	}

	ui, err := h.us.Login(li)
	if err != nil {
		HandleError(w, fmt.Errorf("failed to login: %s", err), http.StatusBadRequest)
		return
	}

	cookie, err := h.cs.CreateCookie(data.CookieInfoFromUserInfo(ui))
	if err != nil {
		HandleError(w, fmt.Errorf("failed to create cookie: %s", err), http.StatusInternalServerError)
		return
	}
	http.SetCookie(w, &http.Cookie{
		Name:  constants.SessionCookieName,
		Value: cookie,
	})

	// TODO: redirect
}
