package main

import (
	"log"
	"net/http"
	"os"
	"flag"

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

	hw := handlers.NewHandlerWrapper(auditWriter, cookieStorage)
	http.HandleFunc("/login", hw.WrapHandler(handlers.NewLoginHandler(cookieStorage, userStorage)))
	http.HandleFunc("/register", hw.WrapHandler(handlers.NewRegisterHandler(userStorage)))
	http.HandleFunc("/profile/picture", hw.WrapHandler(handlers.NewProfilePictureHandler(avatarGenerator)))
	http.HandleFunc("/tasks", hw.WrapHandler(handlers.NewTaskHandler(taskStorage)))
	http.HandleFunc("/", hw.WrapHandler(handlers.NewRootHandler(userStorage)))
	log.Fatal(http.ListenAndServe("localhost:8080", gorilla_handlers.LoggingHandler(os.Stdout, http.DefaultServeMux)))
}
