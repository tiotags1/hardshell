
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hs.h"
#include "hs_internal.h"

int hs_var_init (hs_shell_t * hs, const char * envp[]) {
  basic_ht_t * env_ht = basic_ht_create (1024, 123);
  basic_ht_t * var_ht = basic_ht_create (1024, 125);
  hs->env = env_ht;
  hs->var = var_ht;

  for (const char * const * ptr = envp; *ptr; ptr++) {
    string_t source, name;
    source.ptr = (char*)*ptr;
    source.len = strlen (source.ptr);
    int num = match_string (&source, "([^=]+)=", &name);
    if (num <= 0) { continue; }
    hs_var_set (hs, HS_VAR_ENV, name.ptr, name.len, source.ptr, source.len);
    if (hs->debug)
      printf ("env is '%.*s' = '%.*s'\n", (int)name.len, name.ptr, (int)source.len, source.ptr);
  }
  return 0;
}

int hs_var_set (hs_shell_t * shell, uint32_t flags, const char * name, int name_len, const char * val, int val_len) {
  basic_ht_t * env = shell->env;
  basic_ht_t * var = shell->var;
  basic_ht_t * ht = env;

  basic_ht_hash_t h1 = 0, h2 = 0;
  basic_ht_hash (name, name_len, ht->seed, &h1, &h2);
  // TODO check previous value and release data
  basic_ht_pair_t * pair = basic_ht_get_pair (ht, h1, h2);
  if (pair == NULL && (flags & HS_VAR_ENV) == 0) {
    ht = var;
    pair = basic_ht_get_pair (ht, h1, h2);
    basic_ht_hash (name, name_len, ht->seed, &h1, &h2);
  }
  char * v1 = strndup (name, name_len);
  char * v2 = strndup (val, val_len);
  if (pair == NULL) {
    basic_ht_set_pair (ht, h1, h2, (uintptr_t)v1, (uintptr_t)v2);
    return 0;
  }
  free ((void*)pair->value1);
  free ((void*)pair->value2);
  pair->value1 = (uintptr_t)v1;
  pair->value2 = (uintptr_t)v2;
  return 0;
}

const char * hs_var_get (hs_shell_t * hs, uint32_t flags, const char * name, int name_len) {
  basic_ht_t * ht;
  basic_ht_pair_t * pair;
  basic_ht_hash_t h1 = 0, h2 = 0;

  ht = hs->var;
  basic_ht_hash (name, name_len, ht->seed, &h1, &h2);
  pair = basic_ht_get_pair (ht, h1, h2);
  if (pair) return (const char *)pair->value2;

  ht = hs->env;
  basic_ht_hash (name, name_len, ht->seed, &h1, &h2);
  pair = basic_ht_get_pair (ht, h1, h2);
  if (pair) return (const char *)pair->value2;

  return NULL;
}

static int hs_parse_between (string_t * source, string_t * out, char what) {
  const char * ptr = source->ptr;
  const char * max = ptr + source->len;
  if (source->len == 0) return 0;
  for (; ptr < max; ptr++) {
    if (*ptr == what) {
      //ptr--;
      break;
    }
  }
  int len = ptr - source->ptr;
  out->ptr = source->ptr;
  out->len = len;
  source->ptr += len;
  source->len -= len;
  return len;
}

int hs_shell_expansion (hs_shell_t * hs, string_t * source, string_t * out) {
  if (hs->debug)
    printf ("expanding %d '%.*s'\n", (int)source->len, (int)source->len, source->ptr);

  int len = 0, ret, pos;
  string_t orig = *source, param1;
  while ((ret = hs_parse_between (source, &param1, '$')) > 0) {
    len += ret;
    ret = match_string (source, "$(["HS_VARNAME_PATTERN"]+)", &param1);
    if (ret <= 0) { continue; }
    const char * val = hs_var_get (hs, 0, param1.ptr, param1.len);
    if (val == NULL) { continue; }
    len += strlen (val);
  }

  char * new = malloc (len + 1);
  new[len] = '\0';
  *source = orig;
  pos = 0;
  while ((ret = hs_parse_between (source, &param1, '$')) > 0) {
    memcpy (&new[pos], param1.ptr, param1.len);
    pos += param1.len;
    ret = match_string (source, "$(["HS_VARNAME_PATTERN"]+)", &param1);
    if (ret <= 0) { continue; }
    const char * val = hs_var_get (hs, 0, param1.ptr, param1.len);
    if (val == NULL) continue;
    int len1 = strlen (val);
    memcpy (&new[pos], val, len1);
    pos += len1;
  }

  out->ptr = new;
  out->len = len;

  if (hs->debug)
    printf ("expanded  %d '%.*s'\n", (int)out->len, (int)out->len, out->ptr);

  return out->len;
}


