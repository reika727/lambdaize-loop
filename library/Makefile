CXX                 := clang++
MAX_RECURSION_COUNT ?= 512
CPPFLAGS            := -DMAX_RECURSION_COUNT=$(MAX_RECURSION_COUNT)
CXXFLAGS            := -std=c++17

%.bc: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -emit-llvm -o $@ $^

.PHONY: clean
clean:
	$(RM) *.bc
