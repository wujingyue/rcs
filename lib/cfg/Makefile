LEVEL = ../..

LIBRARYNAME = cfg

LOADABLE_MODULE = 1
BUILD_ARCHIVE = 1

SOURCES = callgraph-fp.cpp cfg.cpp exec.cpp exec-once.cpp fp-instrumenter.cpp \
	  icfg-builder.cpp icfg.cpp identify-thread-funcs.cpp fp-collector.cpp \
	  inter-reach.cpp intra-reach.cpp mbb.cpp partial-icfg-builder.cpp

all:: fp-tracing.bc

fp-tracing.bc: fp-tracing.cpp
	clang++ $< -o $@ -c -emit-llvm

include $(LEVEL)/Makefile.common
