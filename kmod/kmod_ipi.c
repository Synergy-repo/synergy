
/*
 * kernel module to provide IPI interface access from kernel to user level space
 * */

#define pr_fmt( fmt ) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/smp.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/smp.h>
#include <linux/atomic.h>
#include <asm/processor.h>
#include <asm/current.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION( 5, 10, 0 )
#include <linux/sched/signal.h>
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION( 4, 10, 0 )
#include <linux/sched/task_stack.h> /* dependency to task_pt_regs */
#endif

#define KMOD_IPI_MODULE
#include "kmod_ipi.h"

static DEFINE_PER_CPU ( unsigned, synergy_interrupt_pending );
static DEFINE_PER_CPU_READ_MOSTLY ( struct task_struct *, synergy_task );

static struct cpumask mask;

/* low level trap function address in trap.S */
static __user unsigned long _trap_entry;

static int
change_flow_task ( struct pt_regs *regs )
{
  u64 new_sp;
  int ret;

  /* skip red zone, it's go back in user level code (ipi_trap.S), and allocate
   * space to push IP */
  new_sp = regs->sp - 128 - sizeof ( new_sp );

  /* push RIP in user stack (RSP) */
  ret = copy_to_user ( ( unsigned long * ) new_sp,
                       &regs->ip,
                       sizeof ( regs->ip ) );

  if ( unlikely ( ret ) )
    return -EFAULT;

  regs->sp = new_sp;

  /* change RIP to when return from kernel go to handler defined in user space
   */
  regs->ip = _trap_entry;

  // regs->flags &= ~X86_EFLAGS_TF;
  // clear_thread_flag ( TIF_SINGLESTEP );
  return 0;
}

static void
dump ( struct task_struct *task )
{
  pr_debug ( "Task: %s (pid: %d)\n", task->comm, task->pid );
  // pr_debug ( "State: %ld\n", task->state );
  pr_debug ( "Priority: %d\n", task->prio );
  pr_debug ( "Static Priority: %d\n", task->static_prio );
  pr_debug ( "Normal Priority: %d\n", task->normal_prio );
  pr_debug ( "RT Priority: %d\n", task->rt_priority );
  pr_debug ( "Parent PID: %d\n", task->real_parent->pid );

  pr_debug ( "CS: 0x%lx\n", task_pt_regs ( task )->cs );
  pr_debug ( "IP: 0x%lx\n", task_pt_regs ( task )->ip );
  pr_debug ( "SS: 0x%lx\n", task_pt_regs ( task )->ss );
  pr_debug ( "SP: 0x%lx\n", task_pt_regs ( task )->sp );
  pr_debug ( "Flags: 0x%lx\n", task_pt_regs ( task )->flags );
  pr_debug ( "orig_ax: %d\n", ( int ) task_pt_regs ( task )->orig_ax );

  pr_debug ( "Task is in %s mode\n",
             user_mode ( task_pt_regs ( task ) ) ? "user" : "kernel" );
}

static void
worker_interrupt ( void *arg )
{
  pr_debug ( "IPI received on core %d\n", smp_processor_id () );
  struct task_struct *t = this_cpu_read ( synergy_task );

  if ( likely ( t == current ) )
    {
      if ( !change_flow_task ( task_pt_regs ( t ) ) )
        return;

      pr_err ( "error from interrupt in cpu %d\n", smp_processor_id () );
      dump ( t );
    }

  this_cpu_write ( synergy_interrupt_pending, 1 );
}

static void
synergy_sched_in ( struct preempt_notifier *notifier, int cpu )
{
  if ( this_cpu_read ( synergy_interrupt_pending ) )
    {
      this_cpu_write ( synergy_interrupt_pending, 0 );
      /* current is our synergy task  here */
      if ( change_flow_task ( task_pt_regs ( current ) ) )
        {
          WARN_ON ( current != this_cpu_read ( synergy_task ) );
          dump ( current );
        }
    }
}

static void
synergy_sched_out ( struct preempt_notifier *notifier, struct task_struct *next )
{
}

static struct preempt_ops ops = { .sched_in = synergy_sched_in,
                                  .sched_out = synergy_sched_out };

static struct preempt_notifier notifier;

static void
setup_notifier ( void )
{
  this_cpu_write ( synergy_interrupt_pending, 0 );
  preempt_notifier_init ( &notifier, &ops );
  preempt_notifier_register ( &notifier );
}

static void
init_worker ( void )
{
  /* 'current' is guaranted be our application here */
  this_cpu_write ( synergy_task, current );
  setup_notifier ();

  pr_info ( "initialized core %u to task %s (%p)\n",
            smp_processor_id (),
            current->comm,
            current );
}

static int
send_multcast_ipi ( unsigned long __user *ureq )
{
  struct req_ipi req;

  if ( copy_from_user ( &req, ureq, sizeof ( req ) ) )
    return -EFAULT;

  /* TODO: user space is currently limited on sizeof(long) * 8 bits and
   can not represent full cpu set.
   Only timer core use this, then OK. */
  *cpumask_bits ( &mask ) = req.mask.bits;

  preempt_disable ();
  smp_call_function_many ( &mask, worker_interrupt, NULL, 0 );
  preempt_enable ();

  return 0;
}

static long
kmod_ioctl ( struct file *filp, unsigned int cmd, unsigned long arg )
{
  long ret = -1;

  switch ( cmd )
    {
      /* Multcast IPI */
      case KMOD_IPI_SEND:
        ret = send_multcast_ipi ( ( unsigned long __user * ) arg );
        break;
      case KMOD_INIT:
        _trap_entry = arg;
        ret = 0;
        break;
      case KMOD_INIT_WORKER:
        init_worker ();
        ret = 0;
        break;
    }

  return ret;
}

static int
kmod_open ( struct inode *inode, struct file *filp )
{
  return 0;
}

static int
kmod_release ( struct inode *inode, struct file *filp )
{
  return 0;
}

static struct file_operations fops = { .owner = THIS_MODULE,
                                       .unlocked_ioctl = kmod_ioctl,
                                       .open = kmod_open,
                                       .release = kmod_release };

static struct class *cls;

static int __init
kmod_start ( void )
{
  int r = register_chrdev ( KMOD_IPI_MAJOR_NUM, KMOD_IPI_NAME, &fops );

  if ( r < 0 )
    {
      pr_alert ( "Error to register character device" KMOD_IPI_NAME ": %d\n",
                 r );
      return r;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION( 6, 4, 0 )
  cls = class_create ( KMOD_IPI_NAME );
#else
  cls = class_create ( THIS_MODULE, KMOD_IPI_NAME );
#endif
  device_create ( cls,
                  NULL,
                  MKDEV ( KMOD_IPI_MAJOR_NUM, 0 ),
                  NULL,
                  KMOD_IPI_NAME );

  preempt_notifier_inc ();
  pr_info ( "loaded\n" );

  return 0;
}

static void __exit
kmod_exit ( void )
{
  device_destroy ( cls, MKDEV ( KMOD_IPI_MAJOR_NUM, 0 ) );
  class_destroy ( cls );

  unregister_chrdev ( KMOD_IPI_MAJOR_NUM, KMOD_IPI_NAME );

  preempt_notifier_dec ();
  pr_info ( "unloaded\n" );
}

module_init ( kmod_start );
module_exit ( kmod_exit );

MODULE_LICENSE ( "GPL" );
MODULE_DESCRIPTION ( "Provide IPI access interface from kernel to user level" );
