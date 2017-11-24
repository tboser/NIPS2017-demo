# gen_inputs.py
# Generate file "test_inputs.h" with some inputs to test our CNN layers
# NOTE - this program written to support keras version 2.x not 1.2.x 

from __future__ import print_function
import keras
import numpy as np
from keras.datasets import mnist
from keras.models import Sequential
from keras.layers import Dense, Flatten
from keras.layers import Conv2D, MaxPooling2D
from keras import backend as K

# Let's grab the mnist dataset and train our model a tiny bit
#set up data for training
batch_size = 128
num_classes = 10
epochs = 1

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
x_train /= 255
x_test /= 255
print('x_train shape:', x_train.shape)
print(x_train.shape[0], 'train samples')
print(x_test.shape[0], 'test samples')

# convert class vectors to binary class matrices
y_train = keras.utils.to_categorical(y_train, num_classes)
y_test = keras.utils.to_categorical(y_test, num_classes)

#model for training with sigmoid activation
model = Sequential()
model.add(Conv2D(6, kernel_size=(5, 5),
                 activation='relu',
                 input_shape=input_shape))
model.add(MaxPooling2D(pool_size=(2,2)))
model.add(Conv2D(16, (5, 5))) #no activation
model.add(MaxPooling2D(pool_size=(2, 2)))
model.add(Flatten())
model.add(Dense(120, activation='relu'))
model.add(Dense(84, activation='relu'))
model.add(Dense(num_classes, activation='sigmoid'))

model.compile(loss=keras.losses.categorical_crossentropy,
              optimizer=keras.optimizers.Adadelta(),
              metrics=['accuracy'])

#show summary of our model
model.summary()

#train and test our model
model.fit(x_train, y_train,
          batch_size=batch_size,
          epochs=epochs,
          verbose=1,
          validation_data=(x_test, y_test))

#Now let's get some data
def keras_get_layer_output(layer, test_input):
    """
    Helper method, gives the output matrix from a Keras layer
    """
    get_layer_output = K.function([layer.input],
                                  [layer.output])
    return get_layer_output([test_input])[0]

def write_array_to_file(file, array_name, array):
	file.write("float[] "+array_name+" {")
	file.write(str(array[0]))
	for value in array[1:]:
		file.write(", "+str(value))
	file.write("};\n")

i = 0
layer_in = np.reshape(x_test[0], (1, 28, 28, 1))
layer_out = None
fh = open("test_inputs.h", "w")
for layer in model.layers:
	layer_out = keras_get_layer_output(layer, layer_in)
	if i == 0: #conv with relu
		write_array_to_file(fh, "conv_relu_in", layer_in.flatten())
		write_array_to_file(fh, "conv_relu_out", layer_out.flatten())
		write_array_to_file(fh, "conv_relu_weights", layer.get_weights()[0].flatten())
		write_array_to_file(fh, "conv_relu_bias", layer.get_weights()[1].flatten())
	if i == 1: #maxpool
		write_array_to_file(fh, "max_pool_in", layer_in.flatten())
		write_array_to_file(fh, "max_pool_out", layer_out.flatten())
	if i == 2: #conv no activation
		write_array_to_file(fh, "conv_none_in", layer_in.flatten())
		write_array_to_file(fh, "conv_none_out", layer_out.flatten())
		write_array_to_file(fh, "conv_none_weights", layer.get_weights()[0].flatten())
		write_array_to_file(fh, "conv_none_bias", layer.get_weights()[1].flatten())
	if i == 5: #dense with relu
		write_array_to_file(fh, "dense_in", layer_in.flatten())
		write_array_to_file(fh, "dense_out", layer_out.flatten())
		write_array_to_file(fh, "dense_weights", layer.get_weights()[0].flatten())
		write_array_to_file(fh, "dense_bias", layer.get_weights()[1].flatten())
	layer_in = layer_out
	i += 1
fh.close()