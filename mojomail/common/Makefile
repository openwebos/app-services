# Device makefile

BUILD_TYPE ?= release
PLATFORM ?= linux-arm
OBJDIR := $(BUILD_TYPE)-$(PLATFORM)
LOCAL_INCLUDES := -I$(QPEDIR)/include/mojodb -I./inc -I./test -I. -I$(QPEDIR)/include/palmsocket/IncsPublic
LOCAL_CFLAGS := $(CFLAGS) -fPIC -Wall -Werror -Wuninitialized -Winit-self -DMOJ_LINUX -DMOJ_DEBUG_LOGGING -DPALMSOCKET_NEW $(LOCAL_INCLUDES) $(shell pkg-config --cflags glib-2.0) -Wno-error=strict-aliasing
LOCAL_CXXFLAGS := $(LOCAL_CFLAGS) $(CXXFLAGS) -Wno-error=strict-aliasing
LIBS := -lboost_regex -lmojocore -lmojoluna

include Makefile.inc
