package backends

import (
	"context"
	"flag"
	"time"

	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"
)

var (
	mongoConnectString  = flag.String("mongo-connect-string", "mongodb://localhost:27017", "MongoDB address to connect to")
	mongoConnectTimeout = flag.Duration("mongo-connect-timeout", 10*time.Second, "MongoDB connection timeout")
	mongoDatabaseName = flag.String("mongo-db-name", "handy", "Name of the database to use in MongoDB")
)

func MongoDatabase() (*mongo.Database, error) {
	ctx, _ := context.WithTimeout(context.Background(), *mongoConnectTimeout)
	client, err := mongo.Connect(ctx, options.Client().ApplyURI(*mongoConnectString))
	if err != nil {
		return nil, err
	}
	return client.Database("mongoDatabaseName"), nil
}
