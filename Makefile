XXD=xxd

ifneq ($(OS),Windows_NT)
	CC=clang
	LD=clang
	CCFLAGS=-std=c11 -O2 -g -Wall -Wextra -Wpedantic -Wstrict-aliasing
	CCFLAGS+=-Wno-pointer-arith -Wno-unused-parameter
	CCFLAGS+=-Wno-gnu-zero-variadic-macro-arguments
	LDFLAGS=

	EMULD=-lreadline -lsdl2 -lpthread
	ASMLD=
	TESTLD=
	
	EMU_WIN_SRC=
	EMU_WIN_OBJ=
	
	DIRCMD=mkdir -p bin
	CLEAN= clean_posix
else
	CC=gcc
	LD=gcc
	LIBPATH=C:/msys64/mingw64
	SDL2CFG=$(C:/msys64/mingw64/bin/sdl-config --cflags --libs)

	CCFLAGS=-std=c11 -O2 -g -Wall -Wextra -Wpedantic -Wstrict-aliasing
	CCFLAGS+=-I$(LIBPATH)/include/ -v -Wno-pointer-arith
	CCFLAGS+=-Wno-unused-parameter -Wno-gnu-zero-variadic-macro-arguments
	LDFLAGS=
	EMULD=-lsdl2main -lsdl2
	ASMLD=
	TESTLD=
	EMU_WIN_SRC=$(wildcard emu/win/*.c)
	EMU_WIN_OBJ=$(EMU_WIN_SRC:.c=.o)
	DIRCMD=if not exist bin mkdir bin
	CLEAN= clean_win
endif

BUILTIN_MACROS_ASM=asm/macros.asm
BUILTIN_MACROS_C=asm/builtin_macros.c

EMU_SRC=$(wildcard emu/*.c)
EMU_OBJ=$(EMU_SRC:.c=.o)

ASM_SRC=$(wildcard asm/*.c)
ASM_OBJ=$(ASM_SRC:.c=.o)

TEST_SRC=$(wildcard test/*.c)
TEST_OBJ=$(TEST_SRC:.c=.o)
TEST_OBJ+=emu/libemu.o

EMU=bin/emu
ASM=bin/asm
TEST=bin/test

all: $(CLEAN) emu asm test

clean_win:
	if exist bin rmdir /s /q bin
	
#TODO :: Make delete all files specified in .gitignore
	del /S /Q *.o *.exe *.dll

clean_posix:
	rm -f ./bin/*
	rmdir ./bin
	rm -f ./**/*.o

%.o: %.c
	$(CC) -o $@ -c $< $(CCFLAGS)

dirs:
	$(DIRCMD)

emu: dirs $(EMU_OBJ) $(EMU_WIN_OBJ)
	$(LD) -o $(EMU) $(filter %.o, $^) $(EMULD) $(LDFLAGS)

builtin_macros:
	$(XXD) -i $(BUILTIN_MACROS_ASM) > $(BUILTIN_MACROS_C)

asm: dirs builtin_macros $(ASM_OBJ)
	$(LD) -o $(ASM) $(ASMLD) $(LDFLAGS) $(filter %.o, $^) $(BUILTIN_MACROS_O)

test: dirs asm $(TEST_OBJ)
	$(LD) -o $(TEST) $(TESTLD) $(LDFLAGS) $(filter %.o, $^)

