#include "afi.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
  afi_set_t s;
  int idx[AFI_MAX_SERVICES];
  int depth[AFI_MAX_SERVICES];
  int n;
  memset(&s, 0, sizeof(s));
  strcpy(s.svcs[0].name, "syslog");
  strcpy(s.svcs[0].runlevels, "12345");
  strcpy(s.svcs[1].name, "net");
  strcpy(s.svcs[1].runlevels, "2345");
  strcpy(s.svcs[1].depends[0], "syslog");
  s.svcs[1].ndep = 1;
  strcpy(s.svcs[2].name, "cron");
  strcpy(s.svcs[2].runlevels, "2345");
  strcpy(s.svcs[2].depends[0], "syslog");
  s.svcs[2].ndep = 1;
  s.n = 3;
  n = afi_runlevel_filter(&s, "3", idx, AFI_MAX_SERVICES);
  assert(n == 3);
  n = afi_runlevel_filter(&s, "1", idx, AFI_MAX_SERVICES);
  assert(n == 1);
  n = afi_level_list(&s, '2', idx, AFI_MAX_SERVICES);
  assert(n == 3);
  assert(afi_level_ok(&s.svcs[1], '3'));
  assert(!afi_level_ok(&s.svcs[1], '1'));
  assert(afi_level_depth(&s, depth) == 0);
  assert(depth[0] == 0);
  assert(depth[1] == 1);
  assert(depth[2] == 1);
  printf("test_runlevel ok\n");
  return 0;
}
