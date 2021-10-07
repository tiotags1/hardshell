
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#include <unistd.h>
#include <sys/mount.h>

#include <basic_pattern.h>

#include "shell.h"

static char * get_param (sandbox_t * sandbox, string_t * source) {
  match_string (source, "[ ]*");
  const char * ptr = source->ptr;
  const char * max = source->ptr + source->len;
  while (1) {
    if (ptr >= max) break;
    if (*ptr == ' ') break;
    if (*ptr == '\n') break;
    ptr++;
  }
  int len = ptr-source->ptr;
  if (len <= 0) return NULL;
  char * new = strndup (source->ptr, len);
  source->ptr += len;
  source->len -= len;
  match_string (source, "[ ]");
  return new;
}

static int match_comment (string_t * source) {
  const char * ptr = source->ptr;
  const char * max = source->ptr + source->len;
  if (*ptr != '#') return 0;
  while (ptr < max) {
    if (*ptr == '\n') { ptr++; break; }
    ptr++;
  }
  int len = ptr - source->ptr;
  printf ("found comment '%.*s'\n", len-1, source->ptr);
  source->ptr += len;
  source->len -= len;
  return len;
}

static int do_parse (sandbox_t * sandbox, string_t * source) {
  string_t orig = *source;
  match_string (source, "[ ]+");

  int used;
  char * path;

  if ((used = match_comment (source)) > 0) {
  } else if ((used = match_string (source, "ro ")) > 0) {
    while ((path = get_param (sandbox, source))) {
      do_mount (sandbox, path, path, NULL, MS_BIND|MS_NOSUID|MS_RDONLY, NULL);
      free (path);
    }

  } else if ((used = match_string (source, "rw ")) > 0) {
    while ((path = get_param (sandbox, source))) {
      do_mount (sandbox, path, path, NULL, MS_BIND|MS_NOSUID, NULL);
      free (path);
    }

  } else if ((used = match_string (source, "mkdir ")) > 0) {
    while ((path = get_param (sandbox, source))) {
      do_mkdir (sandbox, path);
      free (path);
    }

  } else if ((used = match_string (source, "devfs ")) > 0) {
    char * path = get_param (sandbox, source);
    if (path) {
      do_mount (sandbox, "/dev", path, NULL, MS_BIND|MS_REC, NULL);
      free (path);
    }

  } else if ((used = match_string (source, "procfs ")) > 0) {
    char * path = get_param (sandbox, source);
    if (path) {
      do_mount (sandbox, "/proc", path, NULL, MS_BIND|MS_REC, NULL);
      free (path);
    }

  } else if ((used = match_string (source, "tmpfs ")) > 0) {
    while ((path = get_param (sandbox, source))) {
      char * options = NULL;
      asprintf (&options, "size=%s,nr_inodes=16,mode=755", "1M");
      do_mount (sandbox, NULL, path, "tmpfs", MS_NOSUID|MS_NOEXEC|MS_NODEV, options);
      free (options);
      free (path);
    }

  } else if ((used = match_string (source, "shell\n")) > 0) {
    char * argv1[] = {"/bin/sh", NULL};
    int pid = ok (fork);
    if (pid == 0) {
      do_enter (sandbox);
      ok (execv, argv1[0], argv1);
    }
    waitpid (pid, NULL, 0);

  } else if ((used = match_string (source, "q\n")) > 0) {
    //ok (exit, 0);
    exit (0);
  }
  match_string (source, "[ \n]*");

  return source->ptr - orig.ptr;
}

int main (int argc, const char * argv[]) {
  sandbox_t * sandbox = calloc (1, sizeof (*sandbox));

  // read script or stdin
  int fd = 0;
  if (argc > 1) {
    const char * path = argv[1];
    printf ("loading config file '%s'\n", path);
    fd = ok (open, path, 0);
  }

  char * data = NULL;
  int sz = 0, inc = 4096, c = 0;
  int used;

  int register_signals ();
  register_signals ();

  do {
    sz += c;
    data = realloc (data, sz + inc);
    // process used

    string_t source;
    source.len = sz;
    source.ptr = data;
    match_string (&source, "%s*");

    used = do_parse (sandbox, &source);

    if (used > 0) {
      memmove (data, source.ptr, sz-used);
      sz -= used;
    }
  } while (used > 0 || (c = ok (read, fd, data+sz, inc)));

  printf ("master finished\n");
  return 0;
}

