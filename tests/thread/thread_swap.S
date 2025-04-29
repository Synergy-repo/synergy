

.file "thread_swap.S"
.text

/*
 * Trap Frame Format
 * WARNING: These values reflect the layout of struct thread_tf. Don't change
 * these values without know.
 */

/* arguments registers (can be clobbered) */
#define RDI	(0)
#define RSI	(8)
#define RDX	(16)
#define RCX	(24)
#define R8	(32)
#define R9	(40)

/* temporary registers (can be clobbered) */
#define R10	(48)
#define R11	(56)

/* callee-saved registers (can not be clobbered) */
#define RBX	(64)
#define RBP	(72)
#define R12	(80)
#define R13	(88)
#define R14	(96)
#define R15	(104)

/* special-purpose registers */
#define RAX	(112)	/* return code */
#define RIP	(120)	/* instruction pointer */
#define RSP	(128)	/* stack pointer */

.align 16
.globl thread_set_main
.type thread_set_main, @function
thread_set_main:
  /* preserved registers */
  movq RBX(%rdi), %rbx
  movq RBP(%rdi), %rbp
  movq R12(%rdi), %r12
  movq R13(%rdi), %r13
  movq R14(%rdi), %r14
  movq R15(%rdi), %r15

  movq RSP(%rdi), %rsp
  pushq RIP(%rdi)
  ret

.align 16
.globl thread_swap2worker
.type thread_swap2worker, @function
thread_swap2worker:
  /* save main thread, only preserved registers */
  movq %rbx, RBX(%rdi)
  movq %rbp, RBP(%rdi)
  movq %r12, R12(%rdi)
  movq %r13, R13(%rdi)
  movq %r14, R14(%rdi)
  movq %r15, R15(%rdi)

  /* save rip */
  movq (%rsp), %r8
  movq %r8, RIP(%rdi)

  /* save rsp. Exclude return address because after push rip decrement stack
  to right position */
  leaq 8(%rsp), %r8
  movq %r8, RSP(%rdi)

  /* set worker thread */
  movq RBX(%rsi), %rbx
  movq RBP(%rsi), %rbp
  movq R12(%rsi), %r12
  movq R13(%rsi), %r13
  movq R14(%rsi), %r14
  movq R15(%rsi), %r15

  movq RDI(%rsi), %rdi
  movq RDX(%rsi), %rdx
  movq RCX(%rsi), %rcx
  movq R8(%rsi), %r8
  movq R9(%rsi), %r9
  movq R10(%rsi), %r10
  movq R11(%rsi), %r11

  movq RAX(%rsi), %rax
  movq RSP(%rsi), %rsp

  pushq RIP(%rsi)

  /* finally setup rsi */
  movq RSI(%rsi), %rsi
  ret

.align 16
.globl thread_swap2main
.type thread_swap2main, @function
thread_swap2main:
  /* save worker thread, all register */

  /* preserved registers */
  movq %rbx, RBX(%rdi)
  movq %rbp, RBP(%rdi)
  movq %r12, R12(%rdi)
  movq %r13, R13(%rdi)
  movq %r14, R14(%rdi)
  movq %r15, R15(%rdi)

  /* clobbered registers */
  movq %rdi, RDI(%rdi)
  movq %rsi, RSI(%rdi)
  movq %rdx, RDX(%rdi)
  movq %rcx, RCX(%rdi)
  movq %r8, R8(%rdi)
  movq %r9, R9(%rdi)
  movq %r10, R10(%rdi)
  movq %r11, R11(%rdi)

  /* special-purpose registers */
  movq %rax, RAX(%rdi)

  /* save rip */
  movq (%rsp), %r8
  movq %r8, RIP(%rdi)

  /* save rsp, exclude return address */
  leaq 8(%rsp), %r8
  movq %r8, RSP(%rdi)

  /* set main thread */

  /* preserved registers */
  movq RBX(%rsi), %rbx
  movq RBP(%rsi), %rbp
  movq R12(%rsi), %r12
  movq R13(%rsi), %r13
  movq R14(%rsi), %r14
  movq R15(%rsi), %r15

  movq RSP(%rsi), %rsp
  pushq RIP(%rsi)
  ret
