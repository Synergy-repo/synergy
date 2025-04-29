
#define RAX (8*0)
#define RBX (8*1)
#define RCX (8*2)
#define RDX (8*3)
#define RSI (8*4)
#define RBP (8*5)
#define R8  (8*6)
#define R9  (8*7)
#define R10 (8*8)
#define R11 (8*9)
#define R12 (8*10)
#define R13 (8*11)
#define R14 (8*12)
#define R15 (8*13)
#define RDI (8*14)

/* size for saving all registers */
#define SAVE_SIZE (8*15)

.macro save_registers
    /* allocate stack space to save registers */
    subq $SAVE_SIZE, %rsp

    movq %rax, RAX(%rsp)
    movq %rbx, RBX(%rsp)
    movq %rcx, RCX(%rsp)
    movq %rdx, RDX(%rsp)
    movq %rsi, RSI(%rsp)
    movq %rbp, RBP(%rsp)
    movq %r8,  R8(%rsp)
    movq %r9,  R9(%rsp)
    movq %r10, R10(%rsp)
    movq %r11, R11(%rsp)
    movq %r12, R12(%rsp)
    movq %r13, R13(%rsp)
    movq %r14, R14(%rsp)
    movq %r15, R15(%rsp)
    movq %rdi, RDI(%rsp)
.endm

.macro restore_registers
  movq RAX(%rsp), %rax
  movq RBX(%rsp), %rbx
  movq RCX(%rsp), %rcx
  movq RDX(%rsp), %rdx
  movq RSI(%rsp), %rsi
  movq RBP(%rsp), %rbp
  movq R8(%rsp),  %r8
  movq R9(%rsp),  %r9
  movq R10(%rsp), %r10
  movq R11(%rsp), %r11
  movq R12(%rsp), %r12
  movq R13(%rsp), %r13
  movq R14(%rsp), %r14
  movq R15(%rsp), %r15
  movq RDI(%rsp), %rdi

  /* release stack */
  addq $SAVE_SIZE, %rsp
.endm

