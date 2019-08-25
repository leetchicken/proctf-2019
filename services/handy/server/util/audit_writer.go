package util

import (
	"fmt"
	"log"
	"net/http"
)

type AuditWriter struct {
}

func NewAuditWriter() *AuditWriter {
	return &AuditWriter{}
}

func (w *AuditWriter) Write(r *http.Request, status int) error {
	userStr := "<anonymous>"
	id, lui, err := RetrieveUserInfo(r)
	if err == nil {
		userStr = fmt.Sprintf("'%s' (%s)", lui.Username, id)
	} else if err != UserNotPresentError {
		return fmt.Errorf("failed to retrieve user info: %s", err)
	}

	log.Printf("AUDIT: user %s requested \"%s %s\"", userStr, r.Method, r.URL)
	return nil
}
