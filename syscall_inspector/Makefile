BIN = app_inspector drakvuf_inspector fac_inspector rekall_inspector

CC = g++
CFLAGS = -g -W -Wall -O3 -fomit-frame-pointer -fno-strict-overflow -D_FORTIFY_SOURCE=2 -fstack-protector-all -Wstack-protector --param ssp-buffer-size=1 -DHARDENING
LIBS = -lexplain -lseccomp
FLAGS = $(CFLAGS) $(LIBS)
RM= rm -vf


all: $(BIN)

app_inspector.c:
	python3 policy_writer.py --policy app.policy --master master_inspector.c --output app_inspector.c
fac_inspector.c:
	python3 policy_writer.py --policy fac.policy --master master_inspector.c --output fac_inspector.c
rekall_inspector.c:
	python3 policy_writer.py --policy rekall.policy --master master_inspector.c --output rekall_inspector.c
drakvuf_inspector.c:
	python3 policy_writer.py --policy drakvuf.policy --master master_inspector.c --output drakvuf_inspector.c

app_inspector: app_inspector.c
	$(ECHO_CONF)$(CC) app_inspector.c $(FLAGS) -o $@
fac_inspector: fac_inspector.c
	$(ECHO_CONF)$(CC) fac_inspector.c $(FLAGS) -o $@
rekall_inspector: rekall_inspector.c
	$(ECHO_CONF)$(CC) rekall_inspector.c $(FLAGS) -o $@
drakvuf_inspector: drakvuf_inspector.c
	$(ECHO_CONF)$(CC) drakvuf_inspector.c $(FLAGS) -o $@

clean:
	$(ECHO_CONF)$(RM) $(BIN)
	$(ECHO_CONF)$(RM) app_inspector.c fac_inspector.c rekall_inspector.c drakvuf_inspector.c
