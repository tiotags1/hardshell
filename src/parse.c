
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/wait.h>
#include <sys/mount.h>
#include <unistd.h>

#include "hs.h"
#include "hs_internal.h"

char * hs_parse_param (hs_shell_t * hs, string_t * source) {
  match_string (source, "[ ]*");
  int quotes = 0;
  const char * ptr = source->ptr;
  const char * max = source->ptr + source->len;
  if (*ptr == '"') {
    quotes = 1;
    ptr++;
  }
  while (1) {
    if (ptr >= max) break;
    if (quotes) {
      if (*ptr == '"') { ptr++; break; }
    } else {
      if (*ptr == ' ') break;
    }
    if (*ptr == '\n') break;
    ptr++;
  }
  int len = ptr-source->ptr;
  if (len <= 0) return NULL;
  char * new = strndup (source->ptr + quotes, len - quotes * 2);
  source->ptr += len;
  source->len -= len;
  match_string (source, "[ ]*");
  return new;
}

static int hs_parse_comment (hs_shell_t * hs, string_t * source) {
  const char * ptr = source->ptr;
  const char * max = source->ptr + source->len;
  if (*ptr != '#') return 0;
  while (ptr < max) {
    if (*ptr == '\n') { ptr++; break; }
    ptr++;
  }
  int len = ptr - source->ptr;
  if (hs->debug)
    printf ("found comment '%.*s'\n", len, source->ptr);
  source->ptr += len;
  source->len -= len;
  return len;
}

int hs_parse (hs_shell_t * hs, string_t * source) {
  string_t orig = *source, param1;
  match_string (source, "[ ]+");

  int used;
  char * path;

  if ((used = hs_parse_comment (hs, source)) > 0) {

  } else if ((used = match_string (source, "export%s+(["HS_VARNAME_PATTERN"]+)=", &param1)) > 0) {
    string_t out;
    hs_shell_expansion (hs, source, &out);
    hs_var_set (hs, HS_VAR_ENV, param1.ptr, param1.len, out.ptr, out.len);
    if (hs->debug)
      printf ("env '%.*s' = '%.*s'\n", (int)param1.len, param1.ptr, (int)out.len, out.ptr);
    free ((void*)out.ptr);

  } else if ((used = match_string (source, "(["HS_VARNAME_PATTERN"]+)=", &param1)) > 0) {
    string_t out;
    hs_shell_expansion (hs, source, &out);
    hs_var_set (hs, 0, param1.ptr, param1.len, out.ptr, out.len);
    if (hs->debug)
      printf ("var '%.*s' = '%.*s'\n", (int)param1.len, param1.ptr, (int)out.len, out.ptr);
    free ((void*)out.ptr);

  } else if ((used = match_string (source, "ro ")) > 0) {
    while ((path = hs_parse_param (hs, source))) {
      hs_do_mount (hs, path, path, NULL, MS_BIND|MS_NOSUID|MS_RDONLY, NULL);
      free (path);
    }

  } else if ((used = match_string (source, "rw ")) > 0) {
    while ((path = hs_parse_param (hs, source))) {
      hs_do_mount (hs, path, path, NULL, MS_BIND|MS_NOSUID, NULL);
      free (path);
    }

  } else if ((used = match_string (source, "mkdir ")) > 0) {
    while ((path = hs_parse_param (hs, source))) {
      hs_do_mkdir (hs, path);
      free (path);
    }

  } else if ((used = match_string (source, "devfs ")) > 0) {
    char * path = hs_parse_param (hs, source);
    if (path) {
      hs_do_mount (hs, "/dev", path, NULL, MS_BIND|MS_REC, NULL);
      free (path);
    }

  } else if ((used = match_string (source, "procfs ")) > 0) {
    char * path = hs_parse_param (hs, source);
    if (path) {
      hs_do_mount (hs, "/proc", path, NULL, MS_BIND|MS_REC, NULL);
      free (path);
    }

  } else if ((used = match_string (source, "tmpfs ")) > 0) {
    while ((path = hs_parse_param (hs, source))) {
      char * options = NULL;
      asprintf (&options, "size=%s,nr_inodes=16,mode=755", "1M");
      hs_do_mount (hs, NULL, path, "tmpfs", MS_NOSUID|MS_NOEXEC|MS_NODEV, options);
      free (options);
      free (path);
    }

  } else if ((used = match_string (source, "q")) > 0) {
    //ok (exit, 0);
    exit (0);

  } else if ((used = match_string (source, "(["HS_VARNAME_PATTERN"/]+)", &param1)) > 0) {
    char * path = strndup (param1.ptr, param1.len);
    int hs_exec (hs_shell_t * hs, const char * path, string_t * source);
    hs_exec (hs, path, source);
    free (path);
  }
  match_string (source, "[ \n]*");

  return source->ptr - orig.ptr;
}

int hs_tokenize (hs_shell_t * hs, string_t * source, string_t * command, uint32_t flags) {
  const char * ptr = source->ptr;
  const char * max = ptr + source->len;
  int skip = 0;
  for (;ptr < max; ptr++) {
    switch (*ptr) {
    case '\r':
    break;
    case '\0':
    case ';':
    case '\n':
      skip = 1;
      goto done;
    break;
    default:
      continue;
    }
  }
done:
  int num = (ptr - source->ptr);
  int used = num + skip;
  command->ptr = source->ptr;
  command->len = num;
  source->ptr += used;
  source->len -= used;
  used += match_string (source, "%s*");
  string_t out;
  hs_shell_expansion (hs, command, &out);
  *command = out;
  return used;
}


