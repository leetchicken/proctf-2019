# Binder service description

This service is intended for storing the secret messages. You can look at it using "/list" handler. To upload secret message you can upload some file which receives the argument with password and prints some message (or Invalid key). Each message elf-file runs under seccomp wrapper. So some system call restrictions are presented. To fit this restrictions you must set "-static" option. Using "/upload" handler you can upload message file to service. Using "/run" handler you can view message file. Using "/get" you can get file with message.

# Checker algorithm

1. check. To check that service works it gets the "/list" page and verifies that header of page is presented
2. put. To put flag to service it generates some elf-file which receives some pregenerated password in argument and prints flag. Then it receives the message id and verifies that it is presented on "/list" page. Checker prints generated password for message, message id (received from service) and index of elf-file generator.
3. get. To get flag it verifies that message id is presented on "/list" page. Then it executes "/run" handler with password and verifies that service prints flag correctly. Then it downloads elf file, runs under seccomp wrapper and verifies that flag is presented in output

# Intended way to hack

To get flag of another team you must download elf-file with message and reverse it. Currently there is single elf-file generator is presented on checker. Several complex elf-file generators will be presented.
To get flags from several commands you must write automatic reverse script (using ida,radare2 or gdb scripts).

# Intended way to protect

To protect service you must write you own protector of message files. You can simply pack it using upx or another packer. Or you can reverse file and pack this source code using more complex tools. To get your flag another team must reverse your file.

# TODO

1. Proof-of-work to protect from dos
2. 3-4 more complex elf-file packers (tigress, ollvm or manual)
3. Frontend design
4. Dockerfile