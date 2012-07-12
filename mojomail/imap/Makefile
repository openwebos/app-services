# Device makefile

BUILD_TYPE ?= release
PLATFORM ?= linux-arm
OBJDIR := $(BUILD_TYPE)-$(PLATFORM)
LOCAL_INCLUDES := -I$(QPEDIR)/include/mojodb \
				  -I$(QPEDIR)/include/email-common \
				  -I./inc -I./test -I.
LOCAL_CFLAGS := $(CFLAGS) -Wall -Werror -O2 -Wuninitialized -Winit-self -DMOJ_LINUX -DMOJ_DEBUG_LOGGING $(LOCAL_INCLUDES) $(shell pkg-config --cflags glib-2.0) -Wno-error=strict-aliasing
LOCAL_CXXFLAGS := $(LOCAL_CFLAGS) $(CXXFLAGS) -Wno-error=strict-aliasing
LIBS := -lboost_regex

include Makefile.inc