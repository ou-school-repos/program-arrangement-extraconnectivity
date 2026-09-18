ROOT := ../..
CXX ?= g++
CXXFLAGS ?= -std=c++17 -O3 -march=native -Wall -Wextra -Wpedantic
BUILD_VERSION ?= 0.1.0 ($(shell git -C $(ROOT) rev-parse --short=12 HEAD 2>/dev/null || echo unknown))
CPPFLAGS += -I$(ROOT)/include -DBUILD_VERSION=\"$(BUILD_VERSION)\"
COMMON_HEADERS := $(wildcard $(ROOT)/include/*.hpp)

$(ROOT)/bin/%: %.cpp $(COMMON_HEADERS)
	@mkdir -p $(ROOT)/bin
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $< $(LDFLAGS) $(LDLIBS)
