CXX ?= c++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
CPPFLAGS += -I.

MODULE_SOURCES := CsvLoader.cpp AgentExtractor.cpp
MODULE_HEADERS := CaseDocument.h CsvLoader.h AgentExtractor.h

.PHONY: all check clean
all: build/module_a_demo

build/module_a_demo: $(MODULE_SOURCES) $(MODULE_HEADERS) examples/Demo.cpp
	mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(MODULE_SOURCES) examples/Demo.cpp -o $@

build/module_a_tests: $(MODULE_SOURCES) $(MODULE_HEADERS) tests/TestModuleA.cpp
	mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(MODULE_SOURCES) tests/TestModuleA.cpp -o $@

check: build/module_a_tests
	./build/module_a_tests examples/sample.csv

clean:
	rm -f build/module_a_demo build/module_a_tests
