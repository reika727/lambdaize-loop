CXX                 := clang++
MAX_RECURSION_COUNT ?= 512
CPPFLAGS            := -DMAX_RECURSION_COUNT=$(MAX_RECURSION_COUNT)
CXXFLAGS            := -std=c++17
SRC                 := looper.cpp
TARGET              := looper.bc

$(TARGET): $(SRC)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -emit-llvm -Xclang -disable-O0-optnone -o $@ $^

.PHONY: clean
clean:
	$(RM) $(TARGET)
