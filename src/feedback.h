#ifndef synergy_FEEDBACK_H
#define synergy_FEEDBACK_H

void
synergy_feedback_start_long ( void );

void
synergy_feedback_finished_long ( void );

void
synergy_feedback_start_request ( void );

void
synergy_feedback_finished_request ( void );

void
feedback_init_per_worker ( void );

uint64_t
feedback_get_shorts_tsc_avg ( void );

#endif  // synergy_FEEDBACK_H
