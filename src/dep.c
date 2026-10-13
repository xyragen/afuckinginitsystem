#include "afi.h"
#include <stdio.h>
#include <string.h>
int afi_dep_order(afi_set_t *set, int *order, int n, char *err, size_t errn) {
  int done[AFI_MAX_SERVICES];
  int out = 0;
  int i;
  int moved;
  if (!set) return -1;
  if (set->n == 0) return 0;
  for (i = 0; i < set->n; i++) done[i] = 0;
  for (i = 0; i < set->n; i++) {
    int j;
    for (j = 0; j < set->svcs[i].ndep; j++) {
      if (afi_set_find(set, set->svcs[i].depends[j]) < 0) {
        if (err) snprintf(err, errn, "unknown dep %s of %s", set->svcs[i].depends[j], set->svcs[i].name);
        return -1;
      }
    }
  }
  do {
    moved = 0;
    for (i = 0; i < set->n; i++) {
      int j;
      int ok;
      if (done[i]) continue;
      ok = 1;
      for (j = 0; j < set->svcs[i].ndep; j++) {
        int k = afi_set_find(set, set->svcs[i].depends[j]);
        if (k >= 0 && !done[k]) {
          ok = 0;
          break;
        }
      }
      if (ok) {
        done[i] = 1;
        if (order && out < n) order[out] = i;
        out++;
        moved = 1;
      }
    }
  } while (moved && out < set->n);
  if (out != set->n) {
    if (err) snprintf(err, errn, "dependency cycle detected");
    return -1;
  }
  if (!order) return 0;
  return out;
}
int afi_runlevel_filter(afi_set_t *set, const char *level, int *idx, int max) {
  int n = 0;
  int i;
  char lv = '3';
  if (!set) return 0;
  if (level && *level) lv = *level;
  for (i = 0; i < set->n; i++) {
    if (strchr(set->svcs[i].runlevels, lv)) {
      if (idx && n < max) idx[n] = i;
      n++;
    }
  }
  return n;
}
