include ../../include/profile.mk
ORTOOLS_CFLAGS ?= $(shell pkg-config --cflags ortools 2>/dev/null)
ORTOOLS_LIBS ?= $(shell pkg-config --libs ortools 2>/dev/null)
ifneq ($(strip $(ORTOOLS_CFLAGS)$(ORTOOLS_LIBS)$(ORTOOLS_HOME)),)
CPPFLAGS += $(ORTOOLS_CFLAGS)
LDLIBS += $(ORTOOLS_LIBS)
SOURCES := $(wildcard *.cpp)
BINS := $(SOURCES:%.cpp=$(ROOT)/bin/%)
.PHONY: all
all: $(BINS)
else
.PHONY: all
all:
	@echo "Skipping src/ortools: OR-Tools not configured."
endif
