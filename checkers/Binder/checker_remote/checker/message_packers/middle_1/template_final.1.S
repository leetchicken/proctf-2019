[BITS 64]

SECTION .text
global _start		; make label available to linker
%include "message.inc"
_start:
; Start of obfuscation
  mov INDEX, 0
  mov SECOND_KEY_REG, START_KEY
lp1:
  mov KEY_BYTE, byte [rdi + INDEX]
  inc INDEX

  xor SECOND_KEY_REG_BYTE,KEY_BYTE
  ror SECOND_KEY_REG,8
  cmp byte [rdi + INDEX], 0
  jnz lp1

  cmp SECOND_KEY_REG, SECOND_KEY
  jnz noo

  %ifdef MESSAGE_0
    mov dword [rbp+0],MESSAGE_0
%endif
%ifdef MESSAGE_1
    mov dword [rbp+4],MESSAGE_1
%endif
%ifdef MESSAGE_2
    mov dword [rbp+8],MESSAGE_2
%endif
%ifdef MESSAGE_3
    mov dword [rbp+12],MESSAGE_3
%endif
%ifdef MESSAGE_4
    mov dword [rbp+16],MESSAGE_4
%endif
%ifdef MESSAGE_5
    mov dword [rbp+20],MESSAGE_5
%endif
%ifdef MESSAGE_6
    mov dword [rbp+24],MESSAGE_6
%endif
%ifdef MESSAGE_7
    mov dword [rbp+28],MESSAGE_7
%endif
%ifdef MESSAGE_8
    mov dword [rbp+32],MESSAGE_8
%endif
%ifdef MESSAGE_9
    mov dword [rbp+36],MESSAGE_9
%endif
%ifdef MESSAGE_10
    mov dword [rbp+40],MESSAGE_10
%endif
%ifdef MESSAGE_11
    mov dword [rbp+44],MESSAGE_11
%endif
%ifdef MESSAGE_12
    mov dword [rbp+48],MESSAGE_12
%endif
%ifdef MESSAGE_13
    mov dword [rbp+52],MESSAGE_13
%endif
%ifdef MESSAGE_14
    mov dword [rbp+56],MESSAGE_14
%endif
%ifdef MESSAGE_15
    mov dword [rbp+60],MESSAGE_15
%endif
  mov byte [rbp+MESSAGE_LEN],0x00
  mov r10,MESSAGE_LEN
  mov rsi, rbp
  mov INDEX, 0

lp2:
  xor byte [rsi+INDEX], SECOND_KEY_REG_BYTE
  ror SECOND_KEY_REG, 8
  inc INDEX
  cmp INDEX,MESSAGE_LEN
  jl lp2
jmp yes
noo:
  mov dword [rbp],0x00216f4e
  mov r10,4
yes:
; end of obfustation
