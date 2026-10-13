#include "afi.h"
#include <ctype.h>
#include <dirent.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
static int afi_split_deps(char *v, afi_svc_t *s) {
  char *tok;
  s->ndep = 0;
  tok = strtok(v, ", \t");
  while (tok && s->ndep < AFI_MAX_DEPS) {
    afi_trim(tok);
    if (*tok) {
      strncpy(s->depends[s->ndep], tok, AFI_NAME_MAX - 1);
      s->depends[s->ndep][AFI_NAME_MAX - 1] = 0;
      s->ndep++;
    }
    tok = strtok(0, ", \t");
  }
  return 0;
}
int afi_svc_parse(const char *path, afi_svc_t *out, char *err, size_t errn) {
  FILE *f;
  char line[1024];
  memset(out, 0, sizeof(*out));
  out->timeout = 5;
  out->reload_sig = 1;
  f = fopen(path, "r");
  if (!f) {
    if (err) snprintf(err, errn, "cannot open %s", path);
    return -1;
  }
  while (fgets(line, sizeof(line), f)) {
    char *eq;
    char *k;
    char *v;
    afi_trim(line);
    if (!*line || *line == '#') continue;
    eq = strchr(line, '=');
    if (!eq) {
      if (err) snprintf(err, errn, "bad line: %s", line);
      fclose(f);
      return -1;
    }
    *eq = 0;
    k = line;
    v = eq + 1;
    afi_trim(k);
    afi_trim(v);
    if (strcmp(k, "name") == 0) {
      strncpy(out->name, v, AFI_NAME_MAX - 1);
    } else if (strcmp(k, "desc") == 0) {
      strncpy(out->desc, v, AFI_DESC_MAX - 1);
    } else if (strcmp(k, "exec") == 0) {
      strncpy(out->exec, v, AFI_PATH_MAX - 1);
    } else if (strcmp(k, "args") == 0) {
      strncpy(out->args, v, AFI_DEPS_MAX - 1);
    } else if (strcmp(k, "pidfile") == 0) {
      strncpy(out->pidfile, v, AFI_PATH_MAX - 1);
    } else if (strcmp(k, "depends") == 0) {
      afi_split_deps(v, out);
    } else if (strcmp(k, "runlevels") == 0) {
      strncpy(out->runlevels, v, AFI_RL_MAX - 1);
    } else if (strcmp(k, "respawn") == 0) {
      out->respawn = (strcasecmp(v, "yes") == 0 || strcmp(v, "1") == 0 || strcasecmp(v, "true") == 0);
    } else if (strcmp(k, "timeout") == 0) {
      out->timeout = atoi(v);
      if (out->timeout < 1) out->timeout = 1;
      if (out->timeout > 120) out->timeout = 120;
    } else if (strcmp(k, "reload_sig") == 0) {
      int s = afi_sig_from_name(v);
      if (s < 0) s = atoi(v);
      if (s > 0) out->reload_sig = s;
    } else if (strcmp(k, "user") == 0) {
      strncpy(out->user, v, AFI_NAME_MAX - 1);
    } else if (strcmp(k, "group") == 0) {
      strncpy(out->group, v, AFI_NAME_MAX - 1);
    } else if (strcmp(k, "dir") == 0) {
      strncpy(out->dir, v, AFI_PATH_MAX - 1);
    } else if (strcmp(k, "env") == 0) {
      if (out->nenv < AFI_MAX_ENV && strchr(v, '=')) {
        strncpy(out->env[out->nenv], v, AFI_ENV_MAX - 1);
        out->nenv++;
      } else {
        if (err) snprintf(err, errn, "bad env: %s", v);
        fclose(f);
        return -1;
      }
    } else if (strcmp(k, "ready") == 0) {
      out->ready = atoi(v);
      if (out->ready < 0) out->ready = 0;
    } else {
      if (err) snprintf(err, errn, "unknown key: %s", k);
      fclose(f);
      return -1;
    }
  }
  fclose(f);
  if (!out->name[0]) {
    if (err) snprintf(err, errn, "%s: missing name", path);
    return -1;
  }
  if (!out->exec[0]) {
    if (err) snprintf(err, errn, "%s: missing exec", path);
    return -1;
  }
  if (!out->runlevels[0]) strncpy(out->runlevels, "2345", AFI_RL_MAX - 1);
  return 0;
}
int afi_set_find(afi_set_t *set, const char *name) {
  int i;
  if (!set || !name) return -1;
  for (i = 0; i < set->n; i++) {
    if (strcmp(set->svcs[i].name, name) == 0) return i;
  }
  return -1;
}
int afi_set_load(const char *dir, afi_set_t *set, char *err, size_t errn) {
  DIR *d;
  struct dirent *e;
  memset(set, 0, sizeof(*set));
  d = opendir(dir);
  if (!d) {
    if (err) snprintf(err, errn, "cannot open dir %s", dir);
    return -1;
  }
  while ((e = readdir(d))) {
    size_t n;
    char full[1024];
    if (e->d_name[0] == '.') continue;
    n = strlen(e->d_name);
    if (n < 5 || strcmp(e->d_name + n - 4, ".svc") != 0) continue;
    if (set->n >= AFI_MAX_SERVICES) {
      if (err) snprintf(err, errn, "too many services");
      closedir(d);
      return -1;
    }
    snprintf(full, sizeof(full), "%s/%s", dir, e->d_name);
    if (afi_svc_parse(full, &set->svcs[set->n], err, errn) != 0) {
      closedir(d);
      return -1;
    }
    if (afi_set_find(set, set->svcs[set->n].name) >= 0) {
      if (err) snprintf(err, errn, "duplicate service %s", set->svcs[set->n].name);
      closedir(d);
      return -1;
    }
    set->n++;
  }
  closedir(d);
  return 0;
}
int afi_validate(afi_set_t *set, char *err, size_t errn) {
  int i;
  int j;
  if (!set) return -1;
  for (i = 0; i < set->n; i++) {
    afi_svc_t *s = &set->svcs[i];
    if (!s->name[0] || !s->exec[0]) {
      if (err) snprintf(err, errn, "service %d missing name or exec", i);
      return -1;
    }
    if (access(s->exec, X_OK) != 0) {
      if (err) snprintf(err, errn, "service %s: exec not executable: %s", s->name, s->exec);
      return -1;
    }
    if (s->pidfile[0] && s->pidfile[0] != '/') {
      if (err) snprintf(err, errn, "service %s: pidfile must be absolute", s->name);
      return -1;
    }
    if (s->user[0] && !getpwnam(s->user)) {
      if (err) snprintf(err, errn, "service %s: unknown user %s", s->name, s->user);
      return -1;
    }
    if (s->group[0] && !getgrnam(s->group)) {
      if (err) snprintf(err, errn, "service %s: unknown group %s", s->name, s->group);
      return -1;
    }
    if (s->dir[0] && access(s->dir, X_OK) != 0) {
      if (err) snprintf(err, errn, "service %s: bad dir %s", s->name, s->dir);
      return -1;
    }
    for (j = 0; j < s->ndep; j++) {
      if (strcmp(s->depends[j], s->name) == 0) {
        if (err) snprintf(err, errn, "service %s depends on itself", s->name);
        return -1;
      }
      if (afi_set_find(set, s->depends[j]) < 0) {
        if (err) snprintf(err, errn, "service %s: unknown dependency %s", s->name, s->depends[j]);
        return -1;
      }
    }
  }
  return afi_dep_order(set, 0, 0, err, errn);
}
