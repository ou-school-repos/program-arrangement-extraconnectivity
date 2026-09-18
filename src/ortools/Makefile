include ../../include/profile.mk
ORTOOLS_CFLAGS ?= $(shell pkg-config --cflags ortools 2>/dev/null)
ORTOOLS_LIBS ?= $(shell pkg-config --libs ortools 2>/dev/null)
ifneq ($(strip $(ORTOOLS_CFLAGS)$(ORTOOLS_LIBS)$(ORTOOLS_HOME)),)
# Treat vendor headers as system headers so their extensions do not pollute
# diagnostics for project code (notably Abseil's __int128 declarations).
ORTOOLS_SYSTEM_CFLAGS := $(patsubst -I%,-isystem %,$(ORTOOLS_CFLAGS))
CPPFLAGS += $(ORTOOLS_SYSTEM_CFLAGS)
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
