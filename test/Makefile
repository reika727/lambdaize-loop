CC         := clang
CXX        := clang++
CXXFLAGS   := -std=c++17
CLANGFLAGS := -S -emit-llvm -Xclang -disable-O0-optnone

.PRECIOUS: %.ll %.obfuscated.ll

%.ll: %.c
	$(CC) $(CLANGFLAGS) -o $@ $^

%.ll: %.cpp
	$(CXX) $(CXXFLAGS) $(CLANGFLAGS) -o $@ $^

%.obfuscated.ll: %.ll
	$(MAKE) -C ..
	opt -S -load-pass-plugin ../lambdaize-loop.so -passes=lambdaize-loop -o $@ $^

%.out: %.ll
	$(CXX) -o $@ $^

%.obfuscated.out: %.obfuscated.ll
	$(MAKE) -C ../library looper.o
	$(CXX) -o $@ $^ ../library/looper.o

.PHONY: clean
clean:
	$(RM) *.ll *.out
