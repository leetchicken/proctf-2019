package main

import (
	"io"
	"net/http"
)

func storageHandler(result http.ResponseWriter, req *http.Request){
	result.WriteHeader(200)
	io.WriteString(result, `hey, maan!`)
}


func main()  {
	http.HandleFunc("/", storageHandler)
	http.ListenAndServe(":9000", nil)
}

