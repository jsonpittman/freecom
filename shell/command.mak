.AUTODEPEND

CFG = TCCDOS.CFG
CFG_DEPENDENCIES = COMMAND.MAK ..\strings.h

all: $(CFG) command.exe

##>> Modify this file with your local settings
!include "..\config.mak"

command.exe : $(CFG) batch.obj \
	cb_catch.obj \
	cmdtable.obj \
	command.obj \
	do_batch.obj \
	do_cmd.obj \
	do_exit.obj \
	do_for.obj \
	do_inter.obj \
	do_keep.obj \
	dummies.obj \
	ec_fcts.obj \
	expalias.obj \
	fcttable.obj \
	init.obj \
	kswap.obj \
	loadhigh.obj \
	module.obj \
	redir.obj \
	ver.obj 
	$(LD) /m/s/l /c /d @&&|
$(LIBPATH)\c0s.obj+
batch.obj+
cb_catch.obj+
cmdtable.obj+
command.obj+
do_batch.obj+
do_cmd.obj+
do_exit.obj+
do_for.obj+
do_inter.obj+
do_keep.obj+
dummies.obj+
ec_fcts.obj+
expalias.obj+
fcttable.obj+
init.obj+
kswap.obj+
loadhigh.obj+
module.obj+
redir.obj+
ver.obj
command
		# no map file
$(SUPPL_LIB_PATH)\suppl_s.lib +
..\cmd\cmds.lib +
..\lib\freecom.lib +
..\strings\strings.lib +
$(LIBPATH)\cs.lib
|
