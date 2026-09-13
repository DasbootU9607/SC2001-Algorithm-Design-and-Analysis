CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
CPPFLAGS += -Isrc
PYTHON ?= python3

APP_SOURCES = src/main.cpp src/experiments.cpp src/sorting.cpp
APP_HEADERS = src/experiments.h src/sorting.h

.PHONY: all test quick run check-results

all: build/project1

build:
	mkdir -p build

build/project1: $(APP_SOURCES) $(APP_HEADERS) Makefile | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(APP_SOURCES) -o $@

build/tests: tests/sorting_tests.cpp src/sorting.cpp src/sorting.h Makefile | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/sorting_tests.cpp src/sorting.cpp -o $@

test: build/tests
	./build/tests

quick: build/project1
	./build/project1 --quick

# Keep the published measurements in results/; save new full runs separately.
run: build/project1
	./build/project1 --output results/local

check-results:
	$(PYTHON) scripts/check_results.py results --full
