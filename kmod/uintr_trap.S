
/*
* low level entry points in user space to handle interrupts
*
* _uintr_trap_entry is a low level function used by kmod_uintr module.
* Unlike _trap_entry, _uintr_trap_entry not need save
* flags register and worry about red zone. UIRET instruction handle all things.
*/

/* disable stack executable */
.section .note.GNU-stack,"",@progbits

.file "uintr_trap.S"

#include "register_macros.inc"

.text

.align 16
.globl _uintr_trap_entry
.type _uintr_trap_entry, @function
_uintr_trap_entry:
	.cfi_startproc
	.cfi_signal_frame

  save_registers

  /* UIF is disabled by hardware when receive a interrupt.
  To synergy use, we need reenable this to permit other interrupt. */
  stui

  /* call user level interrupt handler (defined in uintr.c) */
  subq $8, %rsp
  call _uintr_handler
  addq $8, %rsp

  restore_registers

  /* release UIRRV */
  addq $8, %rsp
  uiret

  .cfi_endproc
