package main

import (
	"flag"
	"fmt"
	"html/template"
	"log"
	"net/http"
	"os"

	gorilla_handlers "github.com/gorilla/handlers"

	"handy/server/backends"
	"handy/server/handlers"
	"handy/server/util"
)

var (
	listenHost      = flag.String("listen-host", "0.0.0.0", "Host to listen to.")
	listenPort      = flag.String("listen-port", "8080", "Port to listen to.")
	templatesGlob   = flag.String("templates-glob", "./templates/*.html", "Glob for the HTML templates.")
	staticDirectory = flag.String("static-directory", "./static/", "Directory for static files.")
	certPath        = flag.String("server-cert-path", "/etc/handy/cert.pem", "Path to the TLS certificate.")
	keyPath         = flag.String("server-key-path", "/etc/handy/key.pem", "Path to the TLS private key.")
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
	tokenChecker, err := backends.NewTokenChecker()
	if err != nil {
		log.Fatalf("Failed to create TokenChecker: %s", err)
	}
	t, err := template.ParseGlob(*templatesGlob)
	if err != nil {
		log.Fatalf("Failed to load templates: %s", err)
	}
	if err = util.CheckTLSConfig(*certPath, *keyPath, *listenHost); err != nil {
		log.Fatalf("Failed to check TLS config: %s", err)
	}

	hwf := handlers.NewHandlerWrapperFactory(auditWriter, cookieStorage)
	http.Handle("/login", hwf.Wrap(handlers.NewLoginHandler(cookieStorage, userStorage, t)))
	http.Handle("/logout", hwf.Wrap(handlers.NewLogoutHandler()))
	http.Handle("/register", hwf.Wrap(handlers.NewRegisterHandler(userStorage, tokenChecker, t)))
	http.Handle("/profile", hwf.Wrap(handlers.NewProfileHandler(userStorage, t)))
	http.Handle("/profile/picture", hwf.Wrap(handlers.NewProfilePictureHandler(avatarGenerator)))
	http.Handle("/tasks", hwf.Wrap(handlers.NewTaskHandler(taskStorage, t)))
	http.Handle("/masters", hwf.Wrap(handlers.NewMasterHandler(userStorage, t)))
	http.Handle("/static/", http.StripPrefix("/static/", http.FileServer(http.Dir(*staticDirectory))))
	http.Handle("/", hwf.Wrap(handlers.NewRootHandler(t)))
	defaultHandler := gorilla_handlers.LoggingHandler(os.Stdout, http.DefaultServeMux)
	log.Fatal(http.ListenAndServeTLS(fmt.Sprintf("%s:%s", *listenHost, *listenPort), *certPath, *keyPath, defaultHandler))
}
