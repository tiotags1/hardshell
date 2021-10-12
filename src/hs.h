
#ifndef HS_SHELL_H
#define HS_SHELL_H

#include <stdint.h>

#define HS_VARNAME_PATTERN "%w_-"

enum {
HS_FLAG_INIT = 0x1,
};

enum {
HS_VAR_ENV = 0x1,
};

typedef struct {
  const char * root;
  uint32_t flags;
  void * env, * var;
  uint32_t debug;
} hs_shell_t;

int hs_do_enter (hs_shell_t * hs);
int hs_do_mount (hs_shell_t * hs, const char * source, const char * target, const char * fs, uint32_t flags, void * data);
int hs_do_mkdir (hs_shell_t * hs, const char * path);

#endif

