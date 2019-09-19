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

type taskHandlerTemplateData struct {
	LocalUserInfo *data.LocalUserInfo
	TasksFrom     []*data.TaskInfo
	TasksTo       []*data.TaskInfo
}

type taskHandler struct {
	ts *backends.TaskStorage
	t  *template.Template
}

func NewTaskHandler(ts *backends.TaskStorage, t *template.Template) *taskHandler {
	return &taskHandler{
		ts: ts,
		t:  t,
	}
}

func (h *taskHandler) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	if r.Method == http.MethodGet {
		h.handleGet(w, r)
	} else if r.Method == http.MethodPost {
		h.handlePost(w, r)
	} else {
		HandleError(w, fmt.Errorf("invalid verb for task handler: %s", r.Method), http.StatusBadRequest)
	}
}

func (h *taskHandler) handleGet(w http.ResponseWriter, r *http.Request) {
	id, lui, err := util.RetrieveUserInfo(r)
	if err == util.UserNotPresentError {
		HandleError(w, errors.New("user not authenticated"), http.StatusUnauthorized)
		return
	} else if err != nil {
		HandleError(w, fmt.Errorf("failed to extract user: %s", err), http.StatusBadRequest)
		return
	}

	td := &taskHandlerTemplateData{
		LocalUserInfo: lui,
		TasksFrom:     []*data.TaskInfo{},
		TasksTo:       []*data.TaskInfo{},
	}

	td.TasksFrom, err = h.ts.RetrieveTasksFromUser(id)
	if err != nil {
		HandleError(w, fmt.Errorf("failed to retrieve tasks from user: %s", err), http.StatusInternalServerError)
		return
	}

	if lui.IsMaster {
		td.TasksTo, err = h.ts.RetrieveTasksToUser(id)
		if err != nil {
			HandleError(w, fmt.Errorf("failed to retrieve tasks to user: %s", err), http.StatusInternalServerError)
			return
		}
	}

	err = h.t.ExecuteTemplate(w, "tasks", td)
	if err != nil {
		log.Printf("Template error: %s", err)
	}
}

func (h *taskHandler) handlePost(w http.ResponseWriter, r *http.Request) {
	if err := r.ParseForm(); err != nil {
		HandleError(w, err, http.StatusBadRequest)
		return
	}

	id, _, err := util.RetrieveUserInfo(r)
	if err == util.UserNotPresentError {
		HandleError(w, errors.New("user not authenticated"), http.StatusUnauthorized)
		return
	} else if err != nil {
		HandleError(w, fmt.Errorf("failed to extract user: %s", err), http.StatusBadRequest)
		return
	}

	decoder := schema.NewDecoder()
	ti := &data.TaskInfo{}
	if err := decoder.Decode(ti, r.PostForm); err != nil {
		HandleError(w, fmt.Errorf("failed to parse form: %s", err), http.StatusBadRequest)
		return
	}

	ti.RequesterId = id
	if err = h.ts.CreateTask(ti); err != nil {
		HandleError(w, fmt.Errorf("failed to create task: %s", err), http.StatusBadRequest)
		return
	}
}
