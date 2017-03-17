#===============================================================================
#Copyright (c) 2012-2014 Qualcomm Connected Experiences, Inc. All Rights Reserved.
#
#Vuforia is a trademark of PTC Inc., registered in the United States and other 
#countries.
#==============================================================================


# An Android.mk file must begin with the definition of the LOCAL_PATH
# variable. It is used to locate source files in the development tree. Here
# the macro function 'my-dir' (provided by the build system) is used to return
# the path of the current directory.

LOCAL_PATH := $(call my-dir)

# The following section is used for copying the libVuforia.so prebuilt library
# into the appropriate folder (libs/armeabi and libs/armeabi-v7a respectively)
# and setting the include path for library-specific header files.

CVROOT := /home/al/Dropbox/Tese/MAVOAR/VOModule/opencv/native/jni

OPENCV_INSTALL_MODULES:=on
OPENCV_LIB_TYPE:=STATIC
include $(CVROOT)/OpenCV.mk
#-----------------------------------------------------------------------------

# The CLEAR_VARS variable is provided by the build system and points to a
# special GNU Makefile that will clear many LOCAL_XXX variables for you
# (e.g. LOCAL_MODULE, LOCAL_SRC_FILES, LOCAL_STATIC_LIBRARIES, etc...),
# with the exception of LOCAL_PATH. This is needed because all build
# control files are parsed in a single GNU Make execution context where
# all variables are global.

# The LOCAL_MODULE variable must be defined to identify each module you
# describe in your Android.mk. The name must be *unique* and not contain
# any spaces. Note that the build system will automatically add proper
# prefix and suffix to the corresponding generated file. In other words,
# a shared library module named 'foo' will generate 'libfoo.so'.

LOCAL_MODULE += visualodometry

# The list of additional linker flags to be used when building your
# module. Use the "-l" prefix in front of the name of libraries you want to
# link to your module.
LOCAL_CFLAGS += -std=c++11 -frtti -fexceptions -w
LOCAL_LDLIBS += -llog -landroid


# The list of shared libraries this module depends on at runtime.
# This information is used at link time to embed the corresponding information
# in the generated file. Here we reference the prebuilt library defined earlier
# in this makefile.

# The LOCAL_SRC_FILES variables must contain a list of C/C++ source files
# that will be built and assembled into a module. Note that you should not
# list header file and included files here because the build system will
# compute dependencies automatically for you, just list the source files
# that will be passed directly to a compiler.

LOCAL_SRC_FILES := mvo.cpp mvo_features.cpp


# By default, ARM target binaries will be generated in 'thumb' mode, where
# each instruction is 16-bit wide. You can set this variable to 'arm' to
# set the generation of the module's object files to 'arm' (32-bit
# instructions) mode, resulting in potentially faster yet somewhat larger
# binary code.

LOCAL_ARM_MODE := arm

# BUILD_SHARED_LIBRARY is a variable provided by the build system that
# points to a GNU Makefile script being in charge of collecting all the
# information you have defined in LOCAL_XXX variables since the latest
# 'include $(CLEAR_VARS)' statement, determining what and how to build.
# Replace it with the statement BUILD_STATIC_LIBRARY to generate a static
# library instead.

include $(BUILD_SHARED_LIBRARY)
