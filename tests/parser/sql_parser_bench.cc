// Welcome to the easiest way to parse an SQL query :-)
// Compile the file like this:
//
// cc -I../ -L../ simple.c -lpg_query

// Usage: LD_LIBRARY_PATH="../sql-parser/" ./sql_parser_bench

#include "../util.h"

#include <cstdlib>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <SQLParser.h>

// contains printing utilities
#include "util/sqlhelper.h"

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
  std::string q = query;

  hsql::SQLParserResult result;
  hsql::SQLParser::parse ( q, &result );

  if ( !result.isValid () )
    {
      fprintf ( stderr, "Given string is not a valid SQL query.\n" );
      fprintf ( stderr,
                "%s (L%d:%d)\n",
                result.errorMsg (),
                result.errorLine (),
                result.errorColumn () );
      exit ( EXIT_FAILURE );
    }

  // printf ( "Parsed successfully!\n" );
  // printf ( "Number of statements: %lu\n", result.size () );
  // for ( auto i = 0u; i < result.size (); ++i )
  //  {
  //    // Print a statement summary.
  //    hsql::printStatementInfo ( result.getStatement ( i ) );
  //  }

  uint64_t start;
  for ( int i = 0; i < RUNS; i++ )
    {
      start = rdtsc ();
      hsql::SQLParser::parse ( q, &result );
      samples[i] = rdtsc () - start;
    }
}

int
main ( int argc, char **argv )
{

  init_util ();
  pin_to_cpu ( CORE );

  test ( QUERY1 );
  print ( samples, RUNS, "sql-parser (" QUERY1 ")", 0 );

  test ( QUERY2 );
  print ( samples, RUNS, "sql-parser (" QUERY2 ")", 0 );

  test ( QUERY3 );
  print ( samples, RUNS, "sql-parser (" QUERY3 ")", 0 );

  return 0;
}
