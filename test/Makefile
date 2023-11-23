CC         := clang
CXX        := clang++
CXXFLAGS   := -std=c++17
CLANGFLAGS := -c -emit-llvm -S -Xclang -disable-O0-optnone
PASSDIR    := ../lambdaize-loop

.PRECIOUS: %.ll %.obfuscated.ll

%.ll: %.c
	$(CC) $(CLANGFLAGS) -o $@ $^

%.ll: %.cpp
	$(CXX) $(CXXFLAGS) $(CLANGFLAGS) -o $@ $^

%.obfuscated.unlinked.ll: %.ll
	$(MAKE) -C $(PASSDIR)
	opt -S -load-pass-plugin $(PASSDIR)/lambdaize-loop.so -passes=lambdaize-loop -o $@ $^

%.obfuscated.ll: %.obfuscated.unlinked.ll
	$(MAKE) -C $(PASSDIR)/looper
	llvm-link -S -o $@ $^ $(PASSDIR)/looper/looper.bc

%.out: %.ll
	$(CXX) -o $@ $^

.PHONY: clean
clean:
	$(RM) *.ll *.out
