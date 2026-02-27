
#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rocksdb/c.h>

#include "synergy.h"

static rocksdb_t *db;
static rocksdb_options_t *options;

void
quit_handler (int sig)
{
  (void)sig;
  quit = true;
}

static void
do_get (char *key)
{
  size_t len;
  char *value, *err = NULL;

  rocksdb_readoptions_t *ro = rocksdb_readoptions_create ();

  value = rocksdb_get (db, ro, key, strlen (key), &len, &err);
  free (value);
  free (err);

  rocksdb_readoptions_destroy (ro);
}

static void
do_scan (void)
{
  const char *retr_key;
  size_t len;

  rocksdb_readoptions_t *ro = rocksdb_readoptions_create ();
  rocksdb_iterator_t *iter = rocksdb_create_iterator (db, ro);
  synergy_feedback_start_long ();

  for (rocksdb_iter_seek_to_first (iter); rocksdb_iter_valid (iter);
       rocksdb_iter_next (iter))
    {
      retr_key = rocksdb_iter_key (iter, &len);
      (void)retr_key;
    }

  synergy_feedback_finished_long ();

  rocksdb_iter_destroy (iter);
  rocksdb_readoptions_destroy (ro);
}

static void
handle_request (void *buff, uint16_t buff_len, struct sock sock)
{
#define GET 1
#define SCAN 2

  uint64_t *data = (uint64_t *)buff;
  uint32_t type = data[3];
  uint64_t key = data[5];

  switch (type)
    {
    case GET:
      do_get ((char *)&key);
      break;
    case SCAN:
      do_scan ();
      break;
    default:
      assert (0 && "Invalid request type");
    }

  netio_send (data, buff_len, sock);
}

static void
rocksdb_server (void *arg)
{
  (void)arg;
  int len;
  struct sock sock;
  void *request;

  while (likely (!quit))
    {
      len = get_request (&request, &sock);
      if (likely (len > 0))
        handle_request (request, len, sock);
    }
}

static void
db_init (void)
{
  puts ("Starting rocksdb server");
  options = rocksdb_options_create ();
  rocksdb_options_set_allow_mmap_reads (options, 1);
  rocksdb_options_set_allow_mmap_writes (options, 1);
  rocksdb_slicetransform_t *prefix_extractor
      = rocksdb_slicetransform_create_fixed_prefix (8);
  rocksdb_options_set_prefix_extractor (options, prefix_extractor);
  rocksdb_options_set_plain_table_factory (options, 0, 10, 0.75, 3);
  // get RocksDB to perform well
  rocksdb_options_increase_parallelism (options, 0);
  rocksdb_options_optimize_level_style_compaction (options, 0);
  // create the DB if it's not already present
  // rocksdb_options_set_create_if_missing ( options, 1 );

  // You can also explicitly set these (optional/redundant):
  rocksdb_options_set_max_background_compactions (options, 0);
  rocksdb_options_set_max_background_flushes (options, 0);

  // Optional: Disable automatic compactions if desired
  rocksdb_options_set_disable_auto_compactions (options, 1);

  // open DB
  char *err = NULL;
  const char *DBPath = "/tmpfs/my_db";
  db = rocksdb_open (options, DBPath, &err);
  if (err)
    {
      fprintf (stderr,
               "Error to open database:\n%s\n"
               "Try: make database from root folder with 'make database'\n",
               err);
      exit (1);
    }
}

static void
db_free (void)
{
  puts ("Closing DB");
  // rocksdb_options_destroy (options);
  //  rocksdb_close (db);
}

int
main (int argc, char **argv)
{
  if (SIG_ERR == signal (SIGINT, quit_handler))
    fprintf (stderr, "%s\n", "Error to set sigint handler");

  db_init ();
  synergy_init (argc, argv, rocksdb_server, NULL);
  db_free ();

  return 0;
}
