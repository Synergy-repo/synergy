
#ifndef synergy_BITMASK_H
#define synergy_BITMASK_H

#include <stdatomic.h>
#include <limits.h>

typedef _Atomic uint64_t bitmask;

#define BITMASK_MAX_SIZE ( sizeof ( uint64_t ) * CHAR_BIT )

static inline void
bitmask_init ( bitmask *bm )
{
  *bm = 0;
}

static inline bool
bitmask_is_empty ( bitmask *bm )
{
  return ( 0 == *bm );
}

static inline void
bitmask_atomic_bit_set ( bitmask *bm, unsigned bit )
{
  uint64_t mask = ( UINT64_C ( 1 ) << bit );
  *bm |= mask;
}

static inline void
bitmask_atomic_bit_clear ( bitmask *bm, unsigned bit )
{
  uint64_t mask = ( UINT64_C ( 1 ) << bit );
  *bm &= ~mask;
}

static inline bool
bitmask_bit_is_set ( bitmask *bm, unsigned bit )
{
  uint64_t mask = ( UINT64_C ( 1 ) << bit );
  return ( *bm & mask );
}

/* return true if bit previously was 1. false if bit not changed. */
static inline bool
bitmask_atomic_bit_test_and_clear ( bitmask *bm, unsigned bit )
{
  uint64_t mask = ( UINT64_C ( 1 ) << bit );
  uint64_t old_value = atomic_fetch_and ( bm, ~mask );

  return ( old_value & mask ) != 0;
}

static inline void
bitmask_copy ( bitmask *restrict dst, bitmask *restrict src )
{
  *dst = *src;
}

static inline void
bitmask_and ( bitmask *bm, uint64_t v )
{
  *bm &= v;
}

/* Iterate over each enable (1) bit in bit mask.
 * bm: bitmask.
 * nr: index of current bit enable */
#define BM_FOREACH_SET_BIT( bm, nr )             \
  bitmask __bm_tmp;                              \
  for ( bitmask_copy ( &__bm_tmp, &bm ),         \
        ( nr ) = __builtin_ctzll ( __bm_tmp );   \
        !bitmask_is_empty ( &__bm_tmp );         \
        bitmask_and ( &__bm_tmp, __bm_tmp - 1 ), \
             ( nr ) = __builtin_ctzll ( __bm_tmp ) )

/* return true if was previously 1. false if bit changed from 0 to 1. */
// static bool
// bitmask_atomic_bit_test_and_set ( bitmask *bm, unsigned bit )
//{
//  uint64_t mask = ( UINT64_C ( 1 ) << bit );
//  uint64_t old_value = atomic_fetch_or ( bm, mask );
//
//  return ( old_value & mask );
//}

#endif
