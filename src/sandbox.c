
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sched.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/prctl.h>

#include "shell.h"

int do_init (sandbox_t * sandbox) {
  if (sandbox->flags & SANDBOX_DID_INIT) return 0;
  sandbox->flags |= SANDBOX_DID_INIT;

  const char * root = "/tmp/hello1";
  printf ("sandbox path '%s'\n", root);

  ok (unshare, CLONE_NEWNS | CLONE_NEWUSER);

  ok (mount, NULL, "/", NULL, MS_REC|MS_PRIVATE, NULL);

  sandbox->root = strdup (root);
  hin_create_dir_path (AT_FDCWD, sandbox->root, 0700);

  ok (chdir, sandbox->root);

  ok (prctl, PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);

  return 0;
}

int do_enter (sandbox_t * sandbox) {
  ok (mount, sandbox->root, "/", NULL, MS_RDONLY|MS_BIND|MS_NOSUID, NULL);
  ok (chdir, "/");
}

int do_mount (sandbox_t * sandbox, const char * source, const char * target, const char * fs, uint32_t flags, void * data) {
  do_init (sandbox);

  char * new = NULL;
  asprintf (&new, "%s/%s", sandbox->root, target);

  printf ("mount %s to %s\n", source, new);
  hin_create_dir_path (AT_FDCWD, new, 0700);
  ok (mount, source, new, fs, flags, data);
  free (new);
  return 0;
}

int do_mkdir (sandbox_t * sandbox, const char * path) {
  do_init (sandbox);

  char * new = NULL;
  asprintf (&new, "%s/%s", sandbox->root, path);

  printf ("mkdir %s to %s\n", path, new);
  hin_create_dir_path (AT_FDCWD, new, 0700);
  free (new);
  return 0;
}

