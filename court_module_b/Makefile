CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -I. -I../court_module_a

TEST_BIN := test_module_b

A_SRCS := ../court_module_a/CsvLoader.cpp ../court_module_a/AgentExtractor.cpp
B_SRCS := CaseIndex.cpp
TEST_SRCS := tests/TestModuleB.cpp

.PHONY: all check clean

all: $(TEST_BIN)

$(TEST_BIN): $(B_SRCS) $(A_SRCS) $(TEST_SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $(B_SRCS) $(A_SRCS) $(TEST_SRCS)

check: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f $(TEST_BIN) *.o
