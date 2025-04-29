
#define _GNU_SOURCE
#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../tests/util.h"

#include "uintr.h"

static volatile int worker_ready;

static __thread volatile int worker_stop, wid;

#define MAIN_CORE 0
#define TOT_WORKERS 1

static int workers_cores[] = { 2, 4 };
static int workers_uitt_idx[TOT_WORKERS];

static long values[16];

static int fd_kmod_uintr;

static int red_zone_test = 1;

/* not allocate any thing inside this function to avoid compiler stack
 * allocation (i.e rsp decrement )*/
void
red_zone_write (void)
{
  asm volatile("movq $42, -8(%rsp)\n"   // Store 42 at 8 bytes below RSP
               "movq $42, -16(%rsp)\n"  // Store 42 at 16 bytes below RSP
               "movq $42, -24(%rsp)\n"  // Store 42 at 24 bytes below RSP
               "movq $42, -32(%rsp)\n"  // Store 42 at 32 bytes below RSP
               "movq $42, -40(%rsp)\n"  // Store 42 at 40 bytes below RSP
               "movq $42, -48(%rsp)\n"  // Store 42 at 48 bytes below RSP
               "movq $42, -56(%rsp)\n"  // Store 42 at 56 bytes below RSP
               "movq $42, -64(%rsp)\n"  // Store 42 at 64 bytes below RSP
               "movq $42, -72(%rsp)\n"  // Store 42 at 72 bytes below RSP
               "movq $42, -80(%rsp)\n"  // Store 42 at 80 bytes below RSP
               "movq $42, -88(%rsp)\n"  // Store 42 at 88 bytes below RSP
               "movq $42, -96(%rsp)\n"  // Store 42 at 96 bytes below RSP
               "movq $42, -104(%rsp)\n" // Store 42 at 104 bytes below RSP
               "movq $42, -112(%rsp)\n" // Store 42 at 112 bytes below RSP
               "movq $42, -120(%rsp)\n" // Store 42 at 120 bytes below RSP
               "movq $42, -128(%rsp)\n" // Store 42 at 128 bytes below RSP
  );

  asm volatile("mov $5, %%rax\n\t;"
               "mov $5, %%rdi\n\t;"
               "mov $5, %%rsi\n\t;"
               "mov $5, %%rdx\n\t;"
               "mov $5, %%rcx\n\t;"
               "mov $5, %%rbp\n\t;"
               "mov $5, %%r8\n\t;"
               "mov $5, %%r9\n\t;"
               "mov $5, %%r10\n\t;"
               "mov $5, %%r11\n\t;"
               "mov $5, %%rbx\n\t;"
               "mov $5, %%r12\n\t;"
               "mov $5, %%r13\n\t;"
               "mov $5, %%r14\n\t;"
               "mov $5, %%r15\n\t;"
               :
               :
               :);

  worker_ready++;
  while (!worker_stop)
    ;

  asm volatile(
      "movq -8(%%rsp), %%rax\n"
      "movq %%rax, %0\n"
      "movq -16(%%rsp), %%rax\n"
      "movq %%rax, %1\n"
      "movq -24(%%rsp), %%rax\n"
      "movq %%rax, %2\n"
      "movq -32(%%rsp), %%rax\n"
      "movq %%rax, %3\n"
      "movq -40(%%rsp), %%rax\n"
      "movq %%rax, %4\n"
      "movq -48(%%rsp), %%rax\n"
      "movq %%rax, %5\n"
      "movq -56(%%rsp), %%rax\n"
      "movq %%rax, %6\n"
      "movq -64(%%rsp), %%rax\n"
      "movq %%rax, %7\n"
      "movq -72(%%rsp), %%rax\n"
      "movq %%rax, %8\n"
      "movq -80(%%rsp), %%rax\n"
      "movq %%rax, %9\n"
      "movq -88(%%rsp), %%rax\n"
      "movq %%rax, %10\n"
      "movq -96(%%rsp), %%rax\n"
      "movq %%rax, %11\n"
      "movq -104(%%rsp), %%rax\n"
      "movq %%rax, %12\n"
      "movq -112(%%rsp), %%rax\n"
      "movq %%rax, %13\n"
      "movq -120(%%rsp), %%rax\n"
      "movq %%rax, %14\n"
      "movq -128(%%rsp), %%rax\n"
      "movq %%rax, %15\n"
      : "=m"(values[0]), "=m"(values[1]), "=m"(values[2]), "=m"(values[3]),
        "=m"(values[4]), "=m"(values[5]), "=m"(values[6]), "=m"(values[7]),
        "=m"(values[8]), "=m"(values[9]), "=m"(values[10]), "=m"(values[11]),
        "=m"(values[12]), "=m"(values[13]), "=m"(values[14]), "=m"(values[15])

      :
      : "memory");
}

void
red_zone_check (void)
{
  int i, err = 0;
  for (i = 0; i < 16; i++)
    {
      if (values[i] != 42)
        {
          err = 1;
          printf ("Red zone corruption at offset -%d: expected 42, got %ld\n",
                  (i + 1) * 8, values[i]);
        }
    }
  if (!err)
    puts ("Red zone OK");
}

static void
my_handler (void)
{
  printf ("wid: %d Uhul from handler\n", wid);
  worker_stop = 1;
}

void *
worker (void *arg)
{
  wid = (int)(uint64_t)arg;
  int idx = uintr_init_worker ();
  if (idx < 0)
    {
      fprintf (stderr, "%s\n", "Error to init worker");
      exit (1);
    }

  printf ("wid: %d idx: %d\n", wid, idx);
  workers_uitt_idx[wid] = idx;

  if (red_zone_test)
    {
      red_zone_write ();
      red_zone_check ();
    }
  else
    {
      worker_ready += 1;
      while (!worker_stop)
        {
          char buff[512];
          // read ( 1, buff, sizeof buff );
        }
    }

  printf ("Uhul from worker %d\n", wid);

  uintr_free_worker (idx);

  return NULL;
}

static pthread_t
setup_worker (int core)
{
  static int id = 0;
  cpu_set_t cpus;
  CPU_ZERO (&cpus);
  CPU_SET (core, &cpus);
  pthread_attr_t attr;
  pthread_attr_init (&attr);
  pthread_attr_setaffinity_np (&attr, sizeof (cpu_set_t), &cpus);
  pthread_t tid;
  assert (0 == pthread_create (&tid, &attr, worker, (void *)(uint64_t)id++));
  return tid;
}

static void
test_single_ipi (void)
{
  worker_ready = worker_stop = 0;

  /* setup worker */
  pthread_t tids[TOT_WORKERS];
  for (int i = 0; i < TOT_WORKERS; i++)
    tids[i] = setup_worker (workers_cores[i]);

  while (worker_ready != TOT_WORKERS)
    ;

  /* send UIPI */
  for (int i = 0; i < TOT_WORKERS; i++)
    {
      printf ("SENDUIPI %d\n", i);
      uintr_senduipi (workers_uitt_idx[i]);
    }

  for (int i = 0; i < TOT_WORKERS; i++)
    pthread_join (tids[i], NULL);
}

int
main (void)
{
  init_util ();

  /* setup main thread */
  pin_to_cpu (MAIN_CORE);

  if (uintr_init (my_handler))
    {
      fprintf (stderr, "%s\n", "Error to open kmod_uintr");
      return 1;
    }
  if (uintr_init_sender ())
    {
      fprintf (stderr, "%s\n", "Error to init sender");
      return 1;
    }

  test_single_ipi ();

  uintr_free ();

  return 0;
}
