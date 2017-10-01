# -*- mode: makefile-gmake -*-

OS = $(shell uname)
ARCH = $(shell uname -m)

USEGCC = 1
USECLANG = 0

CFLAGS-add += -Wall -Wextra -pedantic -O3 -ffast-math

ifeq ($(USECLANG),1)
USEGCC = 0
CC = clang
endif

ifeq ($(USEGCC),1)
CC = gcc
endif

ifeq ($(OS), Linux)
SHLIB_EXT = so
CFLAGS-add+=-fPIC
FFLAGS-add+=-fPIC
endif

ifeq ($(OS), FreeBSD)
SHLIB_EXT = so
CFLAGS-add+=-fPIC
FFLAGS-add+=-fPIC
endif

ifeq ($(OS), OpenBSD)
SHLIB_EXT = so
CFLAGS-add+=-fPIC
FFLAGS-add+=-fPIC
endif

ifeq ($(OS), Darwin)
SHLIB_EXT = dylib
CFLAGS-add+=-fPIC
FFLAGS-add+=-fPIC
endif

ifeq ($(OS), WINNT)
SHLIB_EXT = dll
endif

ifeq ($(OS), MINGW64_NT-10.0)
SHLIB_EXT = dll
endif

ifeq ($(OS), MINGW32_NT-10.0-WOW)
SHLIB_EXT = dll
endif

# SunOS with gcc
ifeq ($(OS), SunOS)
SHLIB_EXT = so
CFLAGS-add+=-fPIC
FFLAGS-add+=-fPIC
endif
