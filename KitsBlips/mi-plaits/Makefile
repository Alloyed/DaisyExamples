# Project Name
TARGET = plaits

# Needed because we are overbudget
APP_TYPE = BOOT_SRAM

# Library Locations
LIBDAISY_DIR = ../../libDaisy/
#DAISYSP_DIR = ../../DaisySP/
STMLIB_DIR = ../../stmlib

# Can't actually add to CFLAGS.. due to libDaisy stuff
# silences warning from stmlib JOIN macros
C_INCLUDES += -Wno-unused-local-typedefs
OPT = -Os

# Sources
CPP_SOURCES = plaits.cpp

CC_SOURCES += $(STMLIB_DIR)/dsp/units.cc \
$(STMLIB_DIR)/utils/random.cc \
resources.cc \
dsp/voice.cc \
dsp/chords/chord_bank.cc \
dsp/engine/additive_engine.cc \
dsp/engine/bass_drum_engine.cc \
dsp/engine/chord_engine.cc \
dsp/engine/fm_engine.cc \
dsp/engine/grain_engine.cc \
dsp/engine/hi_hat_engine.cc \
dsp/engine/modal_engine.cc \
dsp/engine/noise_engine.cc \
dsp/engine/particle_engine.cc \
dsp/engine/snare_drum_engine.cc \
dsp/engine/speech_engine.cc \
dsp/engine/string_engine.cc \
dsp/engine/swarm_engine.cc \
dsp/engine/virtual_analog_engine.cc \
dsp/engine/waveshaping_engine.cc \
dsp/engine/wavetable_engine.cc \
dsp/engine2/chiptune_engine.cc \
dsp/engine2/phase_distortion_engine.cc \
dsp/engine2/six_op_engine.cc \
dsp/engine2/string_machine_engine.cc \
dsp/engine2/virtual_analog_vcf_engine.cc \
dsp/engine2/wave_terrain_engine.cc \
dsp/fm/algorithms.cc \
dsp/fm/dx_units.cc \
dsp/physical_modelling/modal_voice.cc \
dsp/physical_modelling/resonator.cc \
dsp/physical_modelling/string_voice.cc \
dsp/physical_modelling/string.cc \
dsp/speech/lpc_speech_synth_controller.cc \
dsp/speech/lpc_speech_synth_phonemes.cc \
dsp/speech/lpc_speech_synth_words.cc \
dsp/speech/lpc_speech_synth.cc \
dsp/speech/naive_speech_synth.cc \
dsp/speech/sam_speech_synth.cc \

C_INCLUDES += \
-I. \
-Idsp \
-Idsp/pvoc \
-Idsp/fx \
-Ibootloader \
-Iresources \
-I../.. \
-I.. \

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

### Need to override all to get support for .cc files
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

#######################################
# build the application
#######################################
# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(CPP_SOURCES:.cpp=.o)))
vpath %.cpp $(sort $(dir $(CPP_SOURCES)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(CC_SOURCES:.cc=.o)))
vpath %.cc $(sort $(dir $(CC_SOURCES)))
# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $(C_STANDARD) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.cpp Makefile | $(BUILD_DIR)
	$(CXX) -c $(CPPFLAGS) $(CPP_STANDARD) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.cpp=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.cc Makefile | $(BUILD_DIR)
	$(CXX) -c $(CPPFLAGS) $(CPP_STANDARD) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.cc=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@

$(BUILD_DIR):
	mkdir $@
