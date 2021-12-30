CC         := clang
CXX        := clang++
CXXFLAGS   := -std=c++17
CLANGFLAGS := -c -emit-llvm -Xclang -disable-O0-optnone

.PRECIOUS: %.bc %.obfuscated.bc

%.bc: %.c
	$(CC) $(CLANGFLAGS) -o $@ $^

%.bc: %.cpp
	$(CXX) $(CXXFLAGS) $(CLANGFLAGS) -o $@ $^

%.obfuscated.bc.unlinked: %.bc
	$(MAKE) -C ..
	opt -load-pass-plugin ../lambdaize-loop.so -passes=lambdaize-loop -o $@ $^

%.obfuscated.bc: %.obfuscated.bc.unlinked
	$(MAKE) -C ../library looper.bc
	llvm-link -o $@ $^ ../library/looper.bc

%.out: %.bc
	$(CXX) -o $@ $^

.PHONY: clean
clean:
	$(RM) *.bc *.out
