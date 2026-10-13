#include "afi.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
  afi_set_t s;
  int order[AFI_MAX_SERVICES];
  int n;
  char err[256];
  memset(&s, 0, sizeof(s));
  strcpy(s.svcs[0].name, "net");
  strcpy(s.svcs[0].exec, "/bin/sleep");
  strcpy(s.svcs[1].name, "fs");
  strcpy(s.svcs[1].exec, "/bin/sleep");
  strcpy(s.svcs[2].name, "app");
  strcpy(s.svcs[2].exec, "/bin/sleep");
  strcpy(s.svcs[2].depends[0], "net");
  strcpy(s.svcs[2].depends[1], "fs");
  s.svcs[2].ndep = 2;
  s.n = 3;
  err[0] = 0;
  n = afi_dep_order(&s, order, AFI_MAX_SERVICES, err, sizeof(err));
  assert(n == 3);
  assert(order[2] == 2);
  err[0] = 0;
  assert(afi_validate(&s, err, sizeof(err)) == 0);
  strcpy(s.svcs[0].depends[0], "app");
  s.svcs[0].ndep = 1;
  err[0] = 0;
  n = afi_dep_order(&s, order, AFI_MAX_SERVICES, err, sizeof(err));
  assert(n < 0);
  s.svcs[0].ndep = 0;
  strcpy(s.svcs[1].depends[0], "nope");
  s.svcs[1].ndep = 1;
  err[0] = 0;
  assert(afi_dep_order(&s, order, AFI_MAX_SERVICES, err, sizeof(err)) < 0);
  printf("test_dep ok\n");
  return 0;
}
