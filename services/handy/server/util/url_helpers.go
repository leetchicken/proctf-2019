package util

import (
	"fmt"
	"net/url"
	"strconv"

	"github.com/google/uuid"
)

func ExtractUUIDFromURL(u *url.URL, name string) (uuid.UUID, error) {
	qValues := u.Query()
	values, ok := qValues[name]
	if !ok || len(values) != 1 {
		return uuid.Nil, fmt.Errorf("parameter '%s' not present or present multiple times", name)
	}


	value, err := uuid.Parse(values[0])
	if err != nil {
		return uuid.Nil, fmt.Errorf("could not parse parmeter '%s' with value '%s' as UUID", name, values[0])
	}
	return value, nil
}

func ExtractIntFromURL(u *url.URL, name string) (int, error) {
	qValues := u.Query()
	values, ok := qValues[name]
	if !ok || len(values) != 1 {
		return 0, fmt.Errorf("parameter '%s' not present or present multiple times", name)
	}
	value, err := strconv.Atoi(values[0])
	if err != nil {
		return 0, fmt.Errorf("could not parse parmeter '%s' with value '%s' as int", name, values[0])
	}
	return value, nil
}