from keras.models import load_model
from keras.layers import Input, Conv2D, Lambda, Dense, Flatten, Dropout, MaxPooling2D, concatenate
from keras.models import Model, Sequential
from keras.regularizers import l2
from keras import backend as K
from keras.optimizers import SGD,Adam
from keras.losses import binary_crossentropy

import random
import numpy as np

def generate_triplet(x1,x2,x3, testsize=0.25):
    trainsize = 1-testsize
    train_len = int(trainsize * len(x1))
    
    triplet_train_pairs = []
    triplet_test_pairs = []
    keys = list(x1.keys())
    i = 0
    for num in keys:
        if num not in x2:
          continue
        if num not in x3:
          continue

        if i < train_len:
          pairs = triplet_train_pairs
        else:
          pairs = triplet_test_pairs

        rnum = np.random.choice(keys)
        if rnum != num and rnum in x1:
          pairs.append([x1[num], x2[num], x1[rnum]])
          pairs.append([x1[num], x3[num], x1[rnum]])
          i+=1


    return np.array(triplet_train_pairs), np.array(triplet_test_pairs)

def triplet_loss(y_true, y_pred, alpha = 0.4):
    total_lenght = y_pred.shape.as_list()[-1]
    
    anchor = y_pred[:,0:int(total_lenght*1/3)]
    positive = y_pred[:,int(total_lenght*1/3):int(total_lenght*2/3)]
    negative = y_pred[:,int(total_lenght*2/3):int(total_lenght*3/3)]

    pos_dist = K.sum(K.square(anchor-positive),axis=1)

    neg_dist = K.sum(K.square(anchor-negative),axis=1)

    basic_loss = pos_dist-neg_dist+alpha
    loss = K.maximum(basic_loss,0.0)
 
    return loss

def create_base_network():
    
    model = Sequential()
    model.add(Conv2D(32,(5,5),padding='same',activation='relu',name='conv1'))
    model.add(Dropout(0.3,name='drop1'))
    model.add(MaxPooling2D(name='pool1'))
    
    model.add(Conv2D(64,(3,3),padding='same',activation='relu',name='conv2'))
    model.add(Dropout(0.3,name='drop2'))
    model.add(MaxPooling2D(name='pool2'))
    
    model.add(Conv2D(128,(3,3),padding='same',activation='relu',name='conv3'))
    model.add(Dropout(0.3,name='drop3'))
    model.add(MaxPooling2D(name='pool3'))
    
    model.add(Conv2D(256,(3,3),padding='same',activation='relu',name='conv4'))
    model.add(Dropout(0.3,name='drop4'))
    model.add(MaxPooling2D(name='pool4'))
    
    model.add(Conv2D(512,(3,3),padding='same',activation='relu',name='conv5'))
    model.add(Dropout(0.3,name='drop5'))
    model.add(MaxPooling2D(name='pool5'))
    
    model.add(Conv2D(1024,(3,3),padding='same',activation='relu',name='conv6'))
    model.add(Dropout(0.3,name='drop6'))
    model.add(MaxPooling2D(name='pool6'))
    
    model.add(Conv2D(2048,(3,3),padding='same',activation='relu',name='conv7'))
    model.add(Dropout(0.3,name='drop7'))
    model.add(MaxPooling2D(name='pool7'))
    
    model.add(Conv2D(4096,(3,3),padding='same',activation='relu',name='conv8'))
    model.add(Dropout(0.3,name='drop8'))    
    
    model.add(Flatten(name='flatten'))
    
    model.add(Dense(16, activation='relu'))
    model.add(Dense(16, activation='linear', name='embeddings', kernel_regularizer=l2(1e-3)))
    
    model.add(Lambda(lambda t: K.l2_normalize(t, axis=1)))
    
    return model

def create_train_network():
	shared_model = create_base_network()

	anchor_input = Input((128,128,3, ), name='anchor_input')
	positive_input = Input((128,128,3, ), name='positive_input')
	negative_input = Input((128,128,3, ), name='negative_input')
 
	encoded_anchor = shared_model(anchor_input)
	encoded_positive = shared_model(positive_input)
	encoded_negative = shared_model(negative_input)

	merged_vector = concatenate([encoded_anchor, encoded_positive, encoded_negative], axis=-1, name='merged_layer')

	train_model = Model(inputs=[anchor_input, positive_input, negative_input], outputs=merged_vector)

	adam_optim = Adam(lr=0.0001, beta_1=0.9, beta_2=0.999)
	train_model.compile(loss=triplet_loss, optimizer=adam_optim)

	return train_model

def create_and_fit_train_model(X1, X2, X3):
    train_model = create_train_network()

    x_train, x_test = generate_triplet(X1,X2,X3,X4,X5)

    Anchor = x_train[:,0,:]
    Positive = x_train[:,1,:]
    Negative = x_train[:,2,:]
    Anchor_test = x_test[:,0,:]
    Positive_test = x_test[:,1,:]
    Negative_test = x_test[:,2,:]

    Y_dummy = np.empty((Anchor.shape[0],1))
    Y_dummy_test = np.empty((Anchor_test.shape[0],1))
    H = model.fit([Anchor,Positive,Negative], y=Y_dummy, validation_data=([Anchor_test,Positive_test,Negative_test],Y_dummy_test), batch_size=64, epochs=30)    

    return model

def create_model(train_model):
    input = Input((128,128,3, ), name='input')
    encoded = train_model.layers[3](input)
    return Model(inputs=input, outputs=encoded)

def load(path):
    custom_objects={'triplet_loss': triplet_loss}
    return load_model(path, custom_objects=custom_objects)
