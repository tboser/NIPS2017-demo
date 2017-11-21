from pprint import pprint
import numpy as np
import time
import matplotlib.animation as anim
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import thread

from matplotlib.widgets import Button
from keras.datasets import mnist
(x_train, y_train), (x_test, y_test) = mnist.load_data()
(nx,ny)=(10,10)
colors=['Greens', 'Reds']
resultsFPGA= np.random.randint(2, size=len(y_test))
resultsGPU= np.random.randint(2, size=len(y_test))
#plt.ion()

def fillCharGrid(fig, gs):
    axs=[]
    (nx,ny) = gs.get_geometry()
    for i in range(nx*ny):
        ax=plt.Subplot(fig, gs[i])
        fig.add_subplot(ax)
        axs += [ax]
    return axs

#plt.subplots_adjust(bottom=0.2)

def plot_mnist(splt, label, pixels):
    plot_mnist_digit(splt, pixels)

def color_mnist(splt, result, pixels):
    plot_mnist_digit(splt, pixels, colors[result])

def plot_mnist_digit(splt, pixels, col='gray'):
    splt.axes.get_xaxis().set_visible(False)
    splt.axes.get_yaxis().set_visible(False)
    splt.imshow(pixels, cmap=col, animated=True)




#"Mother Grid spec with enough room for two mnist chars display side by side, plus 1 row below for summary plots
gsMother = gridspec.GridSpec(3, 1)
#one row with two columns for chars displays
gsChars = gridspec.GridSpecFromSubplotSpec(1, 2, subplot_spec=gsMother[0])
# the two nx*ny grids for the char displays
gsFPGA = gridspec.GridSpecFromSubplotSpec(nx, ny, subplot_spec=gsChars[0])
gsGPU = gridspec.GridSpecFromSubplotSpec(nx, ny, subplot_spec=gsChars[1])
#one row for three summary plots
gsSumm = gridspec.GridSpecFromSubplotSpec(1, 3, subplot_spec=gsMother[1])
#one row for the two buttons
gsButtons = gridspec.GridSpecFromSubplotSpec(1, 2, subplot_spec=gsMother[2])
#

f = plt.figure(figsize=(8, 8))

#add character displays
axsFPGA = fillCharGrid(f, gsFPGA)
#pprint(axsFPGA)
axsGPU = fillCharGrid(f, gsGPU)




#add buttons, fill plots in callbacks

class Index(object):
    indFPGA = 0
    indGPU = 0

    def update(self, axs, ind, results):
        #simulate the reading of results from board
        from random import randint
        istop=0
        while True:
            istart = ind
            ind += randint(0,1000)
            istop = ind
            ids = [i for i in range(istart, istop) if (i%100 == 0)]
            if (istop < len(y_test)):
                color_digits(axs, ids, results)
                self.refresh(f)
            else:
                print "done. Please restart"
                break
        return ind;
    def refresh(self,fig):
        fig.canvas.draw()
        fig.canvas.flush_events()
        
    def start(self, event):
        self.indFPGA=thread.start_new_thread(self.update, (axsFPGA, self.indFPGA, resultsFPGA))
        self.indFPGA=thread.start_new_thread(self.update, (axsGPU, self.indGPU, resultsGPU))
        #        thread.start_new_thread(self.refresh, (f))
        
    def restart(self, event):
        self.indFPGA=0
        self.indGPU=0
        plot_digits(axsFPGA)
        plot_digits(axsGPU)
    
def plot_digits(axs):
    for i in range(0,len(y_test),len(y_test)/nx/ny):        
        plot_mnist(axs[i/nx/ny], y_test[i], x_test[i])
        plt.draw()    
def color_digits(axs, ids, results):
    for i in ids:
        color_mnist(axs[i/nx/ny], results[i], x_test[i])
        plt.draw()    


#axstop = plt.Subplot(f, gsButtons[0])
#f.add_subplot(axstop)
#axstart = plt.Subplot(f, gsButtons[1])
#f.add_subplot(axstart)
axstop = plt.axes([0.7, 0.05, 0.1, 0.075])
axstart = plt.axes([0.81, 0.05, 0.1, 0.075])
callback = Index()
bstart = Button(axstart, 'Start')
bstart.on_clicked(callback.start)
bstop = Button(axstop, 'Restart')
bstop.on_clicked(callback.restart)

plot_digits(axsFPGA)
plot_digits(axsGPU)


#ani=anim.ArtistAnimation(f,images, interval=50, blit=False, repeat=False)
plt.show()
