package handlers

import (
	"log"
	"net/http"
)

func HandleError(w http.ResponseWriter, err error, status int) {
	log.Printf("Error with code %d: %s", status, err)
	http.Error(w, http.StatusText(status), status)
}
