LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def

LOCAL_SRC_FILES:= \
    bsl430-platform.c \
    bsl430.c \
    bsl430-program.c

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    liblog \
    libhi_common \
    libhi_msp

LOCAL_STATIC_LIBRARIES := \
    libpmrpc

LOCAL_MODULE:= libbsl430
LOCAL_32_BIT_ONLY := true

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../include \
    $(COMMON_UNF_INCLUDE) \
    $(COMMON_DRV_INCLUDE) \
    $(COMMON_API_INCLUDE) \
    $(MSP_UNF_INCLUDE) \
    $(MSP_DRV_INCLUDE) \
    $(MSP_API_INCLUDE) \
    $(HI_BOARD_HEAD_FILE_DIR)

include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
include $(SDK_DIR)/Android.def

LOCAL_SRC_FILES:= \
    bsl430-platform.c \
    bsl430.c \
    bsl430-program.c

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    liblog \
    libhi_common \
    libhi_msp

LOCAL_STATIC_LIBRARIES := \
    libpmrpc

LOCAL_MODULE:= libbsl430-clog
LOCAL_32_BIT_ONLY := true

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../include \
    $(COMMON_UNF_INCLUDE) \
    $(COMMON_DRV_INCLUDE) \
    $(COMMON_API_INCLUDE) \
    $(MSP_UNF_INCLUDE) \
    $(MSP_DRV_INCLUDE) \
    $(MSP_API_INCLUDE) \
    $(HI_BOARD_HEAD_FILE_DIR)

LOCAL_CFLAGS := -DBSL430_LOG_CONSOLE

include $(BUILD_STATIC_LIBRARY)


include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    bsl430_test.c

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    liblog \
    libhi_common \
    libhi_msp

LOCAL_STATIC_LIBRARIES := \
    libbsl430-clog \
    libpmrpc

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../include

LOCAL_MODULE := bsl430_test
LOCAL_32_BIT_ONLY := true

include $(BUILD_EXECUTABLE)
