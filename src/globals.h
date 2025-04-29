
#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdbool.h>
#include <stdint.h>

extern bool quit;

extern uint16_t tot_workers;

extern __thread _Atomic uint32_t current_flags;

/* per worker variables */
extern __thread uint32_t worker_id;

/* to synergy shim layer */
extern __thread bool allocator_on;

/* current_flags defines */
#define synergy_MAIN ( 1U << 0 )
#define synergy_SHORT ( 1U << 1 )
#define synergy_LONG ( 1U << 2 )
#define synergy_YIELD ( 1U << 3 )
#define synergy_FINISHED ( 1U << 4 )
#define synergy_RESCHEDULING_DISABLE ( 1U << 5 )

#define CURRENT_SET_MAIN() \
  ( current_flags = ( synergy_MAIN | synergy_RESCHEDULING_DISABLE ) )

#define CURRENT_SET_SHORT() \
  ( current_flags = ( synergy_SHORT | synergy_RESCHEDULING_DISABLE ) )

#define CURRENT_SET_FINISHED() \
  ( current_flags |= ( synergy_FINISHED | synergy_RESCHEDULING_DISABLE ) )

/* long request is only type that can be rescheduled */
#define CURRENT_SET_LONG() ( current_flags = synergy_LONG )

#define CURRENT_SET_YIELD() ( current_flags |= synergy_YIELD )
#define CURRENT_DISABLE_RESCHEDULING() \
  ( current_flags |= synergy_RESCHEDULING_DISABLE )

#define CURRENT_ENABLE_RESCHEDULING() \
  ( current_flags &= ~synergy_RESCHEDULING_DISABLE )

#define CURRENT_IS_MAIN() ( !!( current_flags & synergy_MAIN ) )
#define CURRENT_IS_SHORT() ( !!( current_flags & synergy_SHORT ) )
#define CURRENT_IS_LONG() ( !!( current_flags & synergy_LONG ) )
#define CURRENT_IS_FINISHED() ( !!( current_flags & synergy_FINISHED ) )

#ifdef CONCORD
#define CURRENT_SHOULD_YIELD() ( !!concord_preempt_now )
#else
#define CURRENT_SHOULD_YIELD() ( !!( current_flags & synergy_YIELD ) )
#endif

#define CURRENT_IS_RESCHEDULING_DISABLED() \
  ( !!( current_flags & synergy_RESCHEDULING_DISABLE ) )

#endif
