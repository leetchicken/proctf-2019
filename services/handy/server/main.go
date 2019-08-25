package main

import (
	"flag"
	"log"
	"net/http"
	"os"

	gorilla_handlers "github.com/gorilla/handlers"

	"handy/server/backends"
	"handy/server/handlers"
	"handy/server/util"
)

func main() {
	flag.Parse()

	auditWriter := util.NewAuditWriter()

	mongoDB, err := backends.MongoDatabase()
	if err != nil {
		log.Fatalf("Failed to connecto to the Mongo database: %s", err)
	}
	cookieStorage, err := backends.NewCookieStorage()
	if err != nil {
		log.Fatalf("Failed to create CookieStorage: %s", err)
	}
	userStorage, err := backends.NewUserStorage(mongoDB)
	if err != nil {
		log.Fatalf("Failed to create UserStorage: %s", err)
	}
	taskStorage, err := backends.NewTaskStorage(mongoDB)
	if err != nil {
		log.Fatalf("Failed to create TaskStorage: %s", err)
	}
	avatarGenerator, err := backends.NewAvatarGenerator()
	if err != nil {
		log.Fatalf("Failed to create AvatarGenerator: %s", err)
	}

	hwf := handlers.NewHandlerWrapperFactory(auditWriter, cookieStorage)
	http.Handle("/login", hwf.Wrap(handlers.NewLoginHandler(cookieStorage, userStorage)))
	http.Handle("/register", hwf.Wrap(handlers.NewRegisterHandler(userStorage)))
	http.Handle("/profile", hwf.Wrap(handlers.NewProfileHandler(userStorage)))
	http.Handle("/profile/picture", hwf.Wrap(handlers.NewProfilePictureHandler(avatarGenerator)))
	http.Handle("/tasks", hwf.Wrap(handlers.NewTaskHandler(taskStorage)))
	http.Handle("/masters", hwf.Wrap(handlers.NewMasterHandler(userStorage)))
	http.Handle("/", hwf.Wrap(handlers.NewRootHandler()))
	log.Fatal(http.ListenAndServe("localhost:8080", gorilla_handlers.LoggingHandler(os.Stdout, http.DefaultServeMux)))
}
