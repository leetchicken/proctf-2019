gcc wrapper.cpp -o wrapper -lseccomp
gcc fs_access.cpp -o fs_access.elf -static
gcc exec_ls.cpp -o exec_ls.elf -static
gcc print_access.cpp -o print_access.elf -static
gcc fork_test.cpp -o fork_test.elf -static
