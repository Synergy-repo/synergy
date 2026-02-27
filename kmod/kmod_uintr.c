
/*
 * kernel module to uintr feature
 *
 * This is to synergy context usage only, we dont't care about security of non synergy
 * tasks using this module.
 * In synergy, all receivers start/finish on same time, ensuring the shared UITT
 * consistency.
 *
 * Also, in synergy context, worker kernel threads not are moved of CPU after
 * initial startup. Then we not need enable XAVE feature to save UINTR state
 * and not even care about update upid.ndst value.
 *
 * */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <asm/apic.h>
#include <asm/atomic.h>
#include <asm/irq.h>
#include <asm/processor.h>
#include <asm/tlbflush.h>

#include <linux/bits.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#include "kmod_uintr.h"

/* User Interrupt interface */
#define MSR_IA32_UINTR_RR 0x985
#define MSR_IA32_UINTR_HANDLER 0x986
#define MSR_IA32_UINTR_STACKADJUST 0x987
#define MSR_IA32_UINTR_MISC 0x988 /* 39:32-UINV, 31:0-UITTSZ */
#define MSR_IA32_UINTR_PD 0x989
#define MSR_IA32_UINTR_TT 0x98a

#define X86_CR4_UINTR_BIT 25 /* enable User Interrupts support */
#define X86_CR4_UINTR BIT_ULL (X86_CR4_UINTR_BIT)

// #define UINTR_NOTIFICATION_IRQ_VECTOR ( LOCAL_TIMER_VECTOR + 1 )
#define UINTR_NOTIFICATION_IRQ_VECTOR POSTED_INTR_WAKEUP_VECTOR

#define OS_ABI_REDZONE 128

/* User Interrupt Target Table Entry (UITTE) */
struct uintr_uitt_entry
{
  u8 valid; /* bit 0: valid, bit 1-7: reserved */
  u8 user_vec;
  u8 reserved[6];
  u64 target_upid_addr;
} __packed __aligned (16);

/* User Posted Interrupt Descriptor (UPID) */
struct uintr_upid
{
  struct
  {
    u8 status;    /* bit 0: ON, bit 1: SN, bit 2-7: reserved */
    u8 reserved1; /* Reserved */
    u8 nv;        /* Notification vector (Used by apic) */
    u8 reserved2; /* Reserved */
    u32 ndst;     /* Notification destination */
  } nc __packed;  /* Notification control */
  u64 puir;       /* Posted user interrupt requests */
} __aligned (64);

/* TODO: check this size or make this dynamic */
#define MAX_RECEIVERS 32

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 8, 0)
static __always_inline int
atomic_fetch_add (int i, atomic_t *v)
{
  return xadd (&v->counter, i);
}
#endif

static struct uintr_uitt_entry uitt[MAX_RECEIVERS] = { 0 };
static atomic_t uitt_idx = ATOMIC_INIT (0);

static inline u32
cpu_to_ndst (int cpu)
{
  u32 apicid = (u32)apic->cpu_present_to_apicid (cpu);

  WARN_ON_ONCE (apicid == BAD_APICID);

  if (!x2apic_enabled ())
    return (apicid << 8) & 0xFF00;

  return apicid;
}

static int
init_receiver (unsigned long __user data)
{
  struct uintr_upid *upid;
  struct uintr_uitt_entry *uitte;
  u64 misc_msr;
  unsigned long i, user_handler;
  int cpu;

  /* get user interrupt handler address */
  if (copy_from_user (&user_handler, (unsigned long *__user)data,
                      sizeof (user_handler)))
    {
      pr_err ("Error to get user handler address\n");
      return -EFAULT;
    }

  /* get new slot on UITT */
  i = atomic_fetch_add (1, &uitt_idx);
  if (i >= MAX_RECEIVERS)
    {
      pr_err ("Shared UITT is full with %d entries\n", MAX_RECEIVERS);
      return -EFAULT;
    }

  /* inform the user yours index of uitt */
  if (copy_to_user ((unsigned long __user *)data, &i, sizeof (i)))
    {
      pr_err ("Error to set user index of UITT\n");
      return -EFAULT;
    }

  upid = kzalloc (sizeof (*upid), GFP_KERNEL);
  if (!upid)
    return -ENOMEM;

  cpu = smp_processor_id ();

  /* setup UPID */
  // upid->nc.status = 0b00;
  upid->nc.nv = UINTR_NOTIFICATION_IRQ_VECTOR;
  upid->nc.ndst = cpu_to_ndst (cpu);
  upid->puir = 0;

  wrmsrl (MSR_IA32_UINTR_HANDLER, user_handler);
  wrmsrl (MSR_IA32_UINTR_PD, (u64)upid);
  wrmsrl (MSR_IA32_UINTR_STACKADJUST, OS_ABI_REDZONE);

  rdmsrl (MSR_IA32_UINTR_MISC, misc_msr);
  misc_msr |= (u64)UINTR_NOTIFICATION_IRQ_VECTOR << 32;
  wrmsrl (MSR_IA32_UINTR_MISC, misc_msr);

  /* register our upid on shared UITT */
  uitte = &uitt[i];
  uitte->valid = 1;
  uitte->user_vec = 0; /* TODO: permit user set this value? */
  uitte->target_upid_addr = (u64)upid;

  /* enable uintr */
  cr4_set_bits (X86_CR4_UINTR);

  pr_info ("Registred receiver to task: %s on core: %u with UITT "
           "index: %lu\n",
           current->comm, cpu, i);

  return 0;
}

static int
init_sender (void)
{
  u64 msr;

  /* register UITT and enable SENDUIPI */
  wrmsrl (MSR_IA32_UINTR_TT, (u64)uitt | 1);

  rdmsrl (MSR_IA32_UINTR_MISC, msr);
  msr &= GENMASK_ULL (63, 32); // ensure UITTSZ is 0
  msr |= MAX_RECEIVERS - 1;    // set UITTSZ
  wrmsrl (MSR_IA32_UINTR_MISC, msr);

  cr4_set_bits (X86_CR4_UINTR);

  pr_info ("Registred sender to task: %s on core: %u\n", current->comm,
           smp_processor_id ());

  return 0;
}

static int
uintr_teardown (unsigned long __user arg)
{
  pr_info ("cleanup uintr state to task %s on core %u UITT index %lu\n",
           current->comm, smp_processor_id (), arg);

  struct uintr_upid *upid;
  upid = (struct uintr_upid *)uitt[arg].target_upid_addr;
  uitt[arg].valid = 0;

  kfree (upid);

  atomic_sub (1, &uitt_idx);

  /* Clear UINTR state */
  wrmsrl (MSR_IA32_UINTR_PD, 0);
  wrmsrl (MSR_IA32_UINTR_TT, 0);
  wrmsrl (MSR_IA32_UINTR_MISC, 0);
  wrmsrl (MSR_IA32_UINTR_HANDLER, 0);
  wrmsrl (MSR_IA32_UINTR_STACKADJUST, 0);

  cr4_clear_bits (X86_CR4_UINTR);

  return 0;
}

static long
kmod_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{
  switch (cmd)
    {
    case KMOD_UINTR_INIT_RECEIVER:
      return init_receiver (arg);
    case KMOD_UINTR_INIT_SENDER:
      return init_sender ();
    case KMOD_UINTR_FREE:
      return uintr_teardown (arg);
    }

  return -EINVAL;
}

static int
kmod_open (struct inode *inode, struct file *filp)
{
  return 0;
}

static int
kmod_release (struct inode *inode, struct file *filp)
{
  return 0;
}

static struct file_operations fops = { .owner = THIS_MODULE,
                                       .unlocked_ioctl = kmod_ioctl,
                                       .open = kmod_open,
                                       .release = kmod_release };

static struct class *cls;

static int __init
kmod_start (void)
{
  if (uintr_processor_support ())
    {
      pr_err ("Processor no support uintr feature");
      return -1;
    }

  pr_debug ("Processor support to uintr: OK");

  int r = register_chrdev (KMOD_UINTR_MAJOR_NUM, KMOD_UINTR_NAME, &fops);

  if (r < 0)
    {
      pr_err ("Error to register character device: %d\n", r);
      return r;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
  cls = class_create (KMOD_UINTR_NAME);
#else
  cls = class_create (THIS_MODULE, KMOD_UINTR_NAME);
#endif
  device_create (cls, NULL, MKDEV (KMOD_UINTR_MAJOR_NUM, 0), NULL,
                 KMOD_UINTR_NAME);

  pr_info ("loaded\n");

  return 0;
}

static void __exit
kmod_exit (void)
{
  device_destroy (cls, MKDEV (KMOD_UINTR_MAJOR_NUM, 0));
  class_destroy (cls);

  unregister_chrdev (KMOD_UINTR_MAJOR_NUM, KMOD_UINTR_NAME);

  pr_info ("unloaded\n");
}

module_init (kmod_start);
module_exit (kmod_exit);

MODULE_LICENSE ("GPL");
MODULE_DESCRIPTION ("Uintr module");
