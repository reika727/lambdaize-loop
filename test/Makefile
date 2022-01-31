CC         := clang
CXX        := clang++
CXXFLAGS   := -std=c++17
CLANGFLAGS := -emit-llvm -Xclang -disable-O0-optnone
PASSDIR    := ../lambdaize-loop

.PRECIOUS: %.bc %.obfuscated.bc

%.bc: %.c
	$(CC) -c $(CLANGFLAGS) -o $@ $^

%.bc: %.cpp
	$(CXX) $(CXXFLAGS) -c $(CLANGFLAGS) -o $@ $^

%.obfuscated.unlinked.bc: %.bc
	$(MAKE) -C $(PASSDIR)
	opt -load-pass-plugin $(PASSDIR)/lambdaize-loop.so -passes=lambdaize-loop -o $@ $^

%.obfuscated.bc: %.obfuscated.unlinked.bc
	$(MAKE) -C $(PASSDIR)/looper
	llvm-link -o $@ $^ $(PASSDIR)/looper/looper.bc

%.out: %.bc
	$(CXX) -o $@ $^

.PHONY: clean
clean:
	$(RM) *.bc *.out
