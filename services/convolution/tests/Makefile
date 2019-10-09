all:
	../compiler/compiler transition.asm -elf transition_kernel.o
	g++ -O3 -g -std=c++11 -mavx2 transition.cpp ../dispatch.cpp ../misc.cpp transition_kernel.o -o transition
	./transition 1 100000
	#
	../compiler/compiler mat_mul.asm -elf mat_mul_kernel.o
	g++ -O3 -g -std=c++11 -mavx2 matrix_test.cpp ../dispatch.cpp ../misc.cpp mat_mul_kernel.o -o matrix_test
	./matrix_test 100000 1
	#
	../compiler/compiler save_restore.asm -elf save_restore_kernel.o
	g++ -O3 -g -std=c++11 -mavx2 save_restore_test.cpp ../dispatch.cpp ../misc.cpp save_restore_kernel.o -o save_restore
	./save_restore 1 100000
	#
	../compiler/compiler ref.asm -elf ref.o
