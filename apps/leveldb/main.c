
#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <leveldb/c.h>

#include "synergy.h"

static leveldb_t *db;

void
quit_handler (int sig)
{
  (void)sig;
  quit = true;
}

void
concord_enable (void)
{
}

void
concord_disable (void)
{
}

static void
do_get (char *key)
{
  size_t len;
  char *value, *err = NULL;

  leveldb_readoptions_t *readoptions = leveldb_readoptions_create ();

  value = leveldb_get (db, readoptions, key, strlen (key), &len, &err);
  free (value);
  free (err);

  leveldb_readoptions_destroy (readoptions);
}

static void
do_scan (void)
{
  const char *retr_key;
  size_t len;

  leveldb_readoptions_t *readoptions = leveldb_readoptions_create ();
  leveldb_iterator_t *iter = leveldb_create_iterator (db, readoptions);

  synergy_feedback_start_long ();

  leveldb_iter_seek_to_first (iter);
  while (leveldb_iter_valid (iter))
    {
      retr_key = leveldb_iter_key (iter, &len);
      (void)retr_key;
      leveldb_iter_next (iter);
    }

  synergy_feedback_finished_long ();

  leveldb_iter_destroy (iter);
  leveldb_readoptions_destroy (readoptions);
}

static void
handle_request (void *buff, uint16_t buff_len, struct sock sock)
{
#define GET 1
#define SCAN 2

  uint64_t *data = buff;
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

/* this is a green thread context */
static void
leveldb_server (void *arg)
{
  (void)arg;
  int len;
  struct sock sock;
  void *request;

  while (likely (!quit))
    {
      len = get_request (&request, &sock);
      if (likely (len > 0))
        {
          handle_request (request, len, sock);
        }
    }
}

static void
leveldb_init (void)
{
  // Initialize levelDB
  leveldb_options_t *options = leveldb_options_create ();

  // open DB
  char *err = NULL;
  const char *DBPath = "/tmpfs/my_db";
  db = leveldb_open (options, DBPath, &err);
  if (err)
    {
      fprintf (stderr,
               "Error to open database:\n%s\n"
               "Try: make database from root folder with 'make database'\n",
               err);
      exit (1);
    }
}

int
main (int argc, char **argv)
{
  puts ("Starting leveldb server");

  if (SIG_ERR == signal (SIGINT, quit_handler))
    fprintf (stderr, "%s\n", "Error to set sigint handler");

  leveldb_init ();
  synergy_init (argc, argv, leveldb_server, NULL);

  puts ("Closing DB");
  leveldb_close (db);

  return 0;
}
