# Project Name
TARGET = snecho

# Sources
CPP_SOURCES += \
snecho.cpp \
dsp/snes.cpp \
dsp/psx.cpp \
dsp/resampler.cpp \

C_INCLUDES += \
-I. \
-Idsp \

# Library Locations
LIBDAISY_DIR = ../../libDaisy/
DAISYSP_DIR = ../../DaisySP/

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
