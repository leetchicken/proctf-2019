package util

import (
	"errors"
	"fmt"
	"net/http"

	"handy/server/constants"
	"handy/server/data"
)

var (
	UserNotPresentError = errors.New("user session is not present")
)

func ExtractCookieFromRequest(r *http.Request) string {
	for _, cookie := range r.Cookies() {
		if cookie.Name == constants.SessionCookieName {
			return cookie.Value
		}
	}
	return ""
}

func AddCookieToResponse(w http.ResponseWriter, cookie string) {
	http.SetCookie(w, &http.Cookie{
		Name:  constants.SessionCookieName,
		Value: cookie,
	})
}

func RetrieveUserInfo(r *http.Request) (string, *data.LocalUserInfo, error) {
	ctx := r.Context()
	ci, ok := ctx.Value(constants.SessionCookieName).(*data.CookieInfo)
	if !ok {
		return "", nil, UserNotPresentError
	}
	lui, err := ci.GetLocalUserInfo()
	if err != nil {
		return "", nil, fmt.Errorf("failed to extract local user info: %s", err)
	}
	return ci.GetID(), lui, nil
}
