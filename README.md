# afuckinginitsystem

afuckinginitsystem is a lightweight, modular Linux init system built for speed and simplicity, with easy service management, service dependencies, custom runlevels, process handling, and fast boot times.

## Build

Run make. Needs a C and C++ compiler, nothing else.

    make

Two binaries land in build. afi-init boots the system and reaps children. afi-ctl starts and stops services by hand. Debug builds use `make debug`.

## Usage

Each service is one file:

    name = net
    desc = loopback networking
    exec = /sbin/net-setup
    pidfile = /run/afi/net.pid
    depends = syslog
    runlevels = 2345
    respawn = yes
    timeout = 5
    reload_sig = HUP

Check the config and see the boot order without starting anything:

    afi-ctl -d services validate
    afi-ctl -d services dry-run -l 3
    afi-init -d services -n -l 3

Manual control:

    afi-ctl -d services start net
    afi-ctl -d services status
    afi-ctl -d services reload cron
    afi-ctl -d services restart app
    afi-ctl -d services try-restart app
    afi-ctl -d services stop app

Power commands only work through explicit calls:

    afi-ctl -d services reboot
    afi-ctl -d services shutdown
    afi-ctl -d services rescue

A restart never reboots the box. It stops the service and starts it again, unless the service file sets reload handling and the caller asked for a reload.

## File layout

include holds the shared header. src holds only source files: the init, the control tool, service parsing, dependency sort, runlevel filtering, process handling, and a small C++ supervision table. All objects go to build/obj and all binaries go to build. scripts holds boot and halt helpers. services holds example service files. tests holds unit and shell tests.

## Status

Working: service start, stop, reload, restart, try-restart, status, runlevel filtering, dependency sort with cycle detection, parallel boot within a dependency level, pidfile tracking, supervision with restart backoff, child reaping, config validation, and dry-run.

Experimental: rescue handling and the halt helpers are thin and untested on real hardware. Boot testing belongs in a VM. Do not replace the host init with this.

## Test

    make test
