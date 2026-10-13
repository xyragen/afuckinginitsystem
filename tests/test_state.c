#include "afi.h"
#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
int main(void) {
  afi_svc_t s;
  char err[256];
  char st[16];
  memset(&s, 0, sizeof(s));
  strcpy(s.name, "t");
  strcpy(s.exec, "/bin/sleep");
  strcpy(s.args, "30");
  strcpy(s.pidfile, "/tmp/afi_state_test.pid");
  s.timeout = 2;
  s.reload_sig = 0;
  unlink(s.pidfile);
  assert(afi_state_of(&s) == AFI_STOPPED);
  assert(afi_start_one(&s, err, sizeof(err)) == 0);
  assert(afi_state_of(&s) == AFI_RUNNING);
  afi_state_name(AFI_RUNNING, st, sizeof(st));
  assert(strcmp(st, "running") == 0);
  assert(afi_reload_one(&s, err, sizeof(err)) == 0);
  assert(afi_stop_one(&s, err, sizeof(err)) == 0);
  assert(afi_state_of(&s) == AFI_STOPPED);
  unlink(s.pidfile);
  strcpy(s.pidfile, "/tmp/afi_state_dead.pid");
  assert(afi_write_pid(s.pidfile, 4199999) == 0);
  assert(afi_state_of(&s) == AFI_FAILED);
  unlink(s.pidfile);
  assert(afi_sig_from_name("HUP") == SIGHUP);
  assert(afi_sig_from_name("TERM") == SIGTERM);
  assert(afi_sig_from_name("bogus") < 0);
  printf("test_state ok\n");
  return 0;
}
