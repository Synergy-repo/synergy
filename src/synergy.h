#ifndef synergy_LIB_H
#define synergy_LIB_H

#include <stdint.h>
#include <stdbool.h>

#ifndef likely
#define likely( x ) __builtin_expect ( !!( x ), 1 )
#define unlikely( x ) __builtin_expect ( !!( x ), 0 )
#endif

struct sock
{
  struct rte_mbuf *pkt;
};

typedef void ( *app_func ) ( void * );

#ifdef __cplusplus
extern "C"
{
#endif
  extern bool quit;

  int
  synergy_init ( int argc, char **argv, app_func app, void *cb_arg );

  int
  get_request ( void **data, struct sock *s );

  uint16_t
  netio_send ( void *buff, uint16_t buff_len, struct sock s );

  void
  synergy_feedback_start_long ( void );

  void
  synergy_feedback_finished_long ( void );

#ifdef __cplusplus
}
#endif

#endif  // synergy_LIB_H
