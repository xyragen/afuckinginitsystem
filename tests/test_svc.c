#include "afi.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
int main(void) {
  afi_svc_t s;
  char err[256];
  FILE *f;
  memset(&s, 0, sizeof(s));
  f = fopen("/tmp/afi_test.svc", "w");
  assert(f);
  fprintf(f, "name = web\nboguskey = x\n");
  fclose(f);
  assert(afi_svc_parse("/tmp/afi_test.svc", &s, err, sizeof(err)) != 0);
  f = fopen("/tmp/afi_test.svc", "w");
  assert(f);
  fprintf(f, "name = web\ndesc = web server\nexec = /bin/sleep\nargs = 30\npidfile = /tmp/afi_web.pid\ndepends = net, fs\nrunlevels = 2345\nrespawn = yes\ntimeout = 3\nreload_sig = HUP\nuser = root\ndir = /tmp\nenv = FOO=bar\nenv = BAZ=qux\nready = 2\n");
  fclose(f);
  assert(afi_svc_parse("/tmp/afi_test.svc", &s, err, sizeof(err)) == 0);
  assert(strcmp(s.name, "web") == 0);
  assert(strcmp(s.exec, "/bin/sleep") == 0);
  assert(s.ndep == 2);
  assert(s.respawn == 1);
  assert(s.timeout == 3);
  assert(s.reload_sig == 1);
  assert(strcmp(s.user, "root") == 0);
  assert(strcmp(s.dir, "/tmp") == 0);
  assert(s.nenv == 2);
  assert(s.ready == 2);
  f = fopen("/tmp/afi_test.svc", "w");
  assert(f);
  fprintf(f, "name = web\nexec = /bin/sleep\nenv = NOEQUALS\n");
  fclose(f);
  assert(afi_svc_parse("/tmp/afi_test.svc", &s, err, sizeof(err)) != 0);
  unlink("/tmp/afi_test.svc");
  printf("test_svc ok\n");
  return 0;
}
