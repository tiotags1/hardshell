
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "shell.h"

#include <signal.h>
#include <sys/wait.h>
#include <sys/prctl.h>

void handle_sigchld (int sig) {
  int saved_errno = errno;
  while (waitpid ((pid_t)(-1), 0, WNOHANG) > 0) {}
  errno = saved_errno;
}

int register_signals () {
  struct sigaction sa;
  sa.sa_handler = &handle_sigchld;
  sigemptyset (&sa.sa_mask);
  sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
  ok (sigaction, SIGCHLD, &sa, 0);

  ok (prctl, PR_SET_CHILD_SUBREAPER, 1, 0, 0, 0);
}

int checkreturn (int res, const char *name, const char * filename, int line) {
  if (res >= 0)
    return res;
  fprintf (stderr, "%s:%d %s: %s\n", filename, line, name, strerror (errno));
  exit(-1);
}

int hin_create_dir_path (int dirfd, const char * path, mode_t mode) {
  int fd = openat (dirfd, path, O_DIRECTORY, mode);
  if (fd >= 0) {
    close (fd);
    return 0;
  }
  if (errno != ENOENT) return -1;

  mode_t mask = 0;
  if (mode & 0600) mask |= 0100;
  if (mode & 060) mask |= 010;
  if (mode & 06) mask |= 01;

  const char * ptr = path;
  while (1) {
    if (ptr == NULL) return 0;
    if (*ptr == '/') {
      ptr++;
      continue;
    }
    char * dir_path;
    ptr = strchr (ptr, '/');
    if (ptr == NULL) {
      dir_path = strdup (path);
    } else {
      dir_path = strndup (path, ptr-path);
    }
    int err = mkdirat (dirfd, dir_path, mode | mask);
    if (err == 0) {
      printf ("created for '%s' directory '%s'\n", path, dir_path);
      free (dir_path);
      continue;
    }
    free (dir_path);

    switch (errno) {
    case ENOENT: break;
    case EEXIST: break;
    default:
      return -1;
    break;
    }
  }
}

