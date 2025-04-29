
#pragma once

#include <stdio.h>

#define MAX_CMDS 4

struct bulk_string
{
  unsigned size;
  char *string;
};

struct resp_client
{
  unsigned tot_cmds;
  struct bulk_string bs[MAX_CMDS];
};

/*
 * This is a very simple and limited RESP protocol parser.
 * Only encode/decode client commands (i.e array of bulk strings).
 *
 * EX:
 *   resp_encode "GET key" to "2\r\n$3\r\nGET\r\n$3\r\nkey\r\n"
 *   resp_decode "2\r\n$3\r\nGET\r\n$3\r\nkey\r\n" to struct resp_client obj
 * */

static char *
get_next_token (char *request)
{
  while (*request++ != '\n')
    ;

  return request;
}

static unsigned
atounsigned (char *number)
{
  unsigned n = 0;

  while (*number >= '0' && *number <= '9')
    {
      n = (*number - '0') + (n * 10);
      number++;
    }

  return n;
}

/* only client requests */
static void
resp_decode (struct resp_client *buff, char *request)
{
  char *cur = request;

  assert (*cur == '*');
  cur++;
  unsigned tot_cmds = atounsigned (cur);
  assert (tot_cmds <= MAX_CMDS);
  cur = get_next_token (cur);

  buff->tot_cmds = tot_cmds;

  for (unsigned i = 0; i < tot_cmds; i++)
    {
      assert (*cur == '$');
      cur++;
      buff->bs[i].size = atounsigned (cur);
      cur = get_next_token (cur);
      buff->bs[i].string = cur;
      cur = get_next_token (cur);
    }
}

static int
resp_encode (char *buff, unsigned buff_size, char **cmd, unsigned cmd_count)
{
  char *p = buff;
  int ret = snprintf (p, buff_size, "*%d\r\n", cmd_count);

  /* buff truncated */
  if (ret >= (int)buff_size)
    return -1;

  p += ret;
  buff_size -= ret;

  for (unsigned i = 0; i < cmd_count; i++)
    {
      ret = snprintf (p, buff_size, "$%ld\r\n%s\r\n", strlen (cmd[i]), cmd[i]);

      /* buff truncated */
      if (ret >= (int)buff_size)
        return -1;

      p += ret;
      buff_size -= ret;
    }

  return p - buff;
}
