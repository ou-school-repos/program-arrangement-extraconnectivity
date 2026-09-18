include ../../include/profile.mk
CPPFLAGS += -I/usr/include/nauty
LDLIBS += -lnauty
SOURCES := $(wildcard *.cpp)
BINS := $(SOURCES:%.cpp=$(ROOT)/bin/%)
.PHONY: all
all: $(BINS)
