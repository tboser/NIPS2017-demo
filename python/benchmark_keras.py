import keras
import operator
import time
import numpy as np
from keras.datasets import mnist
from keras.models import Sequential, load_model
from keras.layers import Dense, Flatten
from keras.layers import Conv2D, MaxPooling2D
from keras.layers import Activation, Lambda
from keras.activations import relu
from keras import backend as K

#set up data for training
batch_size = 128
num_classes = 10
epochs = 10

# input image dimensions
img_rows, img_cols = 28, 28

# the data, shuffled and split between train and test sets
(x_train, y_train), (x_test, y_test) = mnist.load_data()

if K.image_data_format() == 'channels_first':
    x_train = x_train.reshape(x_train.shape[0], 1, img_rows, img_cols)
    x_test = x_test.reshape(x_test.shape[0], 1, img_rows, img_cols)
    input_shape = (1, img_rows, img_cols)
else:
    x_train = x_train.reshape(x_train.shape[0], img_rows, img_cols, 1)
    x_test = x_test.reshape(x_test.shape[0], img_rows, img_cols, 1)
    input_shape = (img_rows, img_cols, 1)

x_train = x_train.astype('float32')
x_test = x_test.astype('float32')

print('x_train shape:', x_train.shape)
print(x_train.shape[0], 'train samples')
print(x_test.shape[0], 'test samples')

# convert class vectors to binary class matrices
y_train = keras.utils.to_categorical(y_train, num_classes)
y_test = keras.utils.to_categorical(y_test, num_classes)


model = load_model('lenet.h5')

fh = open("keras_benchmark.txt", "w")
for inp in x_test:
	inp = np.reshape(inp, (1, 28, 28, 1))
	start = time.time()
	res = model.predict(inp)
	end = time.time()

	#print(res)
	for digit in res[0]:
		fh.write("%.8f " % (float(digit)*100000.0)) #to make numbers readable
	fh.write(str(end-start)+"\n")

fh.close()

start = time.time()
res = model.predict(x_test)
end = time.time()

print("Total time to predict:", end-start, "seconds")