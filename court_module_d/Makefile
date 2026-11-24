CXX ?= c++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
A_DIR := ../court_module_a
CPPFLAGS += -I. -I$(A_DIR)

SOURCES := CaseAnalysis.cpp $(A_DIR)/CsvLoader.cpp $(A_DIR)/AgentExtractor.cpp
HEADERS := CaseAnalysis.h $(A_DIR)/CaseDocument.h $(A_DIR)/CsvLoader.h $(A_DIR)/AgentExtractor.h

.PHONY: all check clean
all: build/module_d_tests

build/module_d_tests: $(SOURCES) $(HEADERS) tests/TestModuleD.cpp
	mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SOURCES) tests/TestModuleD.cpp -o $@

check: build/module_d_tests
	./build/module_d_tests $(A_DIR)/examples/sample.csv

clean:
	rm -f build/module_d_tests
