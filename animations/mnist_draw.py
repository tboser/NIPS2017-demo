from pprint import pprint
import numpy as np
import time
import matplotlib.animation as anim
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import matplotlib.patches as mpatches

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
gsMother = gridspec.GridSpec(3, 1, height_ratios=[4,3,1])
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
axAcc.set_xlabel("Fraction")
f.add_subplot(axAcc)
axExe = plt.Subplot(f,gsSumm[1])
axExe.set_xlabel("Secs/100 digits")
f.add_subplot(axExe)
#one row for the two buttons
#gsButtons = gridspec.GridSpecFromSubplotSpec(1, 2, subplot_spec=gsMother[2])
#






#add buttons, fill plots in callbacks

class Index(object):
    indFPGA = 0
    indGPU = 0
    exeFPGA = [0.]
    exeGPU = [0.]
    once = True
    
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
    
    def updateExe(self, ax):
        ax.title.set_text("Execution Time")
        fpatch = mpatches.Patch(color='blue', label='FPGA')
        gpatch = mpatches.Patch(color='yellow', label='GPU')
        ax.legend(handles=[fpatch,gpatch])
        if (self.indFPGA * self.indGPU):
            #just making something up for exec time/digit
            print self.indFPGA - self.exeFPGA[-1]
            self.exeFPGA += [600.0/(self.indFPGA - self.exeFPGA[-1])]
            self.exeGPU += [200.0/(self.indGPU - self.exeGPU[-1])]
            print "exeFPGA= ", self.exeFPGA[-1], " exeGPU= ", self.exeGPU[-1]
            bins = np.linspace(0.,0.6, 30)
            ax.hist(self.exeFPGA, bins, alpha=0.5, color="blue", label="FPGA")
            ax.hist(self.exeGPU, bins, alpha=0.5, color="yellow", label="GPU")
            plt.draw()

    def updateAcc(self, ax):
        ax.title.set_text("Classification Accuracy")
        plt.sca(ax)
        ind=np.arange(2)
        plt.yticks(ind, ['FPGA', 'GPU'])
        gpatch = mpatches.Patch(color='Green', label='Right')
        rpatch = mpatches.Patch(color='Red', label='Wrong')
        ax.legend(handles=[gpatch,rpatch])
        if (self.indFPGA * self.indGPU):
            accFPGA = float(np.count_nonzero(resultsFPGA[:self.indFPGA]))/self.indFPGA
            accGPU = float(np.count_nonzero(resultsGPU[:self.indGPU]))/self.indGPU
            print "accFPGA= ", accFPGA, " accGPU= ", accGPU
            acc= np.array([accFPGA, accGPU])
            ax.barh(ind, acc, color="Green", label="Correct")
            ax.barh(ind, np.array([1,1])-acc, left=acc, color="Red")
            plt.draw()

    
    def start(self, event):
        nImages=len(y_test)
        while (self.indFPGA < nImages or self.indGPU < nImages):
            self.updateAcc(axAcc)
            self.updateExe(axExe)
            self.indFPGA=self.updateChars(axsFPGA, self.indFPGA, resultsFPGA, nImages)
            self.indGPU=self.updateChars(axsGPU, self.indGPU, resultsGPU, nImages)
            if self.once: self.once=False
            
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
bstop = Button(axstop, 'Reset')
bstop.on_clicked(callback.restart)

plot_digits(axsFPGA)
plot_digits(axsGPU)
callback.updateAcc(axAcc)
callback.updateExe(axExe)

#ani=anim.ArtistAnimation(f,images, interval=50, blit=False, repeat=False)
plt.show()
