from pprint import pprint
import numpy as np
import time
import matplotlib.animation as anim
import matplotlib.pyplot as plt
from matplotlib.widgets import Button
from keras.datasets import mnist
(x_train, y_train), (x_test, y_test) = mnist.load_data()
(nx,ny)=(10,10)
colors=['Greens', 'Reds']
results= np.random.randint(2, size=len(y_test))

(f,ax) = plt.subplots(nx,ny)
plt.subplots_adjust(bottom=0.2)

def plot_mnist(i, splts, label, pixels):
    ip = i/nx/ny
    iy = ip % ny
    ix = (ip % (nx*ny)) / nx
    print i, ip, ix, iy, label
    plot_mnist_digit(splts[ix,iy], pixels)

def color_mnist(i, splts, result, pixels):
    ip = i/nx/ny
    iy = ip % ny
    ix = (ip % (nx*ny)) / nx
    print i, ip, ix, iy, result
    plot_mnist_digit(splts[ix,iy], pixels, colors[result])

def plot_mnist_digit(splt, pixels, col='gray'):
    splt.axes.get_xaxis().set_visible(False)
    splt.axes.get_yaxis().set_visible(False)
    splt.imshow(pixels, cmap=col, animated=True)

class Index(object):
    ind = 0

    def start(self, event):
        #simulate the reading of results from board
        from random import randint
        istart = self.ind
        self.ind += randint(0,1000)
        istop = self.ind
        ids = [i for i in range(istart, istop) if (i%100 == 0)]
        if (istop < len(y_test)):
            color_digits(ids)
        else:
            print "done. Please restart"
    def restart(self, event):
        self.ind=0
        plot_digits()
    
def plot_digits():
    for i in range(0,len(y_test),len(y_test)/nx/ny):
        plot_mnist(i, ax, y_test[i], x_test[i])
        plt.draw()    
def color_digits(ids):
    for i in ids:
        color_mnist(i, ax, results[i], x_test[i])
        plt.draw()    

callback = Index()
axstop = plt.axes([0.7, 0.05, 0.1, 0.075])
axstart = plt.axes([0.81, 0.05, 0.1, 0.075])
bstart = Button(axstart, 'Update')
bstart.on_clicked(callback.start)
bstop = Button(axstop, 'Restart')
bstop.on_clicked(callback.restart)

plot_digits()


#ani=anim.ArtistAnimation(f,images, interval=50, blit=False, repeat=False)
plt.show()
