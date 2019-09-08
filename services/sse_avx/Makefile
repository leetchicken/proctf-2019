.PHONY: debug release clean

debug:
	g++ -O0 -g -std=c++11 main.cpp -DDEBUG=1 -o sse-avx-dbg
	
release:
	g++ -O3 -g -std=c++11 main.cpp -o sse-avx
