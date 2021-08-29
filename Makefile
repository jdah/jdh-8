XXD=xxd

ifneq ($(OS),Windows_NT)
	CC=clang
	LD=clang
	CCFLAGS=-std=c11 -O0 -g -Wall -Wextra -Wpedantic -Wstrict-aliasing
	CCFLAGS+=-Wno-pointer-arith -Wno-unused-parameter
	CCFLAGS+=-Wno-gnu-zero-variadic-macro-arguments
	LDFLAGS=

	EMULD=-lreadline -lsdl2 -lpthread
	ASMLD=
	TESTLD=
	DIRCMD=mkdir -p bin
else
	CC=gcc
	LD=gcc
	LIBPATH=C:/cdevlibs

	CCFLAGS=-std=c11 -O0 -g -Wall -Wextra -Wpedantic -Wstrict-aliasing
	CCFLAGS+=-I$(LIBPATH)/include/ -v -Wno-pointer-arith
	CCFLAGS+=-Wno-unused-parameter -Wno-gnu-zero-variadic-macro-arguments
	LDFLAGS=

	EMULD=-L$(LIBPATH)/lib -lmingw32 -lsdl2main -lsdl2 -lws2_32
	ASMLD=
	TESTLD=

	DIRCMD=if not exist bin mkdir bin
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

all: emu asm test

clean:
	rm -f ./bin/*
	rmdir ./bin
	rm -f ./**/*.o

%.o: %.c
	$(CC) -o $@ -c $< $(CCFLAGS)

dirs:
	$(DIRCMD)

emu: dirs $(EMU_OBJ)
	$(LD) -o $(EMU) $(EMULD) $(LDFLAGS) $(filter %.o, $^)

builtin_macros:
	$(XXD) -i $(BUILTIN_MACROS_ASM) > $(BUILTIN_MACROS_C)

asm: dirs builtin_macros $(ASM_OBJ)
	$(LD) -o $(ASM) $(ASMLD) $(LDFLAGS) $(filter %.o, $^) $(BUILTIN_MACROS_O)

test: dirs asm $(TEST_OBJ)
	$(LD) -o $(TEST) $(TESTLD) $(LDFLAGS) $(filter %.o, $^)

