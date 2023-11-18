CXX      := clang++
CXXFLAGS := $(shell llvm-config --cxxflags)
LDFLAGS  := $(shell llvm-config --ldflags)
SRC      := lambdaize-loop.cpp
TARGET   := lambdaize-loop.so
PCH      := includes.pch

$(TARGET): $(SRC) $(PCH)
	$(CXX) $(CXXFLAGS) -Wall -Wextra -include-pch $(PCH) -shared -fPIC $(LDFLAGS) -o $@ $<

%.pch: %.hpp
	$(CXX) $(CXXFLAGS) -Wno-everything -fPIC -o $@ $^

.PHONY: format
format:
	clang-format -i *.cpp *.hpp

.PHONY: clean
clean:
	$(RM) *.pch *.so
	$(MAKE) -C looper $@
