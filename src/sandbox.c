
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sched.h>
#include <unistd.h>
#include <sys/capability.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>

#include "hs.h"
#include "hs_internal.h"

int pivot_root(const char *new_root, const char *put_old);

int hs_do_init (hs_shell_t * hs) {
  /*
  // disallow gdb, strace and others from attaching via PTRACE_ATTACH
  if (0 > prctl(PR_SET_DUMPABLE, 0)) {
    perror("can't prctl(PR_SET_DUMPABLE)");
    return 1;
  }
  // if yama enabled then
  prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY, ...)
  */

  hs->unshare_flags = CLONE_NEWNS | CLONE_NEWUSER | CLONE_NEWPID;
  hs->new_uid = hs->new_gid = 1000;
  hs->workdir = strdup ("/");
}

static int hs_do_start (hs_shell_t * hs) {
  if (hs->flags & HS_FLAG_INIT) return 0;
  hs->flags |= HS_FLAG_INIT;

  hs->old_uid = getuid ();
  hs->old_gid = getgid ();

  ok (unshare, hs->unshare_flags);

  ok (mount, NULL, "/", NULL, MS_REC|MS_PRIVATE, NULL);

  if (hs->root == NULL) {
    hs->root = strdup ("/tmp/hello1");
  }
  printf ("sandbox path '%s'\n", hs->root);
  hs_create_dir_path (AT_FDCWD, hs->root, 0700);

  ok (mount, hs->root, hs->root, NULL, MS_BIND|MS_NOSUID, NULL);

  ok (chdir, hs->root);

  ok (prctl, PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);

  rmdir(".oldproc");
  rmdir("proc");
  ok(mkdir, ".oldproc", 0755);
  ok(mkdir, "proc", 0755);
  /* we need to hang on to the old proc in order to mount our
   * new proc later on
   */
  ok(mount, "/proc", ".oldproc", NULL, MS_BIND|MS_REC, NULL);

  return 0;
}

int hs_do_enter (hs_shell_t * hs) {
  printf ("entering %s\n", hs->root);

  int newuid = hs->new_uid;
  int newgid = hs->new_gid;

  char buf[1024];
  int fd, len;
  fd = ok (open, "/proc/self/uid_map", O_WRONLY);

  len = snprintf (buf, sizeof (buf), "%d %d 1\n", newuid, hs->old_uid);
  ok (write, fd, buf, len);

  ok (close, fd);

  /* must disallow setgroups() before writing to gid_map on
   * versions of linux with this feature:
   */
  if ((fd = open ("/proc/self/setgroups", O_WRONLY)) >= 0) {
    ok (write, fd, "deny", 4);
    ok (close, fd);
  }

  fd = ok (open, "/proc/self/gid_map", O_WRONLY);

  len = snprintf (buf, sizeof (buf), "%d %d 1\n", newgid, hs->old_gid);
  ok (write, fd, buf, len);

  ok (close, fd);

  /* initially we're nobody, change to newgid/newuid */
  ok (setresgid, newgid, newgid, newgid);
  ok (setresuid, newuid, newuid, newuid);

  rmdir (".oldroot");
  ok (mkdir, ".oldroot", 0755);
  ok (pivot_root, ".", ".oldroot");
  ok (umount2, ".oldroot", MNT_DETACH);
  ok (rmdir, ".oldroot");

  ok (umount2, "/.oldproc", MNT_DETACH);
  ok (rmdir, ".oldproc");

  ok (mount, "/", "/", NULL, MS_RDONLY|MS_BIND|MS_NOSUID|MS_REMOUNT, NULL);

  ok (cap_set_mode, CAP_MODE_NOPRIV);

  ok (chdir, hs->workdir);

  return 0;
}

int hs_do_mount (hs_shell_t * hs, const char * source, const char * target, const char * fs, uint32_t flags, void * data) {
  hs_do_start (hs);

  char * new = NULL;
  char * ntarget = target;
  if (*ntarget == '/') ntarget++;
  asprintf (&new, "%s/%s", hs->root, ntarget);

  printf ("mount %s to %s\n", source, new);
  hs_create_dir_path (AT_FDCWD, source, 0700);
  hs_create_dir_path (AT_FDCWD, new, 0700);
  ok (mount, source, new, fs, flags, data);
  free (new);
  return 0;
}

int hs_do_mkdir (hs_shell_t * hs, const char * path) {
  hs_do_start (hs);

  char * new = NULL;
  asprintf (&new, "%s/%s", hs->root, path);

  printf ("mkdir %s to %s\n", path, new);
  hs_create_dir_path (AT_FDCWD, new, 0700);
  free (new);
  return 0;
}

