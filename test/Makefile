.PHONY: all
all:
	../src/fcode detokenize ffb.fcode | diff -w ffb.fth -
	../src/fcode detokenize sslx-cg6.fcode | diff -w sslx-cg6.fth -
	#../src/fcode detokenize test-pci.fcode | diff -w test-pci.fth -
	../src/fcode detokenize -3 xvr100.fcode | diff -w xvr100.fth -
