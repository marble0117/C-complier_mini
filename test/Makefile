XCC = ../xcc
# OPTION = -m32 -Wl,-no_pie
OPTION =  -m32 -Wl,-read_only_relocs,suppress

all: kadai1 kadai2 kadai3 

kadai0: kadai0.c
	${XCC} kadai0.c  > kadai0.s
	gcc ${OPTION} -o kadai0 kadai0.s
	-./kadai0

kadai1: kadai1.c
	${XCC} kadai1.c  > kadai1.s
	gcc ${OPTION} -o kadai1 kadai1.s
	-./kadai1

kadai2: kadai2.c
	${XCC} kadai2.c  > kadai2.s
	gcc ${OPTION} -o kadai2 kadai2.s
	-./kadai2

kadai3: kadai3.c
	${XCC} kadai3.c  > kadai3.s
	gcc ${OPTION} -o kadai3 kadai3.s
	-./kadai3 10

clean:
	-rm -f *.s a.out kadai0 kadai1 kadai2 kadai3
