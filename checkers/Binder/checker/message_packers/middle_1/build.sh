
nasm -f elf64 $1 -o $2.tmp.o
ld --omagic  $2.tmp.o -o $2.tmp
objcopy --set-section-flags .code=contents,alloc,load,code $2.tmp $2
strip -S -g -M -x  $2
chmod +x $2
