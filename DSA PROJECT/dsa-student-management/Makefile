CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O2
TARGET    = student_management
SOURCES   = student_management.cpp

.PHONY: all clean run test

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	python tests/run_tests.py

clean:
	rm -f $(TARGET) $(TARGET).exe students.csv.tmp
