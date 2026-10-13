#ifndef AFI_H
#define AFI_H
#include <stddef.h>
#include <sys/types.h>
#ifdef __cplusplus
extern "C" {
#endif
#define AFI_MAX_SERVICES 128
#define AFI_MAX_DEPS 16
#define AFI_NAME_MAX 64
#define AFI_DESC_MAX 128
#define AFI_PATH_MAX 256
#define AFI_DEPS_MAX 512
#define AFI_RL_MAX 16
#define AFI_MAX_ARGS 32
#define AFI_MAX_ENV 32
#define AFI_ENV_MAX 256
typedef enum {
  AFI_STOPPED = 0,
  AFI_RUNNING = 1,
  AFI_FAILED = 2,
  AFI_UNKNOWN = 3
} afi_state_t;
typedef struct {
  char name[AFI_NAME_MAX];
  char desc[AFI_DESC_MAX];
  char exec[AFI_PATH_MAX];
  char args[AFI_DEPS_MAX];
  char pidfile[AFI_PATH_MAX];
  char depends[AFI_MAX_DEPS][AFI_NAME_MAX];
  int ndep;
  char runlevels[AFI_RL_MAX];
  int respawn;
  int timeout;
  int reload_sig;
  char user[AFI_NAME_MAX];
  char group[AFI_NAME_MAX];
  char dir[AFI_PATH_MAX];
  char env[AFI_MAX_ENV][AFI_ENV_MAX];
  int nenv;
  int ready;
} afi_svc_t;
typedef struct {
  afi_svc_t svcs[AFI_MAX_SERVICES];
  int n;
} afi_set_t;
int afi_trim(char *s);
int afi_sig_from_name(const char *s);
const char *afi_sig_to_name(int sig);
int afi_pid_alive(pid_t pid);
int afi_read_pid(const char *path, pid_t *out);
int afi_write_pid(const char *path, pid_t pid);
pid_t afi_spawn_full(afi_svc_t *s);
pid_t afi_spawn(const char *exec, const char *args);
int afi_stop_pid(pid_t pid, int timeout);
int afi_svc_parse(const char *path, afi_svc_t *out, char *err, size_t errn);
int afi_set_load(const char *dir, afi_set_t *set, char *err, size_t errn);
int afi_set_find(afi_set_t *set, const char *name);
int afi_validate(afi_set_t *set, char *err, size_t errn);
int afi_dep_order(afi_set_t *set, int *order, int n, char *err, size_t errn);
int afi_runlevel_filter(afi_set_t *set, const char *level, int *idx, int max);
afi_state_t afi_state_of(afi_svc_t *s);
int afi_state_name(afi_state_t st, char *out, size_t n);
int afi_level_ok(afi_svc_t *s, char level);
int afi_level_list(afi_set_t *set, char level, int *idx, int max);
int afi_level_depth(afi_set_t *set, int *depth);
int afi_start_one(afi_svc_t *s, char *err, size_t errn);
int afi_stop_one(afi_svc_t *s, char *err, size_t errn);
int afi_reload_one(afi_svc_t *s, char *err, size_t errn);
int afi_sup_add(pid_t pid, const char *name);
int afi_sup_del(pid_t pid);
const char *afi_sup_get(pid_t pid);
int afi_sup_count(void);
void afi_sup_each(void (*fn)(pid_t pid, const char *name));
#ifdef __cplusplus
}
#endif
#endif

