import numpy as np
import time

import matplotlib
matplotlib.use('TkAgg')

import matplotlib.animation as anim
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import matplotlib.patches as mpatches

from matplotlib.widgets import Button
from keras.datasets import mnist

(x_train, y_train), (x_test, y_test) = mnist.load_data()

(nx,ny)=(10,10)
colors=['Greens', 'Reds']


### INITIALIZATION FUNCTIONS ###
def fillCharGrid(fig, gs):
    axs=[]
    (nx,ny) = gs.get_geometry()
    for i in range(nx*ny):
        ax=plt.Subplot(fig, gs[i])
        fig.add_subplot(ax)
        axs += [ax]
    return axs

def plot_mnist(splt, label, pixels):
    return plot_mnist_digit(splt, pixels)

def plot_digits(axs):
    images = [plot_mnist(axs[i/nx/ny], y_test[i], x_test[i]) for i in range(0,len(y_test),len(y_test)/nx/ny)]
    plt.draw()  
    return images

### HELPER METHODS ###
def plot_mnist_digit(splt, pixels, col='gray'):
    splt.axes.get_xaxis().set_visible(False)
    splt.axes.get_yaxis().set_visible(False)
    return splt.imshow(pixels, cmap=col, animated=True)

def refreshPlot(fig):
    #fig.canvas.draw_idle()
    fig.canvas.flush_events()

def hardRefresh(fig):
    fig.canvas.draw_idle()
    fig.canvas.flush_events()
 

def compare_array(truth, arr):
    if truth == arr.index(max(arr)):
        return True
    else:
        return False

def color_mnist_digit(digit, ax, color):
    digit.set_cmap(color)
    ax.draw_artist(ax.patch)
    ax.draw_artist(digit)
    f.canvas.blit(ax.bbox)

class Index(object):

    def __init__(self, fpga_imgs, gpu_imgs, ocl_imgs, axsFPGA, axsGPU, axsOCL):
        self.fpga_imgs = fpga_imgs
        self.gpu_imgs = gpu_imgs
        self.ocl_imgs = ocl_imgs

        self.axsFPGA = axsFPGA
        self.axsGPU = axsGPU
        self.axsOCL = axsOCL

        self.acc_fpga_line = None
        self.acc_gpu_line = None
        self.acc_ocl_line = None

        self.exe_fpga_line = None
        self.exe_gpu_line = None
        self.exe_ocl_line = None

        self.fpga_correct = []
        self.gpu_correct = []
        self.ocl_correct = []

        self.fpga_exetot = []
        self.gpu_exetot = []
        self.ocl_exetot = []

        self.accleg = None
        self.exeleg = None

        self.exe_yticks = None


    def initExe(self, ax):
        ax.cla()
        #ax.set_xlim([0, 3])
        #ax.set_ylim([0 3])
        ax.set_xlabel("log10(Miliseconds per predition)")
        ax.title.set_text("Execution Time")
        fpatch = mpatches.Patch(color='blue', label='FPGA')
        gpatch = mpatches.Patch(color='yellow', label='GPU')
        rpatch = mpatches.Patch(color='red', label="OpenCL")
        ax.legend(handles=[fpatch,gpatch, rpatch])


    def updateExe(self, ax, exeFPGA, exeGPU, exeOCL):
        exeFPGA = np.log10(exeFPGA)
        exeGPU = np.log10(exeGPU)
        exeOCL = np.log10(exeOCL)
        bins = np.linspace(min(exeFPGA),max(exeOCL), 60)
        ax.hist(exeFPGA, bins, alpha=0.5, color="blue", label="FPGA")
        ax.hist(exeGPU, bins, alpha=0.5, color="yellow", label="GPU")
        ax.hist(exeOCL, bins, alpha=0.5, color="red", label="OpenCL")
        plt.draw()

    def init_exe(self, ax):
        ax.cla()
        ax.set_xlim([0,10000])
        ax.set_ylim([0, 45000])
        ax.set_xlabel("Predictions done")
        ax.set_ylabel("Time elapsed (ms)")
        ax.title.set_text("Execution Time")
        self.exe_fpga_line, = ax.plot([0], '-o', color="blue", alpha=0.3, linewidth=0.3, linestyle="-", label="FPGA")
        self.exe_gpu_line, = ax.plot([0], '-o', color="yellow", alpha=0.3, linewidth=0.3, linestyle="-", label="GPU")
        self.exe_ocl_line, = ax.plot([0], '-o', color="red", alpha=0.3, linewidth=0.3, linestyle="-", label="OpenCL")
        self.exeleg = ax.legend()

        plt.sca(ax)
        #self.exe_yticks = ax.yticks(range(0, 10, 2), range(0, 10, 2))

    def update_exe(self, i):
        axExe.draw_artist(axExe.patch)
        axExe.draw_artist(self.exeleg)

        mx = int(max(self.ocl_exetot)+1.0)

        #self.exeloc = range(0, mx, (mx/6)+1)
        #self.exelab = range(0, mx, (mx/6)+1)
        #axExe.draw_artist(self.exeloc)
        #axExe.draw_artist(self.exelab)
        #self.exe_yticks.update(range(0, mx, (mx/6)+1), range(0, mx, (mx/6)+1))
        #axExe.draw_artist(self.exe_yticks)

        #print "fpga exetot"
        #print self.fpga_exetot
        #print range(0, i+1, 100)
        #axExe.set_ylim([0, mx])

        self.exe_fpga_line.set_ydata(self.fpga_exetot)
        self.exe_fpga_line.set_xdata(range(0, i+1, 100))
        axExe.draw_artist(self.exe_fpga_line)

        self.exe_gpu_line.set_ydata(self.gpu_exetot)
        self.exe_gpu_line.set_xdata(range(0, i+1, 100))
        axExe.draw_artist(self.exe_gpu_line)

        self.exe_ocl_line.set_ydata(self.ocl_exetot)
        self.exe_ocl_line.set_xdata(range(0, i+1, 100))
        axExe.draw_artist(self.exe_ocl_line)
    
        f.canvas.blit(axExe.bbox)


    def initAcc(self, ax):
        ax.cla()
        ax.set_xlim([0, 1])
        ax.set_ylim([-1, 3])
        ax.set_xlabel("Fraction")
        ax.title.set_text("Classification Accuracy")
        plt.sca(ax)
        plt.yticks(np.arange(3), ['FPGA', 'GPU', 'OpenCL'])
        gpatch = mpatches.Patch(color='Green', label='Right')
        rpatch = mpatches.Patch(color='Red', label='Wrong')
        ax.legend(handles=[gpatch,rpatch])

    def updateAcc(self, ax, fpga_acc, gpu_acc, ocl_acc):
        ind=np.arange(3)
        acc= np.array([fpga_acc, gpu_acc, ocl_acc])
        ax.barh(ind, acc, color="Green", label="Correct")
        ax.barh(ind, np.array([1,1,1])-acc, left=acc, color="Red")
        plt.draw()

    def init_acc(self, ax):
        ax.cla()
        ax.set_xlim([0,10000])
        ax.set_ylim([0, 1.1])
        ax.set_xlabel("Predictions done")
        ax.set_ylabel("Percent Correct")
        ax.title.set_text("Classification Accuracy")
        self.acc_fpga_line, = ax.plot([0], '-o', color="blue", alpha=0.3, linewidth=0.3, linestyle="-", label="FPGA")
        self.acc_gpu_line, = ax.plot([0], '-o', color="yellow", alpha=0.3, linewidth=0.3, linestyle="-", label="GPU")
        self.acc_ocl_line, = ax.plot([0], '-o', color="red", alpha=0.3, linewidth=0.3, linestyle="-", label="OpenCL")
        self.accleg = ax.legend()
        #self.accticks = ax.xticks(range(0, 10, 2), range(0, 10, 2))

    def update_acc(self, i):
        axAcc.draw_artist(axAcc.patch)
        axAcc.draw_artist(self.accleg)

        #axAcc.set_xlim([0, i])
        #self.accticks.
        #axAcc.draw_artist(self.accticks)
        axAcc.set_xticks(range(0, i, (i/5)+1), range(0, i, (i/5)+1))
        #print(self.fpga_correct)
        #print(range(0, i*100+1, 100))
        self.acc_fpga_line.set_ydata(self.fpga_correct)
        self.acc_fpga_line.set_xdata(range(0, i+1, 100))
        axAcc.draw_artist(self.acc_fpga_line)

        self.acc_gpu_line.set_ydata(self.gpu_correct)
        self.acc_gpu_line.set_xdata(range(0, i+1, 100))
        axAcc.draw_artist(self.acc_gpu_line)

        self.acc_ocl_line.set_ydata(self.ocl_correct)
        self.acc_ocl_line.set_xdata(range(0, i+1, 100))
        axAcc.draw_artist(self.acc_ocl_line)
    
        f.canvas.blit(axAcc.clipbox)


    def start(self, event):
        if len(self.fpga_exetot) != 0:
            self.restart(event)

        with open("fpga_output.txt") as fpga:
            flst = [map(float, line.rstrip().split()) for line in fpga.readlines()]
        with open("gpu_output.txt") as gpu:
            glst = [map(float, line.rstrip().split()) for line in gpu.readlines()]
        with open("ocl_output.txt") as ocl:
            olst = [map(float, line.rstrip().split()) for line in ocl.readlines()]

        fpga_correct = 0
        gpu_correct = 0
        ocl_correct = 0
        gpu_exectime = []
        fpga_exectime = []
        ocl_exectime = []

        self.init_acc(axAcc)
        self.init_exe(axExe)
        hardRefresh(f)

        for idx in range(0, 10000):

            print "idx", idx
            gpu_exectime.append(float(glst[idx][-1])*1000.0)
            fpga_exectime.append(float(flst[idx][-1])*1000.0)
            ocl_exectime.append(float(olst[idx][-1])*1000.0)
            if idx%100 == 0:
                if compare_array(y_test[idx], glst[idx][0:10]):
                    #plot_mnist_digit(self.axsGPU[idx/nx/ny], x_test[idx], 'Greens')
                    color_mnist_digit(self.gpu_imgs[idx/nx/ny], self.axsGPU[idx/nx/ny], 'Greens')
                    gpu_correct += 1
                else:
                    #plot_mnist_digit(self.axsGPU[idx/nx/ny], x_test[idx], 'Reds')
                    color_mnist_digit(self.gpu_imgs[idx/nx/ny], self.axsGPU[idx/nx/ny], 'Reds')

                if compare_array(y_test[idx], flst[idx][0:10]):
                    #plot_mnist_digit(self.axsFPGA[idx/nx/ny], x_test[idx], 'Greens')
                    color_mnist_digit(self.fpga_imgs[idx/nx/ny], self.axsFPGA[idx/nx/ny], 'Greens')
                    fpga_correct += 1
                else:
                    #plot_mnist_digit(self.axsFPGA[idx/nx/ny], x_test[idx], 'Reds')
                    color_mnist_digit(self.fpga_imgs[idx/nx/ny], self.axsFPGA[idx/nx/ny], 'Reds')

                if compare_array(y_test[idx], olst[idx][0:10]):
                    #plot_mnist_digit(self.axsOCL[idx/nx/ny], x_test[idx], 'Greens')
                    color_mnist_digit(self.ocl_imgs[idx/nx/ny], self.axsOCL[idx/nx/ny], 'Greens')
                    ocl_correct += 1
                else:
                    #plot_mnist_digit(self.axsOCL[idx/nx/ny], x_test[idx], 'Reds')
                    color_mnist_digit(self.ocl_imgs[idx/nx/ny], self.axsOCL[idx/nx/ny], 'Reds')

                #plt.draw()
                #update accuracy, #correct
                #plt.pause(0.001)
                #refreshPlot(f)
                self.fpga_correct.append(float(fpga_correct)/float((idx+1)))
                self.gpu_correct.append(float(gpu_correct)/float((idx+1)))
                self.ocl_correct.append(float(ocl_correct)/float((idx+1)))

                self.fpga_exetot.append(sum(fpga_exectime))
                self.gpu_exetot.append(sum(gpu_exectime))
                self.ocl_exetot.append(sum(ocl_exectime))

                self.update_acc(idx)
                self.update_exe(idx)
                time.sleep(0.1)
                refreshPlot(f)

            else:
                if compare_array(y_test[idx], glst[idx][0:10]):
                    gpu_correct += 1
                if compare_array(y_test[idx], flst[idx][0:10]):
                    fpga_correct += 1
                if compare_array(y_test[idx], olst[idx][0:10]):
                    ocl_correct += 1
        
        time.sleep(5)
        self.initAcc(axAcc)
        self.initExe(axExe)

        print "fpga correct:", fpga_correct
        print "gpu correct:", gpu_correct
        print "ocl correct:", ocl_correct
        self.updateAcc(axAcc, float(fpga_correct)/float((idx+1)), float(gpu_correct)/float((idx+1)), float(ocl_correct)/float(idx+1))
        self.updateExe(axExe, fpga_exectime, gpu_exectime, ocl_exectime)

        hardRefresh(f)
            
        print "done. Please restart"            

    def restart(self, event):
        self.fpga_imgs = plot_digits(axsFPGA)
        self.gpu_imgs = plot_digits(axsGPU)
        self.ocl_imgs = plot_digits(axsOCL)

        self.acc_fpga_line = None
        self.acc_gpu_line = None
        self.acc_ocl_line = None

        self.exe_fpga_line = None
        self.exe_gpu_line = None
        self.exe_ocl_line = None

        self.fpga_correct = []
        self.gpu_correct = []
        self.ocl_correct = []

        self.fpga_exetot = []
        self.gpu_exetot = []
        self.ocl_exetot = []    


### CREATE AXES ###
f = plt.figure(figsize=(12, 8))
#f = plt.subplots(figsize=(12,8))
#"Mother Grid spec with enough room for two mnist chars display side by side, plus 1 row below for summary plots
gsMother = gridspec.GridSpec(3, 1, height_ratios=[4,3,1])
#one row with two columns for chars displays
gsChars = gridspec.GridSpecFromSubplotSpec(1, 3, subplot_spec=gsMother[0])
# the two nx*ny grids for the char displays
gsFPGA = gridspec.GridSpecFromSubplotSpec(nx, ny, subplot_spec=gsChars[0])
axsFPGA = fillCharGrid(f, gsFPGA)
#axsFPGA.title.set_text("FPGA")
#gsFPGA.update(top="FPGA")

gsGPU = gridspec.GridSpecFromSubplotSpec(nx, ny, subplot_spec=gsChars[1])
axsGPU = fillCharGrid(f, gsGPU)
#axsGPU.title.set_text("GPU")


gsOCL = gridspec.GridSpecFromSubplotSpec(nx, ny, subplot_spec=gsChars[2])
axsOCL = fillCharGrid(f, gsOCL)
#axsOCL.title.set_text("OCL")

#one row for summary plots
gsSumm = gridspec.GridSpecFromSubplotSpec(1, 2, subplot_spec=gsMother[1])
axAcc = plt.Subplot(f,gsSumm[0])
#axAcc.set_xlabel("Fraction")
f.add_subplot(axAcc)
axExe = plt.Subplot(f,gsSumm[1])
#axExe.set_xlabel("Secs/100 digits")
f.add_subplot(axExe)

fpga_imgs = plot_digits(axsFPGA)
gpu_imgs = plot_digits(axsGPU)
ocl_imgs = plot_digits(axsOCL)

aFPGA = plt.axes([0.2, 0.95, 0.1, 0.05])
aOCL = plt.axes([0.475, 0.95, 0.1, 0.05])
aGPU = plt.axes([0.75, 0.95, 0.1, 0.05])


bGPU = Button(aGPU, 'Keras GPU')
bOCL = Button(aOCL, 'OpenCL FPGA')
bFPGA = Button(aFPGA, 'VHDL FPGA')


callback = Index(fpga_imgs, gpu_imgs, ocl_imgs, axsFPGA, axsGPU, axsOCL)
# bstart.on_clicked(callback.start)
# bstop = Button(axstop, 'Reset')
# bstop.on_clicked(callback.restart)

mng = plt.get_current_fig_manager()
mng.resize(*mng.window.maxsize())

plt.show(block=False)
#imfig, imax = plt.subplots(num=2)

while True:
#    imfig=plt.figure(2)
#    img=plt.imread("HEPTrkX One Slide.jpg")
#    plt.imshow(img, extent=[0, 45000, 0, 7500])
#    hardRefresh(imfig)
#    plt.show(block=False)
#    time.sleep(5)
    #plt.sca(gsMother)
    plt.show(block=False)
    f=plt.figure(1)
    hardRefresh(f)
    callback.start(0)
    time.sleep(5)

