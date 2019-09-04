package handlers

import (
	"context"
	"fmt"
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
		w: w,
	}
	hw.h.ServeHTTP(pw, r)

	err := hw.aw.Write(r)
	if err != nil {
		HandleError(w, err, http.StatusBadRequest)
		return
	}

	for _, p := range pw.proxied {
		if err = p(); err != nil {
			HandleError(w, err, http.StatusInternalServerError)
		}
	}
}

type proxyingResponseWriter struct {
	proxied []func() error

	w http.ResponseWriter
}

func (w *proxyingResponseWriter) Header() http.Header {
	return w.w.Header()
}

func (w *proxyingResponseWriter) Write(buf []byte) (int, error) {
	storedBuf := make([]byte, len(buf))
	copy(storedBuf, buf)
	w.proxied = append(w.proxied, func() error {
		if cnt, err := w.w.Write(storedBuf); err != nil {
			return err
		} else if cnt != len(storedBuf) {
			return fmt.Errorf("failed to write result: %d/%d bytes written", cnt, len(storedBuf))
		}
		return nil
	})
	return len(buf), nil
}

func (w *proxyingResponseWriter) WriteHeader(statusCode int) {
	w.proxied = append(w.proxied, func() error {
		w.w.WriteHeader(statusCode)
		return nil
	})
}
