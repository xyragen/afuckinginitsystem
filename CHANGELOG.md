# Changelog

## 0.1 - 2026-10-13

Initial release.

afi-init boots services in dependency order, starts independent services together, reaps children, and restarts supervised services with backoff. afi-ctl handles start, stop, reload, restart, try-restart, status, list, validate, dry-run, rescue, reboot, and shutdown. Service files are plain key=value pairs with pidfile tracking and configurable stop timeouts and reload signals. Runlevels filter which services boot. Config validation catches missing binaries, unknown dependencies, self-deps, and cycles.
