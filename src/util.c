#include "afi.h"
#include <ctype.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
int afi_trim(char *s) {
  char *p;
  size_t n;
  size_t i;
  if (!s) return -1;
  p = s;
  while (*p && isspace((unsigned char)*p)) p++;
  if (p != s) memmove(s, p, strlen(p) + 1);
  n = strlen(s);
  while (n > 0 && isspace((unsigned char)s[n - 1])) s[--n] = 0;
  for (i = 0; i < n; i++) {
    if (!isprint((unsigned char)s[i]) && s[i] != '\t') {
      memmove(s + i, s + i + 1, n - i);
      n--;
      i--;
    }
  }
  return 0;
}
struct afi_sigmap {
  const char *name;
  int sig;
};
static struct afi_sigmap afi_sigs[] = {
  {"HUP", SIGHUP},
  {"INT", SIGINT},
  {"QUIT", SIGQUIT},
  {"TERM", SIGTERM},
  {"USR1", SIGUSR1},
  {"USR2", SIGUSR2},
  {0, 0}
};
int afi_sig_from_name(const char *s) {
  int i;
  if (!s) return -1;
  for (i = 0; afi_sigs[i].name; i++) {
    if (strcasecmp(s, afi_sigs[i].name) == 0) return afi_sigs[i].sig;
  }
  return -1;
}
const char *afi_sig_to_name(int sig) {
  int i;
  for (i = 0; afi_sigs[i].name; i++) {
    if (afi_sigs[i].sig == sig) return afi_sigs[i].name;
  }
  return "?";
}
int afi_pid_alive(pid_t pid) {
  if (pid <= 0) return 0;
  if (kill(pid, 0) == 0) return 1;
  return 0;
}
int afi_read_pid(const char *path, pid_t *out) {
  FILE *f;
  long v;
  if (!path || !out) return -1;
  f = fopen(path, "r");
  if (!f) return -1;
  if (fscanf(f, "%ld", &v) != 1) {
    fclose(f);
    return -1;
  }
  fclose(f);
  if (v <= 0) return -1;
  *out = (pid_t)v;
  return 0;
}
int afi_write_pid(const char *path, pid_t pid) {
  FILE *f;
  if (!path) return -1;
  f = fopen(path, "w");
  if (!f) return -1;
  fprintf(f, "%d\n", (int)pid);
  fclose(f);
  return 0;
}
pid_t afi_spawn_full(afi_svc_t *s) {
  pid_t pid;
  if (!s || !s->exec[0]) return -1;
  pid = fork();
  if (pid < 0) return -1;
  if (pid > 0) return pid;
  setsid();
  {
    int fd = open("/dev/null", O_RDWR);
    if (fd >= 0) {
      dup2(fd, 0);
      dup2(fd, 1);
      dup2(fd, 2);
      if (fd > 2) close(fd);
    }
  }
  if (s->dir[0]) chdir(s->dir);
  {
    int i;
    for (i = 0; i < s->nenv; i++) putenv(s->env[i]);
  }
  if (s->group[0]) {
    struct group *g = getgrnam(s->group);
    if (g) setgid(g->gr_gid);
  }
  if (s->user[0]) {
    struct passwd *p = getpwnam(s->user);
    if (p) {
      if (!s->group[0]) setgid(p->pw_gid);
      setuid(p->pw_uid);
    }
  }
  {
    char *argv[AFI_MAX_ARGS + 2];
    char buf[AFI_DEPS_MAX];
    char *tok;
    int n = 0;
    argv[n++] = s->exec;
    if (s->args[0]) {
      strncpy(buf, s->args, sizeof(buf) - 1);
      buf[sizeof(buf) - 1] = 0;
      tok = strtok(buf, " \t");
      while (tok && n < AFI_MAX_ARGS + 1) {
        argv[n++] = tok;
        tok = strtok(0, " \t");
      }
    }
    argv[n] = 0;
    execv(s->exec, argv);
  }
  _exit(127);
}
pid_t afi_spawn(const char *exec, const char *args) {
  pid_t pid;
  char *argv[AFI_MAX_ARGS + 2];
  char buf[AFI_DEPS_MAX];
  char *tok;
  int n = 0;
  if (!exec || !*exec) return -1;
  pid = fork();
  if (pid < 0) return -1;
  if (pid > 0) return pid;
  setsid();
  argv[n++] = (char *)exec;
  if (args && *args) {
    strncpy(buf, args, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = 0;
    tok = strtok(buf, " \t");
    while (tok && n < AFI_MAX_ARGS + 1) {
      argv[n++] = tok;
      tok = strtok(0, " \t");
    }
  }
  argv[n] = 0;
  execv(exec, argv);
  _exit(127);
}
int afi_stop_pid(pid_t pid, int timeout) {
  int i;
  int ticks;
  if (pid <= 0) return -1;
  if (!afi_pid_alive(pid)) return 0;
  if (kill(pid, SIGTERM) != 0) {
    if (!afi_pid_alive(pid)) return 0;
    return -1;
  }
  if (timeout < 1) timeout = 5;
  ticks = timeout * 10;
  for (i = 0; i < ticks; i++) {
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 100000000L;
    if (!afi_pid_alive(pid)) return 0;
    if (waitpid(pid, 0, WNOHANG) > 0) return 0;
    nanosleep(&ts, 0);
  }
  if (!afi_pid_alive(pid)) return 0;
  kill(pid, SIGKILL);
  for (i = 0; i < 20; i++) {
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 100000000L;
    if (waitpid(pid, 0, WNOHANG) > 0) return 0;
    if (!afi_pid_alive(pid)) return 0;
    nanosleep(&ts, 0);
  }
  if (afi_pid_alive(pid)) return -1;
  return 0;
}
