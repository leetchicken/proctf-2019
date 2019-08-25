package handlers

import (
	"encoding/json"
	"errors"
	"fmt"
	"net/http"

	"github.com/gorilla/schema"

	"handy/server/backends"
	"handy/server/util"
)

type taskHandler struct {
	ts *backends.TaskStorage
}

func NewTaskHandler(ts *backends.TaskStorage) *taskHandler {
	return &taskHandler{
		ts: ts,
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

	result := map[string][]*backends.TaskInfo{}
	tasksFrom, err := h.ts.RetrieveTasksFromUser(id)
	if err != nil {
		HandleError(w, fmt.Errorf("failed to retrieve tasks from user: %s", err), http.StatusInternalServerError)
		return
	}
	result["tasks_from"] = tasksFrom
	if lui.IsMaster {
		tasksTo, err := h.ts.RetrieveTasksToUser(id)
		if err != nil {
			HandleError(w, fmt.Errorf("failed to retrieve tasks to user: %s", err), http.StatusInternalServerError)
			return
		}
		result["tasks_to"] = tasksTo
	}

	resultJSON, err := json.Marshal(result)
	if err != nil {
		HandleError(w, fmt.Errorf("failed to marshal response: %s", err), http.StatusInternalServerError)
		return
	}
	w.Header().Set("Content-Type", "application/json")
	w.Write(resultJSON)
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
	ti := &backends.TaskInfo{}
	if err := decoder.Decode(ti, r.PostForm); err != nil {
		HandleError(w, fmt.Errorf("failed to parse form: %s", err), http.StatusBadRequest)
		return
	}

	ti.RequesterID = id
	if err = h.ts.CreateTask(ti); err != nil {
		HandleError(w, fmt.Errorf("failed to create task: %s", err), http.StatusBadRequest)
		return
	}
}
