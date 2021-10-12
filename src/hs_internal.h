
#ifndef HS_INTERNAL_H
#define HS_INTERNAL_H

#include "hs.h"

#include <basic_pattern.h>
#include <basic_hashtable.h>

int hs_var_set (hs_shell_t * shell, uint32_t flags, const char * name, int name_len, const char * val, int val_len);
const char * hs_var_get (hs_shell_t * hs, uint32_t flags, const char * name, int name_len);

int hs_shell_expansion (hs_shell_t * hs, string_t * source, string_t * out);

#define ok(fname, arg...) checkreturn(fname(arg), #fname, __FILE__, __LINE__)
int checkreturn (int res, const char *name, const char * filename, int line);

int hs_create_dir_path (int dirfd, const char * path, mode_t mode);

#endif

