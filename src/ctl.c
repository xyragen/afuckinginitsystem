#include "afi.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static void usage(void) {
  printf("usage: afi-ctl [-d dir] <cmd> [name]\n");
  printf("  start <name>        start service and deps\n");
  printf("  stop <name>         stop service\n");
  printf("  restart <name>      stop then start\n");
  printf("  try-restart <name>  restart only if running\n");
  printf("  reload <name>       send reload signal\n");
  printf("  status [name]       show state\n");
  printf("  list                list services\n");
  printf("  runlevel <level>    switch runlevel\n");
  printf("  validate            check config\n");
  printf("  dry-run [-l level]  print boot order\n");
  printf("  log <name>          show recent log lines\n");
  printf("  reboot              stop all and reboot\n");
  printf("  shutdown            stop all and power off\n");
  printf("  rescue              stop all non-rescue services\n");
}
static void afi_mark_need(afi_set_t *set, int idx, int *mark) {
  int stack[AFI_MAX_SERVICES];
  int top = 0;
  int j;
  mark[idx] = 1;
  for (j = 0; j < set->svcs[idx].ndep; j++) {
    int d = afi_set_find(set, set->svcs[idx].depends[j]);
    if (d >= 0 && !mark[d]) {
      mark[d] = 1;
      stack[top++] = d;
    }
  }
  while (top > 0) {
    int cur = stack[--top];
    for (j = 0; j < set->svcs[cur].ndep; j++) {
      int d = afi_set_find(set, set->svcs[cur].depends[j]);
      if (d >= 0 && !mark[d]) {
        mark[d] = 1;
        if (top < AFI_MAX_SERVICES) stack[top++] = d;
      }
    }
  }
}
static void afi_mark_hit(afi_set_t *set, int idx, int *mark) {
  int i;
  int j;
  mark[idx] = 1;
  for (i = 0; i < set->n; i++) {
    for (j = 0; j < set->svcs[i].ndep; j++) {
      int d = afi_set_find(set, set->svcs[i].depends[j]);
      if (d == idx) mark[i] = 1;
    }
  }
}
static int start_with_deps(afi_set_t *set, int idx) {
  int order[AFI_MAX_SERVICES];
  int mark[AFI_MAX_SERVICES];
  int n;
  int i;
  char err[512];
  memset(mark, 0, sizeof(mark));
  afi_mark_need(set, idx, mark);
  n = afi_dep_order(set, order, AFI_MAX_SERVICES, err, sizeof(err));
  if (n < 0) {
    fprintf(stderr, "afi-ctl: %s\n", err);
    return 1;
  }
  for (i = 0; i < n; i++) {
    int k = order[i];
    if (!mark[k]) continue;
    if (afi_state_of(&set->svcs[k]) == AFI_RUNNING) continue;
    if (afi_start_one(&set->svcs[k], err, sizeof(err)) != 0) {
      fprintf(stderr, "afi-ctl: %s\n", err[0] ? err : "start failed");
      return 1;
    }
    printf("started %s\n", set->svcs[k].name);
  }
  return 0;
}
static int stop_dependents(afi_set_t *set, int idx) {
  int order[AFI_MAX_SERVICES];
  int mark[AFI_MAX_SERVICES];
  int n;
  int i;
  char err[256];
  memset(mark, 0, sizeof(mark));
  afi_mark_hit(set, idx, mark);
  n = afi_dep_order(set, order, AFI_MAX_SERVICES, err, sizeof(err));
  if (n < 0) {
    fprintf(stderr, "afi-ctl: %s\n", err);
    return 1;
  }
  for (i = n - 1; i >= 0; i--) {
    int k = order[i];
    afi_state_t st;
    if (!mark[k]) continue;
    st = afi_state_of(&set->svcs[k]);
    if (st != AFI_RUNNING && st != AFI_FAILED) continue;
    if (afi_stop_one(&set->svcs[k], err, sizeof(err)) != 0) {
      fprintf(stderr, "afi-ctl: %s\n", err);
      return 1;
    }
    printf("stopped %s\n", set->svcs[k].name);
  }
  return 0;
}
int main(int argc, char **argv) {
  char dir[1024] = "services";
  char level = '3';
  char err[512];
  afi_set_t set;
  char *cmd;
  char *name = 0;
  int ai = 1;
  int i;
  while (ai < argc && argv[ai][0] == '-') {
    if (strcmp(argv[ai], "-d") == 0 && ai + 1 < argc) {
      strncpy(dir, argv[++ai], sizeof(dir) - 1);
      ai++;
    } else if (strcmp(argv[ai], "-l") == 0 && ai + 1 < argc) {
      level = argv[++ai][0];
      ai++;
    } else {
      break;
    }
  }
  if (ai >= argc) {
    usage();
    return 1;
  }
  cmd = argv[ai++];
  if (ai < argc) name = argv[ai];
  if (afi_set_load(dir, &set, err, sizeof(err)) != 0) {
    fprintf(stderr, "afi-ctl: %s\n", err);
    return 1;
  }
  if (strcmp(cmd, "validate") == 0) {
    if (afi_validate(&set, err, sizeof(err)) != 0) {
      fprintf(stderr, "afi-ctl: %s\n", err);
      return 1;
    }
    printf("ok: %d services\n", set.n);
    return 0;
  }
  if (strcmp(cmd, "dry-run") == 0) {
    int order[AFI_MAX_SERVICES];
    int n = afi_dep_order(&set, order, AFI_MAX_SERVICES, err, sizeof(err));
    if (n < 0) {
      fprintf(stderr, "afi-ctl: %s\n", err);
      return 1;
    }
    for (i = 0; i < n; i++) {
      if (strchr(set.svcs[order[i]].runlevels, level))
        printf("%s\n", set.svcs[order[i]].name);
    }
    return 0;
  }
  if (strcmp(cmd, "list") == 0) {
    for (i = 0; i < set.n; i++) {
      char st[16];
      afi_state_name(afi_state_of(&set.svcs[i]), st, sizeof(st));
      printf("%-16s %-8s %s\n", set.svcs[i].name, st, set.svcs[i].desc);
    }
    return 0;
  }
  if (strcmp(cmd, "runlevel") == 0) {
    FILE *f;
    if (!name || !name[0]) {
      fprintf(stderr, "afi-ctl: runlevel needs a level\n");
      return 1;
    }
    f = fopen("/run/afi/level", "w");
    if (!f) {
      fprintf(stderr, "afi-ctl: cannot set runlevel\n");
      return 1;
    }
    fprintf(f, "%c\n", name[0]);
    fclose(f);
    kill(1, SIGHUP);
    printf("runlevel %c\n", name[0]);
    return 0;
  }
  if (strcmp(cmd, "log") == 0) {
    char path[1024];
    FILE *f;
    char line[1024];
    char *lines[20];
    int nl = 0;
    if (!name) {
      fprintf(stderr, "afi-ctl: log needs a name\n");
      return 1;
    }
    i = afi_set_find(&set, name);
    if (i < 0) {
      fprintf(stderr, "afi-ctl: unknown service %s\n", name);
      return 1;
    }
    snprintf(path, sizeof(path), "/var/log/afi/%s.log", name);
    f = fopen(path, "r");
    if (!f) {
      printf("no log for %s\n", name);
      return 0;
    }
    while (fgets(line, sizeof(line), f)) {
      if (nl < 20) {
        lines[nl] = malloc(strlen(line) + 1);
        if (lines[nl]) strcpy(lines[nl++], line);
      } else {
        int k;
        free(lines[0]);
        for (k = 1; k < 20; k++) lines[k - 1] = lines[k];
        lines[19] = malloc(strlen(line) + 1);
        if (lines[19]) strcpy(lines[19], line);
        else nl = 19;
      }
    }
    fclose(f);
    for (i = 0; i < nl; i++) {
      printf("%s", lines[i]);
      free(lines[i]);
    }
    return 0;
  }
  if (strcmp(cmd, "reboot") == 0) {
    if (getpid() == 1) {
      kill(1, SIGUSR1);
      return 0;
    }
    fprintf(stderr, "afi-ctl: reboot only works from init\n");
    return 1;
  }
  if (strcmp(cmd, "shutdown") == 0) {
    if (getpid() == 1) {
      kill(1, SIGTERM);
      return 0;
    }
    fprintf(stderr, "afi-ctl: shutdown only works from init\n");
    return 1;
  }
  if (strcmp(cmd, "rescue") == 0) {
    int order[AFI_MAX_SERVICES];
    int n = afi_dep_order(&set, order, AFI_MAX_SERVICES, err, sizeof(err));
    if (n < 0) {
      fprintf(stderr, "afi-ctl: %s\n", err);
      return 1;
    }
    for (i = n - 1; i >= 0; i--) {
      int k = order[i];
      if (strchr(set.svcs[k].runlevels, '1')) continue;
      if (afi_state_of(&set.svcs[k]) != AFI_RUNNING) continue;
      if (afi_stop_one(&set.svcs[k], err, sizeof(err)) != 0) {
        fprintf(stderr, "afi-ctl: %s\n", err);
        return 1;
      }
      printf("stopped %s\n", set.svcs[k].name);
    }
    return 0;
  }
  if (strcmp(cmd, "status") == 0) {
    if (!name) {
      for (i = 0; i < set.n; i++) {
        char st[16];
        afi_state_name(afi_state_of(&set.svcs[i]), st, sizeof(st));
        printf("%-16s %s\n", set.svcs[i].name, st);
      }
      return 0;
    }
    i = afi_set_find(&set, name);
    if (i < 0) {
      fprintf(stderr, "afi-ctl: unknown service %s\n", name);
      return 1;
    }
    {
      char st[16];
      afi_state_name(afi_state_of(&set.svcs[i]), st, sizeof(st));
      printf("%s: %s\n", name, st);
    }
    return 0;
  }
  if (!name) {
    usage();
    return 1;
  }
  i = afi_set_find(&set, name);
  if (i < 0) {
    fprintf(stderr, "afi-ctl: unknown service %s\n", name);
    return 1;
  }
  if (strcmp(cmd, "start") == 0) {
    return start_with_deps(&set, i);
  }
  if (strcmp(cmd, "stop") == 0) {
    return stop_dependents(&set, i);
  }
  if (strcmp(cmd, "restart") == 0) {
    if (afi_state_of(&set.svcs[i]) == AFI_RUNNING) {
      if (stop_dependents(&set, i) != 0) return 1;
      sleep(1);
    }
    return start_with_deps(&set, i);
  }
  if (strcmp(cmd, "try-restart") == 0) {
    if (afi_state_of(&set.svcs[i]) != AFI_RUNNING) return 0;
    if (stop_dependents(&set, i) != 0) return 1;
    sleep(1);
    return start_with_deps(&set, i);
  }
  if (strcmp(cmd, "reload") == 0) {
    if (afi_reload_one(&set.svcs[i], err, sizeof(err)) != 0) {
      fprintf(stderr, "afi-ctl: %s\n", err);
      return 1;
    }
    printf("reloaded %s\n", name);
    return 0;
  }
  usage();
  return 1;
}