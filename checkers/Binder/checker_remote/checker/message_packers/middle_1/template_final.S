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

  MESSAGE_PUT_ON_STACK
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
