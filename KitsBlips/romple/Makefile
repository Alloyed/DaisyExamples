# Project Name
TARGET = romple

# Enable FatFS, for reading from SD card
USE_FATFS = 1
# store program in SRAM instead of flash memory. uses the Daisy bootloader
#APP_TYPE = BOOT_SRAM

# Sources
CPP_SOURCES = romple.cpp waveloader.cpp sfzloader.cpp

# Library Locations
LIBDAISY_DIR = ../../libDaisy/
DAISYSP_DIR = ../../DaisySP/

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
