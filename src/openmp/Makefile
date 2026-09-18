include ../../include/profile.mk
CXXFLAGS += -fopenmp
SOURCES := $(wildcard *.cpp)
BINS := $(SOURCES:%.cpp=$(ROOT)/bin/%)
.PHONY: all
all: $(BINS)
