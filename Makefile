CC ?= cc
CXX ?= c++
CFLAGS ?= -O2 -Wall -Wextra -Werror -std=c11 -Iinclude -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
CXXFLAGS ?= -O2 -Wall -Wextra -Werror -std=c++17 -Iinclude -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
DBGFLAGS = -O0 -g -Wall -Wextra -std=c11 -Iinclude -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
PREFIX ?= /usr/local
SBINDIR = $(PREFIX)/sbin
SVCSDIR = /etc/afi/services
OBJDIR = build/obj
LIB_OBJ = $(OBJDIR)/util.o $(OBJDIR)/svc.o $(OBJDIR)/dep.o $(OBJDIR)/state.o $(OBJDIR)/runlevel.o $(OBJDIR)/sup.o
INIT_OBJ = $(OBJDIR)/init.o
CTL_OBJ = $(OBJDIR)/ctl.o
all: build/afi-init build/afi-ctl
build $(OBJDIR):
	mkdir -p build $(OBJDIR)
$(OBJDIR)/%.o: src/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@
$(OBJDIR)/sup.o: src/sup.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
build/afi-init: $(LIB_OBJ) $(INIT_OBJ) | build
	$(CC) $(CFLAGS) $(LIB_OBJ) $(INIT_OBJ) -o $@
build/afi-ctl: $(LIB_OBJ) $(CTL_OBJ) | build
	$(CC) $(CFLAGS) $(LIB_OBJ) $(CTL_OBJ) -o $@
debug: | build $(OBJDIR)
	$(CC) $(DBGFLAGS) -c src/util.c -o $(OBJDIR)/util.o
	$(CC) $(DBGFLAGS) -c src/svc.c -o $(OBJDIR)/svc.o
	$(CC) $(DBGFLAGS) -c src/dep.c -o $(OBJDIR)/dep.o
	$(CC) $(DBGFLAGS) -c src/state.c -o $(OBJDIR)/state.o
	$(CC) $(DBGFLAGS) -c src/runlevel.c -o $(OBJDIR)/runlevel.o
	$(CXX) -O0 -g -Wall -Wextra -std=c++17 -Iinclude -c src/sup.cpp -o $(OBJDIR)/sup.o
	$(CC) $(DBGFLAGS) -c src/init.c -o $(OBJDIR)/init.o
	$(CC) $(DBGFLAGS) -c src/ctl.c -o $(OBJDIR)/ctl.o
	$(CC) $(DBGFLAGS) $(LIB_OBJ) $(INIT_OBJ) -o build/afi-init
	$(CC) $(DBGFLAGS) $(LIB_OBJ) $(CTL_OBJ) -o build/afi-ctl
test: build/afi-init build/afi-ctl build/test_dep build/test_svc build/test_state build/test_runlevel build/test_util
	./build/test_dep
	./build/test_svc
	./build/test_state
	./build/test_runlevel
	./build/test_util
	sh tests/run.sh
build/test_dep: tests/test_dep.c $(LIB_OBJ) | build
	$(CC) $(CFLAGS) tests/test_dep.c $(LIB_OBJ) -o $@
build/test_svc: tests/test_svc.c $(LIB_OBJ) | build
	$(CC) $(CFLAGS) tests/test_svc.c $(LIB_OBJ) -o $@
build/test_state: tests/test_state.c $(LIB_OBJ) | build
	$(CC) $(CFLAGS) tests/test_state.c $(LIB_OBJ) -o $@
build/test_runlevel: tests/test_runlevel.c $(LIB_OBJ) | build
	$(CC) $(CFLAGS) tests/test_runlevel.c $(LIB_OBJ) -o $@
build/test_util: tests/test_util.c $(LIB_OBJ) | build
	$(CC) $(CFLAGS) tests/test_util.c $(LIB_OBJ) -o $@
install: all
	install -d $(DESTDIR)$(SBINDIR) $(DESTDIR)$(SVCSDIR)
	install -m 0755 build/afi-init $(DESTDIR)$(SBINDIR)/afi-init
	install -m 0755 build/afi-ctl $(DESTDIR)$(SBINDIR)/afi-ctl
	install -m 0755 scripts/*.sh $(DESTDIR)$(SBINDIR)/
	install -m 0644 services/*.svc $(DESTDIR)$(SVCSDIR)/
clean:
	rm -rf build
.PHONY: all debug test install clean
