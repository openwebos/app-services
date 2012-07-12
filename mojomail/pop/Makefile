# Device makefile

BUILD_TYPE := release
PLATFORM := linux-arm

LOCAL_INCLUDES := -I$(QPEDIR)/include/mojodb -I$(QPEDIR)/include/email-common
LOCAL_CFLAGS := $(CFLAGS) -Wall -Werror -DMOJ_LINUX $(LOCAL_INCLUDES) $(shell pkg-config --cflags glib-2.0)
LOCAL_CPPFLAGS := $(CPPFLAGS)

include Makefile.inc