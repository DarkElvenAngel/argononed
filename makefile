# ********************************************************************
# Argonone Daemon Makefile
# ********************************************************************
CC      	 = gcc
RM      	 = rm -v
DTC     	 = dtc -@ -I dts -O dtb -o
BASH		 = bash
INSTALL 	 = install
CFLAGS  	 = -Wall -s -O3
LFLAGS  	 = -lpthread -lrt
LFLAGS3 	 = -lrt
OBJ_DAEMON   = build/argononed.o build/event_timer.o
OBJ_CLI      = src/argonone-cli.c
BIN_DAEMON   = argononed
BIN_SHUTDOWN = argonone-shutdown
BIN_CLI 	 = argonone-cli
OVERLAY 	 = argonone.dtbo
GCCVER  	 = $(shell expr `gcc -dumpversion | cut -f1 -d.` \>= 10)
USERID		 = $(shell id -u)
LOGLEVEL 	 = 5

-include makefile.conf
ifndef BOOTLOC
BOOTLOC = /boot
endif
ifndef INITSYS
INITSYS = SYSTEMD
endif
ifndef I2CHELPER
I2CHELPER = 0
endif
ifndef AUTOCOMP
AUTOCOMP = 0
endif
ifndef LOGROTATE
LOGROTATE = 0
endif
ifdef DISABLE_POWER_BUTTON_SUPPORT
CFLAGS += -DDISABLE_POWER_BUTTON_SUPPORT
endif
ifdef USE_SYSFS_TEMP
CFLAGS += -DUSE_SYSFS_TEMP=$(USE_SYSFS_TEMP)
endif
ifdef RUN_IN_FOREGROUND
CFLAGS += -DRUN_IN_FOREGROUND
endif
ifdef ENABLE_COMPILE_WARNINGS
CFLAGS += -Wextra -Wconversion -Wunused -Wuninitialized
ifeq ($(GCCVER), 1)
	CFLAGS  += -fanalyzer
endif
endif

-include OS/_common/$(INITSYS).in
-include OS/$(DISTRO)/makefile.in

ifndef CONFIGURED
ifeq (,$(wildcard makefile.conf))
$(warning Configuration missing or not correct)
endif
endif

ifeq (install,$(findstring install, $(MAKECMDGOALS)))
ifneq ($(USERID), 0)
$(error "(Un)Installing requires elevated privileges")
endif
ifeq ($(PACKAGESYS),ENABLED)
$(error "(Un)Installing Not supported with Package System")
endif
endif
ifeq (update,$(findstring update, $(MAKECMDGOALS)))
ifneq ($(USERID), 0)
$(error "Updating requires elevated privileges")
endif
ifeq ($(PACKAGESYS),ENABLED)
$(error "Updating Not supported with Package System")
endif
endif

.DEFAULT_GOAL := all



build/%.o: src/%.c
	@echo "Compile $<"
	$(CC) -c -o $@ $< $(CFLAGS) -DLOG_LEVEL=$(LOGLEVEL) 

$(BIN_DAEMON): $(OBJ_DAEMON)@echo "Build $(BIN_DAEM O N)"
	$(CC) -o build/$(BIN_DAE  MON) $^ $(CFLAGS) $(LFLAGS)

$(BIN_SHUTDOWN): src/argonone-shutdown.c
	@echo "Build $(BIN_SHUTDOWN)"
	$(CC) -o build/$(BIN_SHUTDOWN) $^ $(CFLAGS)

$(BIN_CLI): $(OBJ_CLI) 
	@echo "Build $(BIN_CLI)"
	$(CC) -o build/$(BIN_CLI) $^ $(CFLAGS) -DLOG_LEVEL=$(LOGLEVEL) $(LFLAGS3)

$(OVERLAY): src/argonone.dts
	@echo "Build $@"
	$(DTC) build/$@ $<

.PHONY: overlay
overlay: $(OVERLAY)
	@echo "MAKE: Overlay"

.PHONY: daemon
daemon: $(BIN_DAEMON) $(BIN_SHUTDOWN)
	@echo "MAKE: Daemon"

.PHONY: cli
cli: $(BIN_CLI)
	@echo "MAKE: CLI"

.PHONY: all
all:: daemon cli overlay
	@echo "MAKE: Complete"

.PHONY: install-daemon
install-daemon:
	@echo -n "Installing daemon "
	@$(INSTALL) build/$(BIN_DAEMON) /usr/sbin/$(BIN_DAEMON) 2>/dev/null && echo "Successful" || { echo "Failed"; true; }
ifeq ($(LOGROTATE),1)
	@$(INSTALL) -m 600 OS/_common/argononed.logrotate /etc/logrotate.d/argononed
endif

.PHONY: install-cli
install-cli:
	@echo -n "Installing CLI "
	@$(INSTALL) -m 0755 build/$(BIN_CLI) /usr/bin/$(BIN_CLI) 2>/dev/null && echo "Successful" || { echo "Failed"; true; }
ifeq ($(AUTOCOMP), 1)
	@echo -n "Installing CLI autocomplete for bash "
	@$(INSTALL) -m 755 OS/_common/argonone-cli-complete.bash /etc/bash_completion.d/argonone-cli 2>/dev/null && echo "Successful" || { echo "Failed"; true; }
endif

.PHONY: install-overlay
install-overlay:
	@echo -n "Installing overlay "
	@$(INSTALL) build/argonone.dtbo $(BOOTLOC)/overlays/argonone.dtbo 2>/dev/null && echo "Successful" || { echo "Failed"; }
	@$(BASH) OS/_common/setup-overlay.sh $(BOOTLOC)/config.txt


.PHONY: install-service
install-service:
	@echo "Installing services "
	@echo -n "argononed.service ... "
	@$(INSTALL) -m $(SERVICE_FILE_PERMISSIONS) $(SERVICE_FILE) $(SERVICE_PATH) 2>/dev/null && echo "Successful" || { echo "Failed"; true; } 
	@echo -n "argonone-shutdown ... "
	@$(INSTALL) $(SHUTDOWN_FILE) $(SHUTDOWN_PATH) 2>/dev/null && echo "Successful" || { echo "Failed"; true; }
ifeq ($(INITSYS), SYSTEMD)
	@echo "Refresh services list"
	@systemctl daemon-reload
endif
	@echo -n "Enable Service "
	@$(SERVICE_ENABLE) argononed &>/dev/null && echo "Successful" || { echo "Failed"; }
	@echo -n "Starting Service "
	@timeout 5s $(SERVICE_START) &>/dev/null && echo "Successful" || { ( [ $$? -eq 124 ] && echo "Timeout" || echo "Failed" ) }


.PHONY: install
install:: install-daemon install-cli install-service install-overlay
ifeq ($(shell if [ -f /usr/bin/argononed ]; then echo 1; fi), 1)
	@echo -n "Removing old daemon ... "
	@$(RM) /usr/bin/argononed 2>/dev/null&& echo "Successful" || { echo "Failed"; true; }
endif
	@echo "Install Complete"

.PHONY: update
update:: install-daemon install-cli install-service
ifeq ($(shell if [ -f /usr/bin/argononed ]; then echo 1; fi), 1)
	@echo -n "Removing old daemon ... "
	@$(RM) /usr/bin/argononed 2>/dev/null&& echo "Successful" || { echo "Failed"; true; }
endif
	@echo "Update Complete"

.PHONY: uninstall
uninstall::
	@echo -n "Stop Service ... "
	@$(SERVICE_STOP) &>/dev/null && echo "Successful" || { echo "Failed"; }
	@echo -n "Disable Service ... "
	@$(SERVICE_DISABLE) &>/dev/null && echo "Successful" || { echo "Failed"; }
	@echo -n "Erase Service ... "
	@$(RM) $(SERVICE_PATH) 2>/dev/null && echo "Successful" || { echo "Failed"; true; }
ifeq ($(INITSYS), OPENRC)
	@echo -n "Erase Shutdown Service ... "
	@$(RM) $(SHUTDOWN_PATH) 2>/dev/null && echo "Successful" || { echo "Failed"; true; }
	@echo -n "Erase argonone-shutdown ... "
	@$(RM) /usr/*bin/shutdown_argonone 2>/dev/null && echo "Successful" || { echo "Failed"; true; }
else
	@echo -n "Erase argonone-shutdown ... "
	@$(RM) $(SHUTDOWN_PATH) 2>/dev/null && echo "Successful" || { echo "Failed"; true; }
endif
	@echo -n "Remove overlay ... "
	@$(RM) $(BOOTLOC)/overlays/argonone.dtbo 2>/dev/null && echo "Successful" || { echo "Failed"; }
	@echo -n "Remove daemon ... "
	@$(RM) /usr/*bin/argononed 2>/dev/null&& echo "Successful" || { echo "Failed"; true; }
	@echo -n "Remove cli-tool ... "
	@$(RM) /usr/bin/argonone-cli 2>/dev/null&& echo "Successful" || { echo "Failed"; true; }
	@echo -n "Remove autocomplete for cli ... "
	$(RM) /etc/bash_completion.d/argonone-cli 2>/dev/null && echo "Successful" || { echo "Failed"; true; }
	@echo -n "Remove logrotate config ... "
	$(RM) /etc/logrotate.d/argononed 2>/dev/null && echo "Successful" || { echo "Failed"; true; }
	@echo "Remove dtoverlay=argonone from $(BOOTLOC)/config.txt"
	@cp $(BOOTLOC)/config.txt $(BOOTLOC)/config.argonone.backup
	@sed -i '/dtoverlay=argonone/d' $(BOOTLOC)/config.txt
	@echo "Uninstall Complete"

.PHONY: clean
clean::
	-@$(RM) *.o 2>/dev/null || true
	-@$(RM) argonone.dtbo 2>/dev/null || true
	-@$(RM) $(BIN_DAEMON) 2>/dev/null || true
	-@$(RM) $(BIN_SHUTDOWN) 2>/dev/null || true
	-@$(RM) $(BIN_CLI) 2>/dev/null || true
	-@$(RM) build/* 2>/dev/null || true

.PHONY: mrproper
mrproper: clean
	-@$(RM) makefile.conf 2>/dev/null || true

.PHONY: dumpvars
dumpvars:
	@$(foreach V,$(sort $(.VARIABLES)), $(if $(filter-out environment% default automatic,$(origin $V)),$(warning $V=$($V) ($(value $V)))))

ifdef MAKE_OVERRIDES
-include OS/$(DISTRO)/override.mk
endif