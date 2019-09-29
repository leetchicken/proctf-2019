.PHONY: debug release clean

all:
	compiler/compiler convolution.asm convolution_kernel.o
	g++ -O3 -g -std=c++11 -mavx2 main.cpp dispatch.cpp httpserver.cpp log.cpp png.cpp convolution_kernel.o -lmicrohttpd -o convolution

debug: all

tests:
	compiler/compiler test/transition.asm transition_kernel.o
	g++ -O3 -g -std=c++11 -mavx2 transition.cpp dispatch.cpp transition_kernel.o -o transition
	#
	compiler/compiler test/mat_mul.asm mat_mul_kernel.o
	g++ -O3 -g -std=c++11 -mavx2 matrix_test.cpp dispatch.cpp mat_mul_kernel.o -o matrix_test
	#
	compiler/compiler test/save_restore.asm save_restore_kernel.o
	g++ -O3 -g -std=c++11 -mavx2 save_restore_test.cpp dispatch.cpp save_restore_kernel.o -o save_restore
