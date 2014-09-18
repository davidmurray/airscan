export ARCHS = armv7 arm64

export ADDITIONAL_CFLAGS = -I$(THEOS_PROJECT_DIR)/ios-reversed-headers

include theos/makefiles/common.mk

TOOL_NAME = airscan
airscan_FILES = main.c
airscan_FRAMEWORKS = CoreFoundation
airscan_PRIVATE_FRAMEWORKS = MobileWiFi
airscan_CODESIGN_FLAGS = -Sentitlements.plist

include $(THEOS_MAKE_PATH)/tool.mk
