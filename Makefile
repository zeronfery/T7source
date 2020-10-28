################################################################################
#
# Makefile project only supported on Mac OS X and Linux Platforms)
# Tailor to simple project , adjust it by xiaoyong pan
################################################################################
PWD=$(shell pwd)
LICHEE_DIR=/home/jrf/T7_projrct/t7_v1.2.0rls
LICHEE_OUT_SYS= $(LICHEE_DIR)/out/sun8iw17p1/linux/common/buildroot
GNUEABI=gnueabi
OPENCV_DIR = /usr/local/ARM/ARMopencv
Chain_tools = /home/jrf/t7_v1.2.0rls/tools/build/toolchain/gcc-linaro-5.3.1-2016.05-x86_64_arm-linux-gnueabi/bin

STD_11=-std=c++11 -std=gnu++11

# CC = $(Chain_tools)/arm-linux-$(GNUEABI)-gcc --sysroot=$(LICHEE_OUT_SYS)/host/usr/arm-buildroot-linux-$(GNUEABI)/sysroot
# CPP= $(Chain_tools)/arm-linux-$(GNUEABI)-g++ --sysroot=$(LICHEE_OUT_SYS)/host/usr/arm-buildroot-linux-$(GNUEABI)/sysroot

CC = $(LICHEE_OUT_SYS)/host/opt/ext-toolchain/bin/arm-linux-$(GNUEABI)-gcc --sysroot=$(LICHEE_OUT_SYS)/host/usr/arm-buildroot-linux-$(GNUEABI)/sysroot
CPP= $(LICHEE_OUT_SYS)/host/opt/ext-toolchain/bin/arm-linux-$(GNUEABI)-g++ --sysroot=$(LICHEE_OUT_SYS)/host/usr/arm-buildroot-linux-$(GNUEABI)/sysroot

MY_LIB_PATH = $(PWD)/my_lib
EXT_OBJS = 

TARGET       = jrf_traingle
############################
# end deprecated interface #
############################
SRCS        := $(shell find -name "*.cpp")
SRCS        += $(shell find -name "*.c")

OBJS = $(patsubst %.c, %.o, $(wildcard *.c))
OBJS += $(patsubst %.cpp, %.o, $(wildcard *.cpp))

#some marcos
DEFINES=-D_SUNXIW17_

CCFLAGS += -g -Wall  -rdynamic -fPIC -std=c++11 -std=gnu++11 $(DEFINES)

INCLUDES  +=  -I./	\
			  -I$(LICHEE_DIR)/tools/pack/chips/sun8iw17p1/hal/gpu/fbdev/include \
			  -I$(OPENCV_DIR)/include \
			  #-I/home/jrf/Application/yaml-cpp/include

LIBRARIES += -L$(MY_LIB_PATH) -L$(LICHEE_DIR)/tools/pack/chips/sun8iw17p1/hal/gpu/fbdev/lib
LIBRARIES +=  L-lpthread -lGLESv2 -lEGL #-lyaml-cpp
LIBRARIES += -L$(OPENCV_DIR)/lib -lopencv_core -lopencv_imgproc -lopencv_imgcodecs -lopencv_highgui -lopencv_videoio -lopencv_calib3d -lopencv_flann #-lopencv_features2d
LIBRARIES += -L$(LICHEE_DIR)/out/sun8iw17p1/linux/common/buildroot/host/usr/arm-buildroot-linux-gnueabi/sysroot/usr/lib
# Target rules
all: build
	@echo "make finish!!!"

build:${TARGET}

%.o : %.cpp
	$(CPP) $(CCFLAGS) $(INCLUDES) -o $@ -c $< 

%.o : %.c
	$(CC)  $(CCFLAGS) $(INCLUDES) -o $@ -c $< 

${TARGET}: $(OBJS) $(EXT_OBJS)
	$(CPP) -o $@ $+ $(CCFLAGS) $(INCLUDES) $(LIBRARIES) 
	#cp -rf ${TARGET} /mnt/hgfs/linux_share/upload
	
clean:
	@rm -f  ${TARGET} $(OBJS) *~
	
cleanall:
	@rm -f  ${TARGET} $(OBJS) *~
	
