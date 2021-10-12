
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sched.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/prctl.h>

#include "hs.h"
#include "hs_internal.h"

static int hs_do_init (hs_shell_t * hs) {
  if (hs->flags & HS_FLAG_INIT) return 0;
  hs->flags |= HS_FLAG_INIT;

  const char * root = "/tmp/hello1";
  printf ("sandbox path '%s'\n", root);

  ok (unshare, CLONE_NEWNS | CLONE_NEWUSER);

  ok (mount, NULL, "/", NULL, MS_REC|MS_PRIVATE, NULL);

  hs->root = strdup (root);
  hs_create_dir_path (AT_FDCWD, hs->root, 0700);

  ok (chdir, hs->root);

  ok (prctl, PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);

  return 0;
}

int hs_do_enter (hs_shell_t * hs) {
  printf ("entering %s\n", hs->root);
  ok (mount, hs->root, "/", NULL, MS_BIND|MS_NOSUID, NULL);
  ok (chdir, "/");
  return 0;
}

int hs_do_mount (hs_shell_t * hs, const char * source, const char * target, const char * fs, uint32_t flags, void * data) {
  hs_do_init (hs);

  char * new = NULL;
  asprintf (&new, "%s/%s", hs->root, target);

  printf ("mount %s to %s\n", source, new);
  hs_create_dir_path (AT_FDCWD, new, 0700);
  ok (mount, source, new, fs, flags, data);
  free (new);
  return 0;
}

int hs_do_mkdir (hs_shell_t * hs, const char * path) {
  hs_do_init (hs);

  char * new = NULL;
  asprintf (&new, "%s/%s", hs->root, path);

  printf ("mkdir %s to %s\n", path, new);
  hs_create_dir_path (AT_FDCWD, new, 0700);
  free (new);
  return 0;
}

