
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

int
resp_encode ( char *buff, unsigned buff_size, char **cmd, unsigned cmd_count );

void
resp_decode ( struct resp_client *buff, char *request );
