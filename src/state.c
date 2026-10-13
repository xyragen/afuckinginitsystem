#include "afi.h"
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
afi_state_t afi_state_of(afi_svc_t *s) {
  pid_t pid;
  if (!s) return AFI_UNKNOWN;
  if (!s->pidfile[0]) return AFI_UNKNOWN;
  if (afi_read_pid(s->pidfile, &pid) != 0) return AFI_STOPPED;
  if (afi_pid_alive(pid)) return AFI_RUNNING;
  return AFI_FAILED;
}
int afi_state_name(afi_state_t st, char *out, size_t n) {
  const char *s = "unknown";
  if (st == AFI_STOPPED) s = "stopped";
  else if (st == AFI_RUNNING) s = "running";
  else if (st == AFI_FAILED) s = "failed";
  if (out && n) snprintf(out, n, "%s", s);
  return (int)st;
}
int afi_start_one(afi_svc_t *s, char *err, size_t errn) {
  pid_t old;
  pid_t pid;
  if (!s || !s->exec[0]) {
    if (err) snprintf(err, errn, "bad service");
    return -1;
  }
  if (afi_read_pid(s->pidfile, &old) == 0 && afi_pid_alive(old)) {
    if (err) snprintf(err, errn, "%s already running", s->name);
    return 1;
  }
  if (access(s->exec, X_OK) != 0) {
    if (err) snprintf(err, errn, "%s: %s not executable", s->name, s->exec);
    return -1;
  }
  unlink(s->pidfile);
  pid = afi_spawn_full(s);
  if (pid < 0) {
    if (err) snprintf(err, errn, "%s: fork failed", s->name);
    return -1;
  }
  if (afi_write_pid(s->pidfile, pid) != 0) {
    if (err) snprintf(err, errn, "%s: cannot write pidfile", s->name);
    return -1;
  }
  afi_sup_add(pid, s->name);
  if (s->ready > 0) {
    int waited = 0;
    int limit = s->ready * 10;
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 100000000L;
    while (waited < limit) {
      pid_t w = waitpid(pid, 0, WNOHANG);
      if (w == pid) {
        unlink(s->pidfile);
        afi_sup_del(pid);
        if (err) snprintf(err, errn, "%s: exited during start", s->name);
        return -1;
      }
      if (afi_pid_alive(pid)) {
        if (waited >= 10) break;
      } else {
        unlink(s->pidfile);
        afi_sup_del(pid);
        if (err) snprintf(err, errn, "%s: died during start", s->name);
        return -1;
      }
      nanosleep(&ts, 0);
      waited++;
    }
  }
  return 0;
}
int afi_stop_one(afi_svc_t *s, char *err, size_t errn) {
  pid_t pid;
  if (!s) {
    if (err) snprintf(err, errn, "bad service");
    return -1;
  }
  if (afi_read_pid(s->pidfile, &pid) != 0) return 0;
  if (!afi_pid_alive(pid)) {
    unlink(s->pidfile);
    return 0;
  }
  if (afi_stop_pid(pid, s->timeout > 0 ? s->timeout : 5) != 0) {
    if (err) snprintf(err, errn, "%s: stop timed out", s->name);
    return -1;
  }
  afi_sup_del(pid);
  unlink(s->pidfile);
  return 0;
}
int afi_reload_one(afi_svc_t *s, char *err, size_t errn) {
  pid_t pid;
  int sig;
  if (!s) {
    if (err) snprintf(err, errn, "bad service");
    return -1;
  }
  if (afi_read_pid(s->pidfile, &pid) != 0 || !afi_pid_alive(pid)) {
    if (err) snprintf(err, errn, "%s not running", s->name);
    return -1;
  }
  sig = s->reload_sig > 0 ? s->reload_sig : 1;
  if (kill(pid, sig) != 0) {
    if (err) snprintf(err, errn, "%s: reload signal failed", s->name);
    return -1;
  }
  return 0;
}
