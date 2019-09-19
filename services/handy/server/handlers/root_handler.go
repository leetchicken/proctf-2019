package handlers

import (
	"fmt"
	"html/template"
	"log"
	"net/http"

	"handy/server/data"
	"handy/server/util"
)

type rootHandlerTemplateData struct {
	LocalUserInfo *data.LocalUserInfo
}

type rootHandler struct {
	t *template.Template
}

func NewRootHandler(t *template.Template) *rootHandler {
	return &rootHandler{
		t: t,
	}
}

func (h *rootHandler) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	if r.Method == http.MethodGet {
		h.handleGet(w, r)
	} else {
		HandleError(w, fmt.Errorf("invalid verb for profile picture handler: %s", r.Method), http.StatusBadRequest)
	}
}

func (h *rootHandler) handleGet(w http.ResponseWriter, r *http.Request) {
	_, lui, err := util.RetrieveUserInfo(r)
	if err == util.UserNotPresentError {
		lui = nil
	} else if err != nil {
		HandleError(w, fmt.Errorf("failed to extract user: %s", err), http.StatusBadRequest)
		return
	}
	err = h.t.ExecuteTemplate(w, "root", &rootHandlerTemplateData{
		LocalUserInfo: lui,
	})
	if err != nil {
		log.Printf("Template error: %s", err)
	}
}
