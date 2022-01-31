CXX      := clang++
CXXFLAGS := $(shell llvm-config --cxxflags) -std=c++17 -Wall -Wextra
LDFLAGS  := $(shell llvm-config --ldflags)
SRC      := lambdaize-loop.cpp
TARGET   := lambdaize-loop.so
PCH      := includes.pch

$(TARGET): $(SRC) $(PCH)
	$(CXX) $(CXXFLAGS) -include-pch $(PCH) -shared -fPIC $(LDFLAGS) -o $@ $<

%.pch: %.hpp
	$(CXX) $(CXXFLAGS) -fPIC -o $@ $^

.PHONY: format
format:
	clang-format -i *.cpp *.hpp

.PHONY: clean
clean:
	$(RM) *.pch *.so
	$(MAKE) -C looper $@
	$(MAKE) -C test $@
