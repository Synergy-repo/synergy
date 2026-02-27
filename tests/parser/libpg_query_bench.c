// Welcome to the easiest way to parse an SQL query :-)
// Compile the file like this:
//
// cc -I../ -L../ simple.c -lpg_query

#include "../util.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <pg_query.h>

#define QUERY1 "SELECT 1;"
#define QUERY2 "SELECT id FROM x WHERE z = 2;"
#define QUERY3                           \
  "SELECT student_id, COUNT(student_id)" \
  " FROM tbl_scores"                     \
  " GROUP BY student_id"                 \
  " HAVING COUNT(student_id)> 1"         \
  " ORDER BY student_id;"

#define CORE 5
#define RUNS 500000

uint32_t samples[RUNS];

void
test ( const char *query )
{
  PgQueryParseResult result;
  result = pg_query_parse ( query );
  if ( result.error )
    {
      printf ( "error: %s at %d\n",
               result.error->message,
               result.error->cursorpos );
      exit ( EXIT_FAILURE );
    }

  // printf ( "\n%s\n", result.parse_tree );

  uint64_t start;
  for ( int i = 0; i < RUNS; i++ )
    {
      start = rdtsc ();
      result = pg_query_parse ( query );
      samples[i] = rdtsc () - start;

      pg_query_free_parse_result ( result );
    }
}

int
main ( int argc, char **argv )
{
  init_util ();
  pin_to_cpu ( CORE );

  test ( QUERY1 );
  print ( samples, RUNS, "libpg_query parser (" QUERY1 ")", 0 );

  test ( QUERY2 );
  print ( samples, RUNS, "libpg_query parser (" QUERY2 ")", 0 );

  test ( QUERY3 );
  print ( samples, RUNS, "libpg_query parser (" QUERY3 ")", 0 );

  // Optional, this ensures all memory is freed upon program exit (useful when
  // running Valgrind)
  pg_query_exit ();

  return 0;
}
