package util

import (
	"fmt"
)

func CheckFieldLength(value string, min, max int) error {
	if len(value) < min || len(value) > max {
		return fmt.Errorf("field length is %d, should be in [%d, %d]", len(value), min, max)
	}
	return nil
}
