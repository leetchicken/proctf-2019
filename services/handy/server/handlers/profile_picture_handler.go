package handlers

import (
	"fmt"
	"context"
	"net/http"

	"handy/server/backends"
	"handy/server/util"
)

func NewProfilePictureHandler(ag *backends.AvatarGenerator) InternalHandler {
	return func (ctx context.Context, w http.ResponseWriter, r *http.Request) {
		id, err := util.ExtractUUIDFromURL(r.URL, "id")
		if err != nil {
			HandleError(w, fmt.Errorf("parameter 'id' invalid: %s", err), http.StatusBadRequest)
			return
		}
		size, err := util.ExtractIntFromURL(r.URL, "size")
		if err != nil {
			HandleError(w, fmt.Errorf("parameter 'size' invalid: %s", err), http.StatusBadRequest)
			return
		}
		if err := ag.GenerateAvatar(id, size, w); err != nil {
			HandleError(w, err, http.StatusBadRequest)
		}
	}
}