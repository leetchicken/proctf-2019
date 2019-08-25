package backends

import (
	"bytes"
	"context"
	"fmt"

	"github.com/golang/protobuf/jsonpb"
	"github.com/golang/protobuf/proto"
	"go.mongodb.org/mongo-driver/bson"
	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"
)

var (
	protobufJsonMarshaler   = &jsonpb.Marshaler{}
	protobufJsonUnmarshaler = &jsonpb.Unmarshaler{
		AllowUnknownFields: true,
	}
)

func ensureIndexExists(c *mongo.Collection, field string) error {
	ctx := context.Background()
	iv := c.Indexes()
	cur, err := iv.List(ctx)
	if err != nil {
		return fmt.Errorf("failed to list indexes: %s", err)
	}
	defer cur.Close(ctx)

	for cur.Next(ctx) {
		elem := &map[string]interface{}{}
		if err := cur.Decode(elem); err != nil {
			return fmt.Errorf("failed to get over indexes: %s", err)
		}

		if _, ok := (*elem)[field]; ok {
			return nil
		}
	}
	if err := cur.Err(); err != nil {
		return fmt.Errorf("indexes reading error: %s", err)
	}

	unique := true
	_, err = iv.CreateOne(
		ctx,
		mongo.IndexModel{
			Keys: bson.D{{field, 1}},
			Options: &options.IndexOptions{
				Unique: &unique,
			},
		},
	)
	return err
}

func protbufToBson(m proto.Message) (*bson.D, error) {
	msg, err := protobufJsonMarshaler.MarshalToString(m)
	if err != nil {
		return nil, fmt.Errorf("failed to convert proto to string: %s", err)
	}
	doc := &bson.D{}
	if err := bson.UnmarshalExtJSON([]byte(msg), true, doc); err != nil {
		return nil, err
	}
	return doc, nil
}

func bsonToProtobuf(d *bson.D, m proto.Message) error {
	msg, err := bson.MarshalExtJSON(d, true, false)
	if err != nil {
		return err
	}
	r := bytes.NewReader(msg)
	return protobufJsonUnmarshaler.Unmarshal(r, m)
}
