#
TARGETS = test_FPGAIORegs DE10_direct ian DE_mod

#
ALT_DEVICE_FAMILY ?= soc_cv_av
SOCEDS_DEST_ROOT = /media/usb_drive/
SOCEDS_ROOT ?= $(SOCEDS_DEST_ROOT)
HWLIBS_ROOT = $(SOCEDS_ROOT)/ip/altera/hps/altera_hps/hwlib
#CROSS_COMPILE = arm-linux-gnueabihf-
#CROSS_COMPILE = arm-angstrom-linux-gnueabi-
CFLAGS = -g -Wall -D$(ALT_DEVICE_FAMILY) -I$(HWLIBS_ROOT)/include/$(ALT_DEVICE_FAMILY)   -I$(HWLIBS_ROOT)/include/
ON_DE10 = -DONDE10
CXXFLAGS = -std=c++11 -Wall $(ON_DE10) -D$(ALT_DEVICE_FAMILY) -I$(HWLIBS_ROOT)/include/$(ALT_DEVICE_FAMILY)   -I$(HWLIBS_ROOT)/include/ -I ../mnist/include
LDFLAGS =  -Wall 
CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
ARCH= arm

build: $(TARGETS)

ian: ian.o

DE10_direct: DE10_direct.o FPGAIORegs.o TrainedLayers.o
	$(CXX) $(LDFLAGS)   $^ -o $@  

DE_mod: DE_mod.o FPGAIORegs.o TrainedLayers.o
	$(CXX) $(LDFLAGS)   $^ -o $@  

test_FPGAIORegs: test_FPGAIORegs.o FPGAIORegs.o TrainedLayers.o
	$(CXX) $(LDFLAGS)   $^ -o $@  

%.o: %.cxx
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(TARGETS) *.a *.o *~ 
