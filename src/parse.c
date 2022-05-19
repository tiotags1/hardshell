
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sched.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <unistd.h>

#include "hs.h"
#include "hs_internal.h"

char * hs_parse_param (hs_shell_t * hs, string_t * source) {
  if (source->len <= 0) return NULL;
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

static int hs_parse_share_tag (const char * path) {
  if (strcmp (path, "net") == 0) {
    return CLONE_NEWNET;
  } else if (strcmp (path, "ipc") == 0) {
    return CLONE_NEWIPC;
  } else if (strcmp (path, "pid") == 0) {
    return CLONE_NEWPID;
  } else if (strcmp (path, "uts") == 0) {
    return CLONE_NEWUTS;
  //} else if (strcmp (path, "time") == 0) {
  //  hs->unshare_flags |= CLONE_NEWTIME;
  } else if (strcmp (path, "sysvsem") == 0) {
    return CLONE_SYSVSEM;
  } else if (strcmp (path, "cgroup") == 0) {
    return CLONE_NEWCGROUP;
  } else {
    printf ("unknown unshare flag '%s'\n", path);
    exit (1);
  }
}

#include <stdarg.h>

char * hs_asprintf (const char * fmt, ...) {
  va_list ap;
  va_start (ap, fmt);
  char * new = NULL;
  int ret = vasprintf (&new, fmt, ap);
  va_end (ap);
  return new;
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

  } else if ((used = match_string (source, "ro%s+")) > 0) {
    while ((path = hs_parse_param (hs, source))) {
      hs_do_mount (hs, path, path, NULL, MS_BIND|MS_NOSUID|MS_RDONLY, NULL);
      free (path);
    }

  } else if ((used = match_string (source, "rw%s+")) > 0) {
    while ((path = hs_parse_param (hs, source))) {
      hs_do_mount (hs, path, path, NULL, MS_BIND|MS_NOSUID, NULL);
      free (path);
    }

  } else if ((used = match_string (source, "mount_run%s+")) > 0) {
    while ((path = hs_parse_param (hs, source))) {
      char * path1 = path;
      if (*path1 == '/') path1++;
      char * new1 = hs_asprintf ("/run/%s", path1);
      hs_do_mount (hs, new1, new1, NULL, MS_BIND|MS_NOSUID, NULL);
      free (path);
      free (new1);
    }

  } else if ((used = match_string (source, "mount_user_run%s+")) > 0) {
    while ((path = hs_parse_param (hs, source))) {
      char * path1 = path;
      if (*path1 == '/') path1++;
      char * new1 = hs_asprintf ("/run/user/%d/%s", hs->old_uid, path1);
      char * new2 = hs_asprintf ("/run/user/%d/%s", hs->new_uid, path1);
      hs_do_mount (hs, new1, new2, NULL, MS_BIND|MS_NOSUID, NULL);
      free (path);
      free (new1);
      free (new2);
    }

  } else if ((used = match_string (source, "mount%s+")) > 0) {
    char * src = hs_parse_param (hs, source);
    char * trg = hs_parse_param (hs, source);
    if (src && trg) {
      hs_do_mount (hs, src, trg, NULL, MS_BIND|MS_NOSUID, NULL);
    }
    if (src) free (src);
    if (trg) free (trg);

  } else if (match_string (source, "cd%s+") > 0) {
    if ((path = hs_parse_param (hs, source))) {
      hs->workdir = path;
    }

  } else if ((used = match_string (source, "mkdir%s+")) > 0) {
    while ((path = hs_parse_param (hs, source))) {
      hs_do_mkdir (hs, path);
      free (path);
    }

  } else if ((used = match_string (source, "devfs%s+")) > 0) {
    char * path = hs_parse_param (hs, source);
    if (path) {
      hs_do_mount (hs, "/dev", path, NULL, MS_BIND|MS_REC, NULL);
      free (path);
    }

  } else if ((used = match_string (source, "procfs%s+")) > 0) {
    char * path = hs_parse_param (hs, source);
    if (path) {
      hs_do_mount (hs, "/proc", path, NULL, MS_BIND|MS_REC, NULL);
      free (path);
    }

  } else if ((used = match_string (source, "sysfs%s+")) > 0) {
    char * path = hs_parse_param (hs, source);
    if (path) {
      hs_do_mount (hs, "/sys", path, NULL, MS_BIND|MS_REC, NULL);
      free (path);
    }

  } else if ((used = match_string (source, "newid%s+")) > 0) {
    char * path = hs_parse_param (hs, source);
    if (path) {
      hs->new_uid = atoi (path);
      hs->new_gid = hs->new_uid;
      free (path);
    }

  } else if ((used = match_string (source, "setroot%s+")) > 0) {
    char * path = hs_parse_param (hs, source);
    if (path) {
      hs->root = path;
    }

  } else if ((used = match_string (source, "tmpfs%s+")) > 0) {
    while ((path = hs_parse_param (hs, source))) {
      char * options = NULL;
      asprintf (&options, "size=%s,nr_inodes=16,mode=755", "1M");
      hs_do_mount (hs, NULL, path, "tmpfs", MS_NOSUID|MS_NOEXEC|MS_NODEV, options);
      free (options);
      free (path);
    }

  } else if ((used = match_string (source, "unshare%s+")) > 0) {
    while ((path = hs_parse_param (hs, source))) {
      hs->unshare_flags |= hs_parse_share_tag (path);
      free (path);
    }

  } else if ((used = match_string (source, "share%s+")) > 0) {
    while ((path = hs_parse_param (hs, source))) {
      hs->unshare_flags &= ~hs_parse_share_tag (path);
      free (path);
    }

  } else if ((used = match_string_equal (source, "exit%s*")) > 0) {
    exit (0);

  } else if ((used = match_string_equal (source, "q%s*")) > 0) {
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
  const char * cmd = NULL;
  int skip = 0;
  for (;ptr < max; ptr++) {
    switch (*ptr) {
    case '\r':
    break;
    case '\0':
    case ';':
    case '\n':
      skip = 1;
      cmd = ptr;
      goto done;
    break;
    case '&':
      hs->flags |= HS_FLAG_DONT_WAIT;
      skip = 1;
      cmd = ptr-1;
      goto done;
    break;
    case '>': {
      if (*(ptr+1) == '>') {
        ptr++;
        hs->flags |= HS_FLAG_APPEND;
      }
      cmd = ptr-1;
      string_t new, param;
      new.ptr = ptr+1;
      new.len = max - new.ptr;
      int n = match_string (&new, "%s*([%w/]+)%s*", &param);
      if (n < 0) {
        printf ("syntax error\n");
      }
      if (hs->debug)
        printf ("redirect to '%.*s'\n", param.len, param.ptr);
      char * path = strndup (param.ptr, param.len);
      int flags = O_CLOEXEC | O_CREAT | O_WRONLY;
      if (hs->flags & HS_FLAG_APPEND) {
        flags |= O_APPEND;
      } else {
        flags |= O_TRUNC;
      }
      hs->out_fd = ok (open, path, flags, 0777);
      ptr = new.ptr;
      goto done;
    break; }
    default:
      continue;
    }
  }
done:
  int num = (ptr - source->ptr);
  int used = num + skip;
  command->ptr = source->ptr;
  command->len = (cmd - source->ptr);
  source->ptr += used;
  source->len -= used;
  used += match_string (source, "%s*");
  string_t out;
  hs_shell_expansion (hs, command, &out);
  *command = out;
  //if (hs->debug)
  //  printf ("command was '%.*s'\n", command->len, command->ptr);
  return used;
}


