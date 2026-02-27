
#include <stdbool.h>
#include <stdint.h>

#include "synergy_internal.h"
#include "globals.h"

bool quit = false;

uint16_t tot_workers;

/* when this flag is true, the current user thread cannot be reschediling to
 * other cpu core and all interrupts received when this flag is true should
 * return whitout switch current context.
 * In general, the current user level is writing/waiting a cpu flag context when
 * this flag is true and reschediling the user thread to other core make thread
 * read cpu flag incorrect. Yet, the current thread can be in critical section
 * (e.g lock). */

__thread _Atomic uint32_t current_flags;

__thread uint32_t worker_id = ~0U;

/* to synergy shim layer */
__thread bool allocator_on;
