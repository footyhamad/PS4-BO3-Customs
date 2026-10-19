# OpenOrbis Linux build for BO3-Customs.
# Compiles against OpenOrbis headers/libraries, then emits a native PS4 PRX.

OO_PS4_TOOLCHAIN ?= $(HOME)/OpenOrbis-PS4-Toolchain
CC       := clang
CXX      := clang++
LD       := ld.lld
OUT      ?= build
TARGET   := $(OUT)/BO3-Customs.prx
ELF      := $(OUT)/BO3-Customs.elf

# OpenOrbis ships a FreeBSD-targeted libc++ sysroot. Include time.h explicitly
# so libc++'s pthread compatibility layer sees nanosleep under strict targets.
COMMON_FLAGS := --target=x86_64-pc-freebsd12-elf -DPS4 -D_POSIX_C_SOURCE=200809L -include time.h -fPIC -funwind-tables -isysroot $(OO_PS4_TOOLCHAIN) -isystem $(OO_PS4_TOOLCHAIN)/include -I.
CFLAGS   := $(COMMON_FLAGS) -c
CXXFLAGS := $(COMMON_FLAGS) -std=c++20 -isystem $(OO_PS4_TOOLCHAIN)/include/c++/v1
LIBS     := -lc -lc++ -lkernel -lSceSysmodule -lSceMouse -lSceUserService -lSceNet
GOLDHEN_SDK ?= $(HOME)/GoldHEN_Plugins_SDK

# Diagnostic build: link only the no-op entrypoint and the official-style CRT.
# This deliberately excludes every mod subsystem and its imported APIs.
LDFLAGS  = -m elf_x86_64 -pie --script $(OO_PS4_TOOLCHAIN)/link.x -e _init --eh-frame-hdr -L$(OO_PS4_TOOLCHAIN)/lib $(LIBS) $(GOLDHEN_SDK)/build/crtprx.o

CPP_SOURCES := $(wildcard *.cpp)
C_SOURCES   := $(wildcard *.c)
ifeq ($(BOOT_PROBE),1)
CXXFLAGS += -DBO3_CUSTOMS_BOOT_PROBE
CPP_SOURCES := main.cpp
C_SOURCES :=
LIBS := -lSceLibcInternal -lkernel -lSceSysmodule
endif
ifneq ($(filter-out 0,$(DIAG_STAGE)),)
CXXFLAGS += -DBO3_CUSTOMS_DIAG_STAGE=$(DIAG_STAGE)
endif
OBJECTS     := $(patsubst %.cpp,$(OUT)/%.o,$(CPP_SOURCES)) $(patsubst %.c,$(OUT)/%.o,$(C_SOURCES))

.PHONY: all clean
all: $(TARGET)

$(OUT):
	mkdir -p "$@"

$(OUT)/%.o: %.cpp | $(OUT)
	$(CXX) $(CXXFLAGS) -c -o "$@" "$<"

$(OUT)/%.o: %.c | $(OUT)
	$(CC) $(CFLAGS) -o "$@" "$<"

$(ELF): $(OBJECTS)
	$(LD) $(OBJECTS) -o "$@" $(LDFLAGS)

$(TARGET): $(ELF)
	OO_PS4_TOOLCHAIN="$(OO_PS4_TOOLCHAIN)" "$(OO_PS4_TOOLCHAIN)/bin/linux/create-fself" -in="$<" --out="$(OUT)/BO3-Customs.oelf" --lib="$@" --paid 0x3800000000000011
	test -s "$@"
	file "$@"

clean:
	rm -rf "$(OUT)"
