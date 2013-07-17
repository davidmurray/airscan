include theos/makefiles/common.mk

TOOL_NAME = airscan
airscan_FILES = main.c
airscan_PRIVATE_FRAMEWORKS = MobileWiFi

include $(THEOS_MAKE_PATH)/tool.mk
