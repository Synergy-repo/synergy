
/*
* User level threads switch.
* Here is necessary handle only preserved register and some other special
* register, as rbp, rip and rdi (first argument to thread).
* Preempted thread has all registers restored by preemption system
* (i.e signals or kmod_ipi in or case).
*
* This impĺementation doesn't care about others register, like point floating,
* if don't general purpose.
*/

/*
 * Trap Frame Format
 * WARNING: These values reflect the layout of struct thread_tf. Don't change
 * these values without know.
 *
 * struct thread_tf should be first element in struct thread.
 */

/* preserved registers */
#define RBX	(8*0)
#define RBP	(8*1)
#define R12	(8*2)
#define R13	(8*3)
#define R14	(8*4)
#define R15	(8*5)

/* special-purpose registers */
#define RIP	(8*6)	/* instruction pointer */
#define RSP	(8*7)	/* stack pointer */

.file "thread_switch.S"
.section        .note.GNU-stack,"",@progbits
.text

.align 16
.globl thread_set
.type thread_set, @function
thread_set:
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
.globl thread_switch
.type thread_switch, @function
thread_switch:
  /* save current thread */

  /* preserved registers */
  movq %rbx, RBX(%rdi)
  movq %rbp, RBP(%rdi)
  movq %r12, R12(%rdi)
  movq %r13, R13(%rdi)
  movq %r14, R14(%rdi)
  movq %r15, R15(%rdi)

  /* Save rip */
  movq (%rsp), %r10
  movq %r10, RIP(%rdi)

  /* Save rsp. Skip return address because after push rip decrement stack
  to right position */
  leaq 8(%rsp), %r11
  movq %r11, RSP(%rdi)

  /* set new thread */

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

