package util

import (
	"fmt"
	"net/url"
	"strconv"

	"github.com/google/uuid"
)

func ExtractUUIDFromURL(u *url.URL, name string) (uuid.UUID, error) {
	valueStr, err := ExtractStringFromValues(u.Query(), name)
	value, err := uuid.Parse(valueStr)
	if err != nil {
		return uuid.Nil, fmt.Errorf("could not parse parmeter '%s' with value '%s' as UUID", name, valueStr)
	}
	return value, nil
}

func ExtractIntFromURL(u *url.URL, name string) (int, error) {
	valueStr, err := ExtractStringFromValues(u.Query(), name)
	value, err := strconv.Atoi(valueStr)
	if err != nil {
		return 0, fmt.Errorf("could not parse parmeter '%s' with value '%s' as int", name, valueStr)
	}
	return value, nil
}

func ExtractStringFromValues(v url.Values, name string) (string, error) {
	values, ok := v[name]
	if !ok || len(values) != 1 {
		return "", fmt.Errorf("parameter '%s' not present or present multiple times", name)
	}
	return values[0], nil
}
