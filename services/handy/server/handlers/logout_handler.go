package handlers

import (
	"fmt"
	"net/http"

	"handy/server/constants"
)

type logoutHandler struct {
}

func NewLogoutHandler() *logoutHandler {
	return &logoutHandler{}
}

func (h *logoutHandler) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	if r.Method == http.MethodGet {
		h.handleGet(w, r)
	} else {
		HandleError(w, fmt.Errorf("invalid verb for logout handler: %s", r.Method), http.StatusBadRequest)
	}
}

func (h *logoutHandler) handleGet(w http.ResponseWriter, r *http.Request) {
	http.SetCookie(w, &http.Cookie{
		Name:   constants.SessionCookieName,
		Value:  "",
		MaxAge: 0,
	})
	http.Redirect(w, r, "/", http.StatusSeeOther)
}
