#include "afi.h"
#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
int main(void) {
  pid_t p;
  assert(afi_sup_count() == 0);
  assert(afi_sup_add(1234, "web") == 0);
  assert(afi_sup_add(5678, "db") == 0);
  assert(afi_sup_count() == 2);
  assert(strcmp(afi_sup_get(1234), "web") == 0);
  assert(afi_sup_get(9999) == 0);
  assert(afi_sup_add(1234, "web2") == 0);
  assert(strcmp(afi_sup_get(1234), "web2") == 0);
  assert(afi_sup_count() == 2);
  assert(afi_sup_del(1234) == 0);
  assert(afi_sup_count() == 1);
  assert(afi_sup_del(9999) != 0);
  assert(afi_sup_del(5678) == 0);
  assert(afi_sup_count() == 0);
  assert(afi_trim(0) != 0);
  assert(!afi_pid_alive(-1));
  assert(!afi_pid_alive(4199999));
  assert(afi_read_pid("/nonexistent-afi-pid", &p) != 0);
  assert(afi_spawn(0, 0) < 0);
  assert(afi_spawn("", 0) < 0);
  assert(afi_stop_pid(-1, 1) != 0);
  assert(afi_stop_pid(4199999, 1) == 0);
  printf("test_util ok\n");
  return 0;
}
