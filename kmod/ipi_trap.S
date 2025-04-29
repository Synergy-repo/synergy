
/*
* low level entry points in user space to handle interrupts
*
* _trap_entry is a low level function used by kmod_ipi module
*
* On kernel module (kmod_ipi) we put IP(instruction pointer)
* of app thread on stack. After calling user handling function
* we go back to application code before interruption.
*
* When we got here all register has application value state.
* So we save, on stack, all register before calling user handler interruption
* and restore after. Futhermore, we release the red zone size,
* set in kernel module, that avoid write on application red zone.
*/

/* disable stack executable */
.section .note.GNU-stack,"",@progbits

.file "ipi_trap.S"

#include "register_macros.inc"

.text

.align 16
.globl _ipi_entry
.type _ipi_entry, @function
_ipi_entry:
	.cfi_startproc
	.cfi_signal_frame
  /* Save flag register. Save only low 16 bits from flags. */
  pushf

  save_registers

  /* call we high level interrupt handler */
  call _ipi_handler

  restore_registers

  /* restore flags */
  popf

  /* in kernel module kmod_ipi we put return address to thread app on stack.
   * after get return address on stack top and increase 8 bytes, also
   * increase rsp by more 128 bytes because red zone skiped on kernel module. */
  ret $128

  .cfi_endproc
