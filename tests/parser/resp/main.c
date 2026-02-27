
#include "../../util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "resp.h"

#define RUNS 500000
static uint32_t samples[RUNS];

#define ARRAY_LEN( a ) ( sizeof ( a ) / sizeof ( *a ) )

void
dump ( struct resp_client *r )
{
  char buff[64];
  printf ( "tot_cmds: %d\n", r->tot_cmds );
  for ( unsigned i = 0; i < r->tot_cmds; i++ )
    {
      assert ( sizeof ( buff ) > r->bs[i].size );

      memcpy ( buff, r->bs[i].string, r->bs[i].size );
      buff[r->bs[i].size] = '\0';

      printf ( "command %d: %s\n", i, buff );
    }
}

enum request_type
{
  GET = 0,
  SET,
  SCAN,
  UNKNOW
};

enum request_type
get_request_type ( struct resp_client *r )
{
  char *type = r->bs[0].string;

  if ( strncmp ( type, "GET", strlen ( "GET" ) ) == 0 )
    return GET;

  if ( strncmp ( type, "SET", strlen ( "SET" ) ) == 0 )
    return SET;

  if ( strncmp ( type, "SCAN", strlen ( "SCAN" ) ) == 0 )
    return SCAN;

  return UNKNOW;
}

void
test ( void )
{
  char buff[512];
  char *cmd[] = { "SCAN", "key", "value" };

  int ret = resp_encode ( buff, sizeof ( buff ), cmd, ARRAY_LEN ( cmd ) );
  ( void ) ret;
  assert ( ret == 0 );
  printf ( "encoded: %s\n", buff );

  struct resp_client rc;
  resp_decode ( &rc, buff );
  dump ( &rc );

  enum request_type rt = get_request_type ( &rc );
  printf ( "request type: %d\n", rt );
}

void
bench ( void )
{
  char encoded[128];
  char *cmd[] = { "SET", "mykey", "myvalue" };

  struct resp_client decoded;

  int r = resp_encode ( encoded, sizeof ( encoded ), cmd, ARRAY_LEN ( cmd ) );
  if ( r )
    {
      fprintf ( stderr, "error to encode" );
      exit ( 1 );
    }

  for ( unsigned i = 0; i < RUNS; i++ )
    {
      uint64_t start = rdtsc ();
      resp_decode ( &decoded, encoded );
      samples[i] = rdtsc () - start;
    }

  dump ( &decoded );

  print ( samples, RUNS, "resp_decode: 'GET mykey'", 0 );
}

int
main ( int argc, char **argv )
{
  init_util ();
  print_tsc ();
  pin_to_cpu ( 0 );

  bench ();

  return 0;
}
