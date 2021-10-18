
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include "hs.h"
#include "hs_internal.h"

int main (int argc, const char * argv[], const char * envp[]) {
  hs_shell_t * hs = calloc (1, sizeof (*hs));
  //hs->debug = 0xffffffff;
  hs_do_init (hs);

  int hs_var_init (hs_shell_t * hs, const char * envp[]);
  hs_var_init (hs, envp);

  // read script or stdin
  int fd = 0;
  if (argc > 1) {
    const char * path = argv[1];
    printf ("loading config file '%s'\n", path);
    fd = ok (open, path, 0);
  }

  char * data = NULL;
  int count = 0, sz = 0, inc = 4096, c = 0;
  int used = 0;

  int hs_register_signals ();
  hs_register_signals ();

  while (1) {
    if (used == 0) {
      c = count + inc;
      if (c > sz) {
        sz = c;
        data = realloc (data, sz);
      }
      c = ok (read, fd, data + count, sz - count);
      if (c <= 0) break;
      count += c;
    }

    string_t source, command;
    source.ptr = data;
    source.len = count;
    used = 0;
    int hs_tokenize (hs_shell_t * hs, string_t * source, string_t * command, uint32_t flags);
    used = hs_tokenize (hs, &source, &command, 0);

    if (used > 0) {
      int hs_parse (hs_shell_t * hs, string_t * source);
      hs_parse (hs, &command);

      memmove (data, source.ptr, count-used);
      count -= used;
    }
  }

  return 0;
}

