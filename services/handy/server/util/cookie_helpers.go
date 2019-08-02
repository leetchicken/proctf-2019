package util

import (
	"net/http"
)

const (
	cookieName = "session"
)

func ExtractCookieFromRequest(r *http.Request) string {
	for _, cookie := range r.Cookies() {
		if cookie.Name == cookieName {
			return cookie.Value
		}
	}
	return ""
}

func AddCookieToResponse(w http.ResponseWriter, cookie string) {
	http.SetCookie(w, &http.Cookie{
		Name: cookieName,
		Value: cookie,
	})
}