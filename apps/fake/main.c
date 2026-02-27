
#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "synergy.h"
#include "fake_work.h"

//#define RESP

#ifdef RESP
#include "resp.h"
#endif

void
quit_handler (int sig)
{
  (void)sig;
  quit = true;
}

static void
do_short (uint32_t ns_sleep)
{
  fake_work_ns (ns_sleep);
}

static void
do_long (uint32_t ns_sleep)
{
  synergy_feedback_start_long ();
  fake_work_ns (ns_sleep);
  synergy_feedback_finished_long ();
}

static void
handle_request (void *buff, uint16_t buff_len, struct sock sock)
{
  uint64_t *data = buff;

#ifndef RESP
  // uint32_t type = data[3];
  uint32_t ns_sleep = data[4];

  if (ns_sleep < 10000)
    do_short (ns_sleep);
  else
    do_long (ns_sleep);

#else

  /* RESP PROTOCOL */
  struct resp_client resp;
  char *resp_request = (char *)&data[6];

  /* parse resp request */
  resp_decode (&resp, resp_request);

  /* get fields of request */
  char *cmd = resp.bs[0].string;
  unsigned cmd_size = resp.bs[0].size;
  unsigned ns_sleep = atounsigned (resp.bs[1].string);

  if (!strncmp (cmd, "LONG", cmd_size))
    do_long (ns_sleep);
  else
    do_short (ns_sleep);

#endif

  netio_send (data, buff_len, sock);
}

/* this is a green thread context */
static void
fake_work_server (void *arg)
{
  (void)arg;
  int len;
  struct sock sock;
  void *request;

  while (likely (!quit))
    {
      len = get_request (&request, &sock);
      if (len > 0)
        handle_request (request, len, sock);
    }
}

int
main (int argc, char **argv)
{
  if (SIG_ERR == signal (SIGINT, quit_handler))
    fprintf (stderr, "%s\n", "Error to set sigint handler");

  synergy_init (argc, argv, fake_work_server, NULL);

  return 0;
}
