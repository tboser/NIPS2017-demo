from pprint import pprint
import numpy as np
import time
import matplotlib.animation as anim
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec

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




f = plt.figure(figsize=(8, 8))
#"Mother Grid spec with enough room for two mnist chars display side by side, plus 1 row below for summary plots
gsMother = gridspec.GridSpec(3, 1)
#one row with two columns for chars displays
gsChars = gridspec.GridSpecFromSubplotSpec(1, 2, subplot_spec=gsMother[0])
# the two nx*ny grids for the char displays
gsFPGA = gridspec.GridSpecFromSubplotSpec(nx, ny, subplot_spec=gsChars[0])
#add character displays
axsFPGA = fillCharGrid(f, gsFPGA)
gsGPU = gridspec.GridSpecFromSubplotSpec(nx, ny, subplot_spec=gsChars[1])
axsGPU = fillCharGrid(f, gsGPU)
#one row for summary plots
gsSumm = gridspec.GridSpecFromSubplotSpec(1, 2, subplot_spec=gsMother[1])
axAcc = plt.Subplot(f,gsSumm[0])
f.add_subplot(axAcc)
#one row for the two buttons
gsButtons = gridspec.GridSpecFromSubplotSpec(1, 2, subplot_spec=gsMother[2])
#






#add buttons, fill plots in callbacks

class Index(object):
    indFPGA = 0
    indGPU = 0

    def updateChars(self, axs, ind, results, nImages):
        #simulate the reading of results from board
        from random import randint
        istop=0
        istart = ind
        ind += randint(0,1000)
        istop = ind
        ids = [i for i in range(istart, istop) if (i%100 == 0)]
        if (istop < nImages):
            color_digits(axs, ids, results)
        return istop;
    
            
    def start(self, event):
        nImages=len(y_test)
        while (self.indFPGA < nImages or self.indGPU < nImages):
            accFPGA = float(np.count_nonzero(resultsFPGA))/len(resultsFPGA)
            accGPU = float(np.count_nonzero(resultsGPU))/len(resultsGPU)
            print "accFPGA= ", accFPGA, " accGPU= ", accGPU
            ind=np.arange(2)
            acc= np.array([accFPGA, accGPU])
            axAcc.bar(ind, acc, color="Green")
            axAcc.bar(ind, np.array([1,1])-acc, bottom=acc, color="Red")
            plt.sca(axAcc)
            plt.xticks(ind, ['FPGA', 'GPU'])
            plt.draw()
            self.indFPGA=self.updateChars(axsFPGA, self.indFPGA, resultsFPGA, nImages)
            self.indGPU=self.updateChars(axsGPU, self.indGPU, resultsGPU, nImages)
            
        print "done. Please restart"            
        from threading import Thread

    def restart(self, event):
        self.indFPGA=0
        self.indGPU=0
        plot_digits(axsFPGA)
        plot_digits(axsGPU)
    
def refreshPlot(fig):
    fig.canvas.draw()
    fig.canvas.flush_events()

def plot_digits(axs):
    for i in range(0,len(y_test),len(y_test)/nx/ny):        
        plot_mnist(axs[i/nx/ny], y_test[i], x_test[i])
    plt.draw()    

def color_digits(axs, ids, results):
#    mutex.acquire()
#    try:
    for i in ids:
        color_mnist(axs[i/nx/ny], results[i], x_test[i])
    plt.draw()    
    refreshPlot(f)
#    finally:
#        mutex.release()


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
