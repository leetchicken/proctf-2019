[BITS 64]

SECTION .text
global _start		; make label available to linker
_start:
  mov rbp,rsp
  sub rbp, 256

  mov rbx, [rsp]
  cmp rbx, 2
  jne err1

  mov rdi, [rsp+8+8]

  call rel1
rel1:
  pop rbx
  add rbx,payload_data-rel1
  mov rcx, ENCODED_DATA_LEN
lp_arith_1:
  mov edx, [rbx]
  DECODER_CODE
  mov [rbx],edx
  add rbx, 4
  dec ecx
  jnz lp_arith_1
; Start of obfuscation
payload_data  dd PAYLOAD_ENCODED
; end of obfustation
  call rel2
rel2:
  pop rbx
  sub rbx,rel2-payload_data
  mov rcx, ENCODED_DATA_LEN
lp_arith_2:
  mov edx, [rbx]
  ENCODER_CODE
  mov [rbx],edx
  add rbx, 4
  dec ecx
  jnz lp_arith_2

  mov rsi, rbp
  mov rdx, r10
  mov eax, 1
  mov rdi, 1
  syscall
  jmp exit

err1:
  mov rsi,err_mes_1
  mov rdx,err_mes_2-err_mes_1
  mov eax, 1
  mov rdi,1
  syscall
  jmp exit
err2:
  mov rsi,err_mes_2
  mov rdx,err_mes_3-err_mes_2
  mov eax, 1
  mov rdi,1
  syscall
  jmp exit
exit:
  mov rax, 60
  mov rdi,0
  syscall
buf:  TIMES 64 db 0
err_mes_1  db "Not enough args",13,10,0
err_mes_2  db "Invalid key",13,10,0
err_mes_3  db 10,0
