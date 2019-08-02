package handlers

import (
	"context"
	"net/http"

	"handy/server/backends"
	"handy/server/util"
)

type InternalHandler = func (context.Context, http.ResponseWriter, *http.Request)

type HandlerWrapper struct {
	aw *util.AuditWriter
	cs *backends.CookieStorage
}

func NewHandlerWrapper(aw *util.AuditWriter, cs *backends.CookieStorage) *HandlerWrapper {
	return &HandlerWrapper{
		aw: aw,
		cs: cs,
	}
}

func (hw *HandlerWrapper) WrapHandler(ih InternalHandler) func (w http.ResponseWriter, r *http.Request) {
	return func (w http.ResponseWriter, r *http.Request) {
		ctx := context.Background()
		cookie := util.ExtractCookieFromRequest(r)
		if cookie != "" {
			ci, err := hw.cs.UnpackCookie(cookie)
			if err != nil {
				HandleError(w, err, http.StatusBadRequest)
				return
			}
			ctx = context.WithValue(ctx, "cookie", ci)
		}

		ih(ctx, w, r)

		hw.aw.Write(ctx, r)
	}
}