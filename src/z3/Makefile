include ../../include/profile.mk
Z3_CFLAGS ?= $(shell pkg-config --cflags z3 2>/dev/null)
Z3_LIBS ?= $(shell pkg-config --libs z3 2>/dev/null)
ifneq ($(strip $(Z3_CFLAGS)$(Z3_LIBS)),)
CPPFLAGS += $(Z3_CFLAGS)
LDLIBS += $(Z3_LIBS)
SOURCES := $(wildcard *.cpp)
BINS := $(SOURCES:%.cpp=$(ROOT)/bin/%)
.PHONY: all
all: $(BINS)
else
.PHONY: all
all:
	@echo "Skipping src/z3: Z3 not configured."
endif
