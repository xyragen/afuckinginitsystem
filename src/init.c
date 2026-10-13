#include "afi.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
static volatile sig_atomic_t afi_got_chld = 0;
static volatile sig_atomic_t afi_got_term = 0;
static volatile sig_atomic_t afi_got_hup = 0;
static volatile sig_atomic_t afi_got_usr1 = 0;
static volatile sig_atomic_t afi_got_usr2 = 0;
static char afi_dir[1024] = "services";
static char afi_level = '3';
static void afi_on_chld(int s) {
  (void)s;
  afi_got_chld = 1;
}
static void afi_on_term(int s) {
  (void)s;
  afi_got_term = 1;
}
static void afi_on_hup(int s) {
  (void)s;
  afi_got_hup = 1;
}
static void afi_on_usr1(int s) {
  (void)s;
  afi_got_usr1 = 1;
}
static void afi_on_usr2(int s) {
  (void)s;
  afi_got_usr2 = 1;
}
static long afi_now(void) {
  return (long)time(0);
}
typedef struct {
  int fails;
  long last;
} afi_hist_t;
static afi_hist_t afi_hist[AFI_MAX_SERVICES];
static void afi_reap(afi_set_t *set) {
  pid_t pid;
  int st;
  while ((pid = waitpid(-1, &st, WNOHANG)) > 0) {
    const char *nm = afi_sup_get(pid);
    int i = -1;
    if (nm) i = afi_set_find((afi_set_t *)set, nm);
    afi_sup_del(pid);
    if (i >= 0) {
      afi_svc_t *s = &((afi_set_t *)set)->svcs[i];
      unlink(s->pidfile);
      if (s->respawn && !afi_got_term && !afi_got_usr1 && !afi_got_usr2) {
        long now = afi_now();
        if (now - afi_hist[i].last > 60) afi_hist[i].fails = 0;
        afi_hist[i].last = now;
        afi_hist[i].fails++;
        if (afi_hist[i].fails <= 5) {
          char err[256];
          afi_start_one(s, err, sizeof(err));
        }
      }
    }
  }
}
static int afi_boot(afi_set_t *set) {
  int order[AFI_MAX_SERVICES];
  int depth[AFI_MAX_SERVICES];
  int started[AFI_MAX_SERVICES];
  int maxd = 0;
  int n;
  int d;
  int i;
  char err[512];
  n = afi_dep_order(set, order, AFI_MAX_SERVICES, err, sizeof(err));
  if (n < 0) {
    fprintf(stderr, "afi-init: %s\n", err);
    return -1;
  }
  for (i = 0; i < set->n; i++) {
    depth[i] = 0;
    started[i] = 0;
  }
  for (i = 0; i < n; i++) {
    int k = order[i];
    int j;
    int m = 0;
    for (j = 0; j < set->svcs[k].ndep; j++) {
      int x = afi_set_find(set, set->svcs[k].depends[j]);
      if (x >= 0 && depth[x] + 1 > m) m = depth[x] + 1;
    }
    depth[k] = m;
    if (m > maxd) maxd = m;
  }
  for (d = 0; d <= maxd; d++) {
    int count = 0;
    for (i = 0; i < n; i++) {
      int k = order[i];
      afi_svc_t *s = &set->svcs[k];
      int j;
      int ok;
      if (depth[k] != d) continue;
      if (!strchr(s->runlevels, afi_level)) continue;
      ok = 1;
      for (j = 0; j < s->ndep; j++) {
        int x = afi_set_find(set, s->depends[j]);
        if (x >= 0 && !started[x]) {
          ok = 0;
          break;
        }
      }
      if (!ok) {
        fprintf(stderr, "afi-init: %s: dependency not started\n", s->name);
        continue;
      }
      err[0] = 0;
      if (afi_start_one(s, err, sizeof(err)) != 0) {
        if (err[0]) fprintf(stderr, "afi-init: %s\n", err);
        continue;
      }
      started[k] = 1;
      count++;
    }
    if (count > 1) {
      struct timespec ts;
      ts.tv_sec = 0;
      ts.tv_nsec = 200000000L;
      nanosleep(&ts, 0);
      afi_reap(set);
    }
  }
  return 0;
}
static void afi_halt_all(afi_set_t *set) {
  int order[AFI_MAX_SERVICES];
  int n;
  int i;
  char err[256];
  n = afi_dep_order(set, order, AFI_MAX_SERVICES, 0, 0);
  if (n < 0) return;
  for (i = n - 1; i >= 0; i--) {
    afi_stop_one(&set->svcs[order[i]], err, sizeof(err));
  }
}
static void afi_power(int cmd) {
  sync();
  reboot(cmd);
}
int main(int argc, char **argv) {
  afi_set_t set;
  char err[512];
  int dry = 0;
  int i;
  int is_one = (getpid() == 1);
  struct sigaction sa;
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
      strncpy(afi_dir, argv[++i], sizeof(afi_dir) - 1);
    } else if (strcmp(argv[i], "-l") == 0 && i + 1 < argc) {
      afi_level = argv[++i][0];
    } else if (strcmp(argv[i], "-n") == 0) {
      dry = 1;
    }
  }
  if (afi_set_load(afi_dir, &set, err, sizeof(err)) != 0) {
    fprintf(stderr, "afi-init: %s\n", err);
    return 1;
  }
  if (afi_validate(&set, err, sizeof(err)) != 0) {
    fprintf(stderr, "afi-init: %s\n", err);
    return 1;
  }
  if (dry) {
    int order[AFI_MAX_SERVICES];
    int n = afi_dep_order(&set, order, AFI_MAX_SERVICES, err, sizeof(err));
    if (n < 0) {
      fprintf(stderr, "afi-init: %s\n", err);
      return 1;
    }
    for (i = 0; i < n; i++) {
      if (strchr(set.svcs[order[i]].runlevels, afi_level))
        printf("%s\n", set.svcs[order[i]].name);
    }
    return 0;
  }
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = afi_on_chld;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
  sigaction(SIGCHLD, &sa, 0);
  sa.sa_handler = afi_on_term;
  sa.sa_flags = SA_RESTART;
  sigaction(SIGTERM, &sa, 0);
  sigaction(SIGINT, &sa, 0);
  sa.sa_handler = afi_on_hup;
  sigaction(SIGHUP, &sa, 0);
  sa.sa_handler = afi_on_usr1;
  sigaction(SIGUSR1, &sa, 0);
  sa.sa_handler = afi_on_usr2;
  sigaction(SIGUSR2, &sa, 0);
  signal(SIGPIPE, SIG_IGN);
  if (is_one) setsid();
  if (afi_boot(&set) != 0) {
    if (is_one) {
      for (;;) pause();
    }
    return 1;
  }
  for (;;) {
    pause();
    if (afi_got_chld) {
      afi_got_chld = 0;
      afi_reap(&set);
    }
    if (afi_got_hup) {
      afi_got_hup = 0;
      for (i = 0; i < set.n; i++) {
        pid_t p;
        if (strchr(set.svcs[i].runlevels, afi_level)) continue;
        if (afi_read_pid(set.svcs[i].pidfile, &p) == 0 && afi_pid_alive(p))
          afi_stop_one(&set.svcs[i], 0, 0);
      }
      afi_boot(&set);
    }
    if (afi_got_usr1) {
      afi_got_usr1 = 0;
      afi_halt_all(&set);
      if (is_one) afi_power(RB_AUTOBOOT);
      return 0;
    }
    if (afi_got_usr2) {
      afi_got_usr2 = 0;
      afi_halt_all(&set);
      if (is_one) {
        for (;;) pause();
      }
      return 0;
    }
    if (afi_got_term) {
      afi_halt_all(&set);
      if (is_one) afi_power(RB_POWER_OFF);
      return 0;
    }
  }
  return 0;
}
