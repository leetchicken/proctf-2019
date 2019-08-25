package handlers

import (
	"context"
	"log"
	"net/http"

	"handy/server/backends"
	"handy/server/constants"
	"handy/server/util"
)

type HandlerWrapperFactory struct {
	aw *util.AuditWriter
	cs *backends.CookieStorage
}

func NewHandlerWrapperFactory(aw *util.AuditWriter, cs *backends.CookieStorage) *HandlerWrapperFactory {
	return &HandlerWrapperFactory{
		aw: aw,
		cs: cs,
	}
}

func (f *HandlerWrapperFactory) Wrap(h http.Handler) *handlerWrapper {
	return &handlerWrapper{
		aw: f.aw,
		cs: f.cs,
		h:  h,
	}
}

type handlerWrapper struct {
	aw *util.AuditWriter
	cs *backends.CookieStorage
	h  http.Handler
}

func (hw *handlerWrapper) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	cookie := util.ExtractCookieFromRequest(r)
	if cookie != "" {
		ci, err := hw.cs.UnpackCookie(cookie)
		if err != nil {
			HandleError(w, err, http.StatusBadRequest)
			return
		}
		ctx = context.WithValue(ctx, constants.SessionCookieName, ci)
	}
	r = r.WithContext(ctx)

	pw := &proxyingResponseWriter{
		w:        w,
		finished: false,
		status:   http.StatusOK,
	}
	hw.h.ServeHTTP(pw, r)

	err := hw.aw.Write(r, pw.status)
	if err != nil {
		if !pw.finished {
			HandleError(w, err, http.StatusBadRequest)
		} else {
			log.Printf("Failed to write audit log: %s", err)
		}
	}
}

type proxyingResponseWriter struct {
	w        http.ResponseWriter
	finished bool
	status   int
}

func (w *proxyingResponseWriter) Header() http.Header {
	return w.w.Header()
}

func (w *proxyingResponseWriter) Write(buf []byte) (int, error) {
	w.finished = true
	w.status = http.StatusOK
	return w.w.Write(buf)
}

func (w *proxyingResponseWriter) WriteHeader(statusCode int) {
	w.finished = true
	w.status = statusCode
	w.w.WriteHeader(statusCode)
}
