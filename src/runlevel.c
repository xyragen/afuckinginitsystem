#include "afi.h"
#include <signal.h>
#include <stdio.h>
#include <string.h>
int afi_level_ok(afi_svc_t *s, char level) {
  if (!s || !s->runlevels[0]) return 1;
  return strchr(s->runlevels, level) != 0;
}
int afi_level_list(afi_set_t *set, char level, int *idx, int max) {
  char lv[2];
  lv[0] = level;
  lv[1] = 0;
  return afi_runlevel_filter(set, lv, idx, max);
}
int afi_level_depth(afi_set_t *set, int *depth) {
  int order[AFI_MAX_SERVICES];
  int n;
  int i;
  n = afi_dep_order(set, order, AFI_MAX_SERVICES, 0, 0);
  if (n < 0) return -1;
  for (i = 0; i < set->n; i++) depth[i] = 0;
  for (i = 0; i < n; i++) {
    int k = order[i];
    int j;
    int m = 0;
    for (j = 0; j < set->svcs[k].ndep; j++) {
      int d = afi_set_find(set, set->svcs[k].depends[j]);
      if (d >= 0 && depth[d] + 1 > m) m = depth[d] + 1;
    }
    depth[k] = m;
  }
  return 0;
}
