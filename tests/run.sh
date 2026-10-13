#!/bin/sh
set -e
cd "$(dirname "$0")/.."
rm -rf /tmp/afi_run
mkdir -p /tmp/afi_run/svcs
cat > /tmp/afi_run/svcs/net.svc <<EOF
name = net
desc = loopback net
exec = /bin/sleep
args = 30
pidfile = /tmp/afi_run/net.pid
runlevels = 2345
timeout = 2
EOF
cat > /tmp/afi_run/svcs/app.svc <<EOF
name = app
desc = app server
exec = /bin/sleep
args = 30
pidfile = /tmp/afi_run/app.pid
depends = net
runlevels = 35
timeout = 2
EOF
cat > /tmp/afi_run/svcs/worker.svc <<EOF
name = worker
desc = background worker
exec = /bin/sleep
args = 30
pidfile = /tmp/afi_run/worker.pid
runlevels = 12345
timeout = 2
env = QUEUE=default
EOF
./build/afi-ctl -d /tmp/afi_run/svcs validate
./build/afi-ctl -d /tmp/afi_run/svcs dry-run -l 3 | grep -q net
./build/afi-ctl -d /tmp/afi_run/svcs dry-run -l 3 | grep -q worker
./build/afi-ctl -d /tmp/afi_run/svcs dry-run -l 3 | grep -q app
test "$(./build/afi-ctl -d /tmp/afi_run/svcs dry-run -l 3 | tail -n 1)" = "app"
./build/afi-ctl -d /tmp/afi_run/svcs list | grep -q net
./build/afi-ctl -d /tmp/afi_run/svcs status | grep -q net
./build/afi-ctl -d /tmp/afi_run/svcs start app
./build/afi-ctl -d /tmp/afi_run/svcs status app | grep -q running
./build/afi-ctl -d /tmp/afi_run/svcs status net | grep -q running
./build/afi-ctl -d /tmp/afi_run/svcs start worker
./build/afi-ctl -d /tmp/afi_run/svcs log worker | grep -q "no log"
./build/afi-ctl -d /tmp/afi_run/svcs try-restart app
./build/afi-ctl -d /tmp/afi_run/svcs reload app
./build/afi-ctl -d /tmp/afi_run/svcs restart worker
./build/afi-ctl -d /tmp/afi_run/svcs status worker | grep -q running
./build/afi-ctl -d /tmp/afi_run/svcs stop worker
./build/afi-ctl -d /tmp/afi_run/svcs try-restart worker
./build/afi-ctl -d /tmp/afi_run/svcs status worker | grep -q stopped
./build/afi-ctl -d /tmp/afi_run/svcs stop app
./build/afi-ctl -d /tmp/afi_run/svcs stop net
./build/afi-ctl -d /tmp/afi_run/svcs status app | grep -q stopped
./build/afi-init -d /tmp/afi_run/svcs -n -l 3 | grep -q net
./build/afi-init -d /tmp/afi_run/svcs -n -l 1 | grep -q worker
if ./build/afi-init -d /tmp/afi_run/svcs -n -l 1 | grep -q app; then
  echo "app should not be in runlevel 1"
  exit 1
fi
cat > /tmp/afi_run/svcs/bad.svc <<EOF
name = bad
exec = /bin/sleep
pidfile = /tmp/afi_run/bad.pid
depends = missing
EOF
if ./build/afi-ctl -d /tmp/afi_run/svcs validate 2>/dev/null; then
  echo "expected validate to fail"
  exit 1
fi
rm -f /tmp/afi_run/svcs/bad.svc
cat > /tmp/afi_run/svcs/cyc_a.svc <<EOF
name = cyc_a
exec = /bin/sleep
pidfile = /tmp/afi_run/cyc_a.pid
depends = cyc_b
EOF
cat > /tmp/afi_run/svcs/cyc_b.svc <<EOF
name = cyc_b
exec = /bin/sleep
pidfile = /tmp/afi_run/cyc_b.pid
depends = cyc_a
EOF
if ./build/afi-ctl -d /tmp/afi_run/svcs validate 2>/dev/null; then
  echo "expected cycle to fail"
  exit 1
fi
rm -f /tmp/afi_run/svcs/cyc_a.svc /tmp/afi_run/svcs/cyc_b.svc
./build/afi-ctl -d /tmp/afi_run/svcs validate
rm -rf /tmp/afi_run /tmp/afi_state_test.pid /tmp/afi_state_dead.pid
echo "run.sh ok"
