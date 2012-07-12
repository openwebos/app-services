# Device makefile

BUILD_TYPE ?= release
PLATFORM ?= linux-arm
OBJDIR := $(BUILD_TYPE)-$(PLATFORM)

LOCAL_INCLUDES := \
	-I$(QPEDIR)/include/mojodb \
	-I$(QPEDIR)/include/palmsocket/IncsPublic \
	-I$(QPEDIR)/include/email-common \
	-I./src -I./test -I. \
	-I./inc
	
LOCAL_CFLAGS := $(CFLAGS) -Wall -Werror -DMOJ_LINUX $(shell pkg-config --cflags glib-2.0) -Wno-error=strict-aliasing
LOCAL_CXXFLAGS := $(LOCAL_CFLAGS) $(CXXFLAGS) -Wno-error=strict-aliasing

include Makefile.inc