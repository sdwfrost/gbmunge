# -*- mode: makefile-gmake -*-

OS = $(shell uname)
ARCH = $(shell uname -m)

USEGCC = 1
USECLANG = 0

CFLAGS-add += -Wall -Wextra -pedantic -O3

ifeq ($(USECLANG),1)
USEGCC = 0
CC = clang
endif

ifeq ($(USEGCC),1)
CC = gcc
CFLAGS-add += -D__USE_XOPEN -D_GNU_SOURCE
endif

