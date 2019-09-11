# Binder Service Description

This service is intended for storing secret messages. You can look at it using "/list" handler. To upload secret message you can upload some file which gets the argument with password and prints some message (or Invalid key). Each message elf-file runs under the seccomp wrapper. So some system call restrictions are applied. To fit these restrictions you must set the "-static" option. Using "/upload" handler you can upload a message file to the service. Using "/run" handler you can view message file. Using "/get" you can get file with message.

# Checker Algorithm

1. check. To check that service works it gets the "/list" page and verifies that header of page is presented
2. put. To put flag to service it generates some elf-file which receives some pregenerated password in argument and prints flag. Then it receives the message id and verifies that it is presented on "/list" page. Checker prints generated password for message, message id (received from service) and index of elf-file generator.
3. get. To get flag it verifies that message id is presented on "/list" page. Then it executes "/run" handler with password and verifies that service prints flag correctly. Then it downloads elf file, runs under seccomp wrapper and verifies that flag is presented in output

# Intended Way to Hack

To get flag of another team you must download elf-file with message and reverse it. Currently there is single elf-file generator is presented on checker. Several complex elf-file generators will be presented.
To get flags from several commands you must write automatic reverse script (using ida,radare2 or gdb scripts).

# Intended Way to Protect

To protect service you must write you own protector of message files. You can simply pack it using upx or another packer. Or you can reverse file and pack this source code using more complex tools. To get your flag another team must reverse your file.

# Message packing

Message packing algorithm is the code of checker, which takes flag and encapsulates it to some ELF-file. This file uploads to team's servers at 'put' operation of checker and verifies at 'get' operation of checker. It gives flag only in case of valid password in arguments of ELF-file. There are two message packers are presented

## Simple packer

Program to create ELF-file is written on C and compiled with gcc. The algorithm of password verification takes the password P and xors it with some initial 4-byte number I with next algorithm:

1. Take first byte of I and xor it with first byte of P
2. Rotate right I
3. Take second byte of I and xor it with first byte of P
4. Continue it for each symbol of P

If resulting I equals to some constant then this constant is used to decrypt message with flag using 4-byte xor algorithm. 

To extract message with flag participant must write some program which takes message and resulting key to decrypt it.

## Middle packer

Program to create ELF-file is written on intel x64 assembler and compiled with nasm. The algorithm of password verification is same as in previous packer, but the password verification and message decryption code is packed into some payload. This payload decrypts during program execution with some polymorfic arithmetic decoder. So the program looks like this:

Decoder 1
Decoder 2
...
Decoder N
Payload
verification and message decryption code 
Encoder N
...
Encoder 2
Encoder 1

Each decoder K decodes the underlying payload till encoder K. Decoder and encoder looks like a cycle with several arithmetic operations under every byte. 

To make the task harder all symbols removed with strip command. To hack each message with this packer participant must write program which unpacks verification and message decryption code and extracts mesage and key to decrypt it.
