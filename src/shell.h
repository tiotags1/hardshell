
#ifndef BASIC_SHELL_H
#define BASIC_SHELL_H

#include <stdint.h>

/*
#include <basic_hashtable.h>

#define BASIC_SHELL_VARNAME "%w_-"

int set_variable (basic_ht_t * ht, const char * name, int name_len, const char * val, int val_len);

int basic_shell_expansion (string_t * source, string_t * out);
*/

enum { SANDBOX_DID_INIT = 1 };

typedef struct {
  const char * root;
  uint32_t flags;
} sandbox_t;

int do_mkdir (sandbox_t * sandbox, const char * path);
int do_mount (sandbox_t * sandbox, const char * source, const char * target, const char * fs, uint32_t flags, void * data);
int do_enter (sandbox_t * sandbox);

#define ok(fname, arg...) checkreturn(fname(arg), #fname, __FILE__, __LINE__)
int checkreturn (int res, const char *name, const char * filename, int line);

int hin_create_dir_path (int dirfd, const char * path, mode_t mode);

#endif

