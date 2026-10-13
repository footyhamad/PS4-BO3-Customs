# OpenOrbis Linux build for BO3-Customs.
# Produces a real PS4 PRX via the SDK's create-lib tool (not a renamed ELF).

OO_PS4_TOOLCHAIN ?= $(HOME)/OpenOrbis-PS4-Toolchain
CC       ?= clang
CXX      ?= clang++
LD       ?= ld.lld
OUT      ?= build
TARGET   := $(OUT)/BO3-Customs.prx
ELF      := $(OUT)/BO3-Customs.elf

CFLAGS   := --target=x86_64-pc-freebsd12-elf -DPS4 -fPIC -funwind-tables -c -isysroot $(OO_PS4_TOOLCHAIN) -isystem $(OO_PS4_TOOLCHAIN)/include -I.
CXXFLAGS := $(CFLAGS) -std=c++20 -isystem $(OO_PS4_TOOLCHAIN)/include/c++/v1
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
	$(CXX) $(CXXFLAGS) -o "$@" "$<"

$(OUT)/%.o: %.c | $(OUT)
	$(CC) $(CFLAGS) -o "$@" "$<"

$(ELF): $(OBJECTS)
	$(LD) $(OBJECTS) -o "$@" $(LDFLAGS)

$(TARGET): $(ELF)
	"$(OO_PS4_TOOLCHAIN)/bin/linux/create-lib" -in="$<" -out="$@"
	test -s "$@"
	file "$@"

clean:
	rm -rf "$(OUT)"
