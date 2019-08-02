package util

import (
	"errors"
	"net/http"
	"context"
)

type AuditWriter struct {

}

func NewAuditWriter() *AuditWriter {
	return &AuditWriter{}
}

func (w *AuditWriter) Write(ctx context.Context, r *http.Request) error {
	return errors.New("not implemented")
}