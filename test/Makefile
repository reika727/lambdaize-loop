CC         := clang
CXX        := clang++
CXXFLAGS   := -std=c++17
CLANGFLAGS := -c -emit-llvm -Xclang -disable-O0-optnone

.PRECIOUS: %.bc %.optimized.bc

%.bc: %.c
	$(CC) $(CLANGFLAGS) -o $@ $^

%.bc: %.cpp
	$(CXX) $(CXXFLAGS) $(CLANGFLAGS) -o $@ $^

%.obfuscated.unlinked.bc: %.bc
	$(MAKE) -C ..
	opt -load-pass-plugin ../lambdaize-loop.so -passes=lambdaize-loop -o $@ $^

%.obfuscated.bc: %.obfuscated.unlinked.bc
	$(MAKE) -C ../library looper.bc
	llvm-link -o $@ $^ ../library/looper.bc

%.optimized.bc: %.bc
	opt -O3 -o $@ $^

%.out: %.optimized.bc
	$(CXX) -o $@ $^

.PHONY: clean
clean:
	$(RM) *.bc *.out
