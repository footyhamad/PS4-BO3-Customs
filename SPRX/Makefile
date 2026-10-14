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
LIBS     := -lc -lc++ -lkernel -lSceSysmodule_stub_weak -lSceMouse_stub_weak -lSceUserService_stub_weak -lSceNet_stub_weak
LDFLAGS  := -m elf_x86_64 -pie --script $(OO_PS4_TOOLCHAIN)/link.x --eh-frame-hdr -L$(OO_PS4_TOOLCHAIN)/lib -L./lib $(LIBS) $(OO_PS4_TOOLCHAIN)/lib/crtlib.o ./lib/libkernel_gen_stub_weak.a ./lib/libkernel_unity_stub_weak.a

CPP_SOURCES := $(wildcard *.cpp)
C_SOURCES   := $(wildcard *.c)
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
