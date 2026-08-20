TARGET := iphone:clang:latest:7.0
INSTALL_TARGET_PROCESSES = SpringBoard

include $(THEOS)/makefiles/common.mk

TWEAK_NAME = WatchdogDisabler

TOOL_NAME = disabledog

WatchdogDisabler_FILES = Tweak.x
WatchdogDisabler_CFLAGS = -fobjc-arc

disabledog_FILES = disabledog.c
disabledog_FRAMEWORKS = CoreFoundation IOKit
disabledog_CODESIGN_FLAGS = -Sentitlements.plist
disabledog_INSTALL_PATH = /usr/bin

include $(THEOS_MAKE_PATH)/tweak.mk
include $(THEOS_MAKE_PATH)/tool.mk
