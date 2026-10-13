#include "afi.h"
#include <cstring>
#include <sys/types.h>
#include <unistd.h>
#define AFI_SUP_MAX 256
static pid_t afi_sup_pids[AFI_SUP_MAX];
static char afi_sup_names[AFI_SUP_MAX][AFI_NAME_MAX];
static int afi_sup_n = 0;
int afi_sup_add(pid_t pid, const char *name) {
  int i;
  for (i = 0; i < afi_sup_n; i++) {
    if (afi_sup_pids[i] == pid) {
      if (name) {
        strncpy(afi_sup_names[i], name, AFI_NAME_MAX - 1);
        afi_sup_names[i][AFI_NAME_MAX - 1] = 0;
      }
      return 0;
    }
  }
  if (afi_sup_n >= AFI_SUP_MAX) return -1;
  afi_sup_pids[afi_sup_n] = pid;
  if (name) {
    strncpy(afi_sup_names[afi_sup_n], name, AFI_NAME_MAX - 1);
    afi_sup_names[afi_sup_n][AFI_NAME_MAX - 1] = 0;
  } else {
    afi_sup_names[afi_sup_n][0] = 0;
  }
  afi_sup_n++;
  return 0;
}
int afi_sup_del(pid_t pid) {
  int i;
  for (i = 0; i < afi_sup_n; i++) {
    if (afi_sup_pids[i] == pid) {
      int last = afi_sup_n - 1;
      afi_sup_pids[i] = afi_sup_pids[last];
      if (i != last) memmove(afi_sup_names[i], afi_sup_names[last], AFI_NAME_MAX);
      afi_sup_n--;
      return 0;
    }
  }
  return -1;
}
const char *afi_sup_get(pid_t pid) {
  int i;
  for (i = 0; i < afi_sup_n; i++) {
    if (afi_sup_pids[i] == pid) return afi_sup_names[i];
  }
  return 0;
}
int afi_sup_count(void) {
  return afi_sup_n;
}
void afi_sup_each(void (*fn)(pid_t pid, const char *name)) {
  int i;
  if (!fn) return;
  for (i = 0; i < afi_sup_n; i++) fn(afi_sup_pids[i], afi_sup_names[i]);
}
