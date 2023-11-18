CXX      := clang++
CXXFLAGS := $(shell llvm-config --cxxflags) -Wall -Wextra
LDFLAGS  := $(shell llvm-config --ldflags)
SRC      := count-cyclomatic-complexity.cpp
TARGET   := count-cyclomatic-complexity.so

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -shared -fPIC $(LDFLAGS) -o $@ $<

.PHONY: format
format:
	clang-format -i *.cpp

.PHONY: clean
clean:
	$(RM) $(TARGET)
