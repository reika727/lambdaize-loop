CC       := clang
CXX      := clang++
CXXFLAGS := $(shell llvm-config --cxxflags) -std=c++17 -Wall -Wextra
LDFLAGS  := $(shell llvm-config --ldflags)
TARGET   := obfuscated.ll
TBO      := to-be-obfuscated.ll
PCH      := includes.pch
PASS.SO  := lambdaize-loop.so
PASS     := lambdaize-loop

.PRECIOUS: %.pch

.PHONY: $(TARGET)
$(TARGET): $(TBO) $(PASS.SO)
	opt -S -load-pass-plugin ./$(PASS.SO) -passes=$(PASS) -o $@ $(TBO)

%.ll: %.c
	$(CC) -S -emit-llvm -Xclang -disable-O0-optnone -o $@ $^

%.so: %.cpp $(PCH)
	$(CXX) $(CXXFLAGS) -include-pch $(PCH) -shared -fPIC $(LDFLAGS) -o $@ $<

%.pch: %.hpp
	$(CXX) $(CXXFLAGS) -fPIC -o $@ $^

.PHONY: format
format:
	clang-format -i *.c *.cpp *.hpp

.PHONY: clean
clean:
	$(RM) *.ll *.pch *.so
