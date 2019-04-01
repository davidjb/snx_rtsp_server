NAME = snx_rtsp_server

PWD     	:= $(shell pwd)
TARGET      ?= arm-linux
CROSS_COMPILE   ?= $(TARGET)-
CPP     	:= $(CROSS_COMPILE)g++
CC      	:= $(CROSS_COMPILE)gcc
AR      	:= $(CROSS_COMPILE)ar
RANLIB      := $(CROSS_COMPILE)ranlib
STRIP 		:= $(CROSS_COMPILE)strip
INSTALL_DIR	?= ../rootfs/usr
BIN_INS_DIR	:= $(INSTALL_DIR)/bin/example
LIB_INS_DIR	:= $(INSTALL_DIR)/lib/example
CATEGORY	:= ipc_func/rtsp_server
KERNEL_DIR	?= ../../../../../kernel/linux-2.6.35.12
MIDDLEWARE_INS_DIR ?= ../../../../../middleware/_install
AUTOCONF_DIR	?= ../../../../../buildscript/include
GENERATED_DIR   := $(AUTOCONF_DIR)/generated
INC     	= -I$(KERNEL_DIR)/src/include
INC     	+= -I$(MIDDLEWARE_INS_DIR)
INC     	+= -I$(MIDDLEWARE_INS_DIR)/include/snx_vc
INC			+= -I$(MIDDLEWARE_INS_DIR)/include/snx_rc
INC			+= -I$(MIDDLEWARE_INS_DIR)/include/snx_isp
INC			+= -I$(MIDDLEWARE_INS_DIR)/include/snx_record
INC			+= -I$(MIDDLEWARE_INS_DIR)/include
INC 		+= -I$(PWD)/include
INC			+= -I live/BasicUsageEnvironment/include
INC			+= -I live/groupsock/include
INC			+= -I live/liveMedia/include
INC			+= -I live/UsageEnvironment/include

## toolchain
EXTRA_LDFLAGS	= 
EXTRA_CFLAGS	+= -I$(AUTOCONF_DIR)
EXTRA_LDFLAGS   += -Wl,-Bstatic -Wl,-Bdynamic

CFLAGS      := -Wall -O2 $(EXTRA_CFLAGS) $(INC) -DSOCKLEN_T=socklen_t -DNO_SSTREAM=1 -D_LARGEFILE_SOURCE=1  -D_FILE_OFFSET_BITS=64 -static
CFLAGS		+= -DBSD=1 -DLOCALE_NOT_USED
CFLAGS		+= -DAUDIO_STREAM=1
#CFLAGS		+= -DDEBUG

LDFLAGS     := $(EXTRA_LDFLAGS) -Wl,--as-needed -lpthread
LDFLAGS 	+= -Wl,--no-export-dynamic -Wl,--no-gc-sections -Wl,--print-gc-sections $(EXTRA_LDFLAGS)

LIVE_LIB 	= live/UsageEnvironment/libUsageEnvironment.a
LIVE_LIB 	+= live/groupsock/libgroupsock.a
LIVE_LIB 	+= live/BasicUsageEnvironment/libBasicUsageEnvironment.a
LIVE_LIB 	+= live/liveMedia/libliveMedia.a


LDFLAGS		+= -lm -ldl
LDFLAGS		+= -L$(MIDDLEWARE_INS_DIR)/lib
LDFLAGS		+= -lsnx_record 
LDFLAGS		+= -lavformat  -lavcodec -lavutil 
LDFLAGS		+= -lxml
LDFLAGS		+= -lz

SNX_VC_LIB = $(MIDDLEWARE_INS_DIR)/lib/libsnx_vc.so
SNX_RC_LIB = $(MIDDLEWARE_INS_DIR)/lib/libsnx_rc.so
SNX_RC_LIB += $(MIDDLEWARE_INS_DIR)/lib/libsnx_isp.so 
SNX_AUDIO_LIB = $(MIDDLEWARE_INS_DIR)/lib/libasound.so $(MIDDLEWARE_INS_DIR)/lib/libsnx_audio.so

SNX_VIDEO_CODEC = snx_video_codec.o
VERSION = version.o
SRC_OBJ	= main.o 
SRC_OBJ	+=ServerMediaSubsession.o
SRC_OBJ	+=H264_V4l2DeviceSource.o V4l2DeviceSource.o
SRC_OBJ	+=AlsaDeviceSource.o
SRC_OBJ	+= sn98600_v4l2.o 
SRC_OBJ	+= sn98600_record_audio.o
SRC_OBJ	+= sn986_play.o
SRC_OBJ	+= data_buf.o
SRC_OBJ += uvc/sonix_xu_ctrls.o 
#SRC_OBJ +=uvc/v4l2uvc.o uvc/nalu.o 
#targets =
targets = live555
targets += snx_rtsp_server


.PHONY : clean distclean all
%.o : %.cpp
	$(CPP) -c $(CFLAGS) $<

%.o : %.c
	$(CC) -c $(CFLAGS) $<

all: $(targets)

$(VERSION): ../../version.c
	$(CPP) -I$(GENERATED_DIR) -c -o $@ $<

live555:
	$(MAKE) -C live all  INSTALL_DIR=$(abspath $(BIN_INS_DIR))

snx_rtsp_server: $(VERSION)    $(SRC_OBJ) $(LIVE_LIB) $(SNX_VC_LIB) $(SNX_RC_LIB) $(SNX_AUDIO_LIB)
	cp sonix_xu_ctrls.o uvc/
	$(CPP) -o $@ $^ -rdynamic $(LDFLAGS)
	$(STRIP) $@


.PHONY: install
install:
	@ if [ ! -d $(BIN_INS_DIR)/$(CATEGORY) ]; \
	then \
	install -d $(BIN_INS_DIR)/$(CATEGORY); \
	fi
	install -c $(NAME) $(BIN_INS_DIR)/$(CATEGORY)

clean:
	rm -f *.o *.yuv *.bak *.a *.out *.so *.d uvc/*.o snx_rtsp_server
	$(MAKE) -C live $@  INSTALL_DIR=$(abspath $(BIN_INS_DIR))

distclean : clean
	rm -f $(targets)
