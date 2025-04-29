
#define _GNU_SOURCE
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>

#include <leveldb/c.h>

#include "util.h"

#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

#define DFL_DB_PATH "/tmpfs/my_db"
#define SAMPLES 100000UL
#define NUM_ENTRIES 5000

#define NDEBUG

static leveldb_t *db;

static uint32_t samples[SAMPLES];

__thread int concord_preempt_now;
void concord_enable(void){}
void concord_disable(void){}
void concord_func(void){}

static void
do_get ( void )
{
  size_t len;
  char *value, *err = NULL, *key = "k4501";
  
  leveldb_readoptions_t *readoptions = leveldb_readoptions_create ();

  value = leveldb_get ( db, readoptions, key, strlen ( key ), &len, &err );
  free ( value );
  free ( err );
  
  leveldb_readoptions_destroy ( readoptions );
}

static void
do_scan ( void )
{
  const char *retr_key;
  size_t len;
  char *err = NULL;
  leveldb_readoptions_t *readoptions = leveldb_readoptions_create ();

  leveldb_iterator_t *iter = leveldb_create_iterator ( db, readoptions );
  leveldb_iter_seek_to_first ( iter );
  while ( leveldb_iter_valid ( iter ) )
    {
      retr_key = leveldb_iter_key ( iter, &len );
#ifndef NDEBUG
      char *value = leveldb_get ( db, readoptions, retr_key, len, &len, &err );
      assert ( !err );
      printf ( "key:%s value:%s\n", retr_key, value );
#endif
      leveldb_iter_next ( iter );
    }
  leveldb_iter_destroy ( iter );
  leveldb_readoptions_destroy ( readoptions );
}

void
bench ( void )
{
  uint64_t start;
  int i;
  // puts ( "start" );
  // leveldb_readoptions_t *readoptions = leveldb_readoptions_create ();
  // do_get ( readoptions );
  // puts ( "end" );

  i = SAMPLES;
  while ( i-- )
    {
      start = __rdtsc ();
      do_get ();
      samples[i] = __rdtsc () - start;
    }
  print ( samples, SAMPLES, "Benchmark GET", 0 );

  i = SAMPLES;
  while ( i-- )
    {
      start = __rdtsc ();
      leveldb_readoptions_t *readoptions = leveldb_readoptions_create ();
      do_scan ();
      samples[i] = __rdtsc () - start;
    }

  print ( samples, SAMPLES, "Benchmark SCAN", 0 );
}

int
main ( int argc, char **argv )
{
  const char *DBPath = ( argc == 2 ) ? argv[1] : DFL_DB_PATH;

  init_util ();
  pin_to_cpu ( 0 );

  printf ( "Cycles by us %u\n", cycles_by_us );

  leveldb_options_t *options = leveldb_options_create ();

  // open DB
  char *err = NULL;
  db = leveldb_open ( options, DBPath, &err );
  if ( err )
    {
      fprintf ( stderr,
                "Error to open database:\n%s\n"
                "Run: make -C ../database; ../database/create_db\n",
                err );
      return 1;
    }

  bench ();

  // cleanup
  // leveldb_readoptions_destroy ( readoptions );
  leveldb_options_destroy ( options );
  leveldb_close ( db );
}
