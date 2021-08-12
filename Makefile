# Technical prelude
SHELL := bash
.ONESHELL:
.SHELLFLAGS := -eu -o pipefail -c
.DELETE_ON_ERROR:
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules

BIN := mm
GCC ?= gcc
CFLAGS ?= -g -Og -Wall -std=c18
LDFLAGS ?= -g -Og -Wall -std=c18

.PHONY: all
all: $(BIN)

main.o: main.c
	$(GCC) -c $(CFLAGS) -o $@ $<

$(BIN): main.o
	$(GCC) $(LDFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm -f mm main.o
