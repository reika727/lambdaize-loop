CXX                 := clang++
CXXFLAGS            := -std=c++17
MAX_RECURSION_COUNT ?= 512
CPPFLAGS            := -DMAX_RECURSION_COUNT=$(MAX_RECURSION_COUNT)
CLANGFLAGS          := -c -emit-llvm

%.bc: %.cpp
	$(CXX) $(CXXFLAGS) $(CLANGFLAGS) -o $@ $^

.PHONY: clean
clean:
	$(RM) *.bc
