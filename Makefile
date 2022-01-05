XXD=xxd

ifneq ($(OS),Windows_NT)
	ARCH_LINUX := $(shell grep "Arch Linux" /etc/os-release 2>/dev/null)

	ifeq ($(ARCH_LINUX),)
		SDL_NAME = sdl
	else
		SDL_NAME = SDL
	endif

	CC=clang
	LD=clang
	CCFLAGS  = -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Wstrict-aliasing
	CCFLAGS += -Wno-pointer-arith -Wno-unused-parameter
	CCFLAGS += -Wno-gnu-zero-variadic-macro-arguments -Wno-gnu-statement-expression
	CCFLAGS += $(shell sdl2-config --cflags)
	LDFLAGS  =

	EMULD=-lreadline -lpthread $(shell sdl2-config --libs)
	ASMLD=
	TESTLD=

	EMU_WIN_SRC=
	EMU_WIN_OBJ=

	DIRCMD=mkdir -p bin
	CLEAN=clean_posix
else
	CC=gcc
	LD=gcc
	LIBPATH=C:/msys64/mingw64
	SDL2CFG=$(C:/msys64/mingw64/bin/sdl-config --cflags --libs)

	CCFLAGS  = -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Wstrict-aliasing
	CCFLAGS += -I$(LIBPATH)/include/ -v -Wno-pointer-arith
	CCFLAGS += -Wno-unused-parameter -Wno-gnu-zero-variadic-macro-arguments
	CCFLAGS += -Wno-gnu-statement-expression
	LDFLAGS  =
	EMULD    = -lsdl2main -lsdl2
	ASMLD    =
	TESTLD   =
	EMU_WIN_SRC = $(wildcard emu/win/*.c)
	EMU_WIN_OBJ = $(EMU_WIN_SRC:.c=.o)
	DIRCMD 			= if not exist bin mkdir bin
	CLEAN 			=	clean_win
endif

BUILTIN_MACROS_ASM = asm/macros.asm
BUILTIN_MACROS_C   = asm/builtin_macros.c
BUILTIN_MACROS_O   = $(BUILTIN_MACROS_C:.c=.o)

EMU_SRC  = $(wildcard emu/*.c)
EMU_OBJ  = $(EMU_SRC:.c=.o)
EMU_OBJ += common/jdh8util.o

ASM_SRC = $(wildcard asm/*.c)
ASM_OBJ = $(ASM_SRC:.c=.o)

TEST_SRC  = $(wildcard test/*.c)
TEST_OBJ  = $(TEST_SRC:.c=.o)
TEST_OBJ += emu/libemu.o

EMU 	= bin/emu
ASM   = bin/asm
TEST  = bin/test

all: emu asm test

clean: $(CLEAN)

clean_win:
	if exist bin rmdir /s /q bin
	del /S /Q *.o *.exe *.dll
	del /S /Q $(BUILTIN_MACROS_C)

clean_posix:
	rm -f $(BUILTIN_MACROS_C)
	rm -rf ./bin
	rm -f ./**/*.o

%.o: %.c
	$(CC) -o $@ -c $< $(CCFLAGS)

dirs:
	$(DIRCMD)

emu: dirs $(EMU_OBJ) $(EMU_WIN_OBJ)
	$(LD) -o $(EMU) $(filter %.o, $^) $(EMULD) $(LDFLAGS)

builtin_macros:
	$(XXD) -i $(BUILTIN_MACROS_ASM) > $(BUILTIN_MACROS_C)
	$(CC) -o $(BUILTIN_MACROS_O) -c $(BUILTIN_MACROS_C) $(CCFLAGS)

asm: dirs builtin_macros $(ASM_OBJ)
	$(LD) -o $(ASM) $(ASMLD) $(LDFLAGS) $(filter %.o, $^) $(BUILTIN_MACROS_O)

test: dirs asm $(TEST_OBJ)
	$(LD) -o $(TEST) $(TESTLD) $(LDFLAGS) $(filter %.o, $^)

