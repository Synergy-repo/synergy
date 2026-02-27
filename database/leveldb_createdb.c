
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#include "leveldb/c.h"

#define DFL_NUM_ENTRIES 15000
#define DFL_DB_PATH "/tmpfs/my_db"

/*
 * argv[1] num_entries
 * argv[2] DB path */
int
main ( int argc, char **argv )
{
  int num_entries = ( argc >= 2 ) ? atoi ( argv[1] ) : DFL_NUM_ENTRIES;
  const char *DBPath = ( argc == 3 ) ? argv[2] : DFL_DB_PATH;

  leveldb_t *db;
  leveldb_options_t *options = leveldb_options_create ();
  leveldb_options_set_create_if_missing ( options, 1 );

  // open DB
  char *err = NULL;
  db = leveldb_open ( options, DBPath, &err );
  if ( err )
    {
      fprintf ( stderr, "%s\n", err );
      exit ( 1 );
    }

  char key[16], value[16], *value_ret;
  size_t len;

  // Put key-value
  leveldb_writeoptions_t *writeoptions = leveldb_writeoptions_create ();
  for ( int i = 0; i < num_entries; i++ )
    {
      snprintf ( key, sizeof key, "k%d", i );
      snprintf ( value, sizeof value, "v%d", i );
      leveldb_put ( db,
                    writeoptions,
                    key,
                    strlen ( key ),
                    value,
                    strlen ( value ) + 1,
                    &err );
      assert ( !err );
    }

   //test get value
   leveldb_readoptions_t *readoptions = leveldb_readoptions_create ();
   for ( int i = 0; i < num_entries; i++ )
    {
      snprintf ( key, sizeof key, "k%d", i );
      value_ret =
              leveldb_get ( db, readoptions, key, strlen ( key ), &len, &err
              );
      assert ( !err );
      snprintf ( value, sizeof value, "v%d", i );
      assert ( strcmp ( value, value_ret ) == 0 );
      //printf ( "key:%s value:%s\n", key, value_ret );
    }

  // test iterator
  // leveldb_iterator_t *iter = leveldb_create_iterator ( db, readoptions );
  // leveldb_iter_seek_to_first ( iter );
  // while ( leveldb_iter_valid ( iter ) )
  //  {
  //    const char *ikey = leveldb_iter_key ( iter, &len );
  //    value_ret = leveldb_get ( db, readoptions, ikey, len, &len, &err );

  //    assert ( !err );
  //    //printf ( "key:%s value:%s\n", ikey, value_ret );
  //    leveldb_iter_next ( iter );
  //  }
  // leveldb_readoptions_destroy ( readoptions );

  // cleanup
   leveldb_writeoptions_destroy ( writeoptions );
   leveldb_options_destroy ( options );
   leveldb_close ( db );

   printf ( "Database created on %s with %d entries\n", DBPath, num_entries );

  return 0;
}
