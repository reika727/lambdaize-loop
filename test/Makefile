CC         := clang
CXX        := clang++
CXXFLAGS   := -std=c++17
CLANGFLAGS := -emit-llvm -Xclang -disable-O0-optnone

.PRECIOUS: %.bc %.obfuscated.bc

%.bc: %.c
	$(CC) -c $(CLANGFLAGS) -o $@ $^

%.bc: %.cpp
	$(CXX) $(CXXFLAGS) -c $(CLANGFLAGS) -o $@ $^

%.obfuscated.unlinked.bc: %.bc
	$(MAKE) -C ..
	opt -load-pass-plugin ../lambdaize-loop.so -passes=lambdaize-loop -o $@ $^

%.obfuscated.bc: %.obfuscated.unlinked.bc
	$(MAKE) -C ../looper looper.bc
	llvm-link -o $@ $^ ../looper/looper.bc

%.out: %.bc
	$(CXX) -o $@ $^

.PHONY: clean
clean:
	$(RM) *.bc *.out
