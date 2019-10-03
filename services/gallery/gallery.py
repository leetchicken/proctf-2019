import dnn
import numpy as np

#MODEL_PATH = "models/model.h5"
#MODEL_PATH = "model_big_30_30.h5"
MODEL_PATH = "models/model_big_30_30_predict.h5"

# train_model = dnn.create_and_train_model()
# model = dnn.create_model(train_model)
# model.save(MODEL_PATH)

model = dnn.load(MODEL_PATH)

def calc_dist(painting, replica):
	painting_emb = model.predict(np.array([np.array(painting)]))
	replica_emb = model.predict(np.array([np.array(replica)]))
	dist = np.linalg.norm(painting_emb - replica_emb)
	return dist