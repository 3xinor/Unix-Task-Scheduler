myapp: CFS

CFS: CFS.o
	gcc -o CFS scheduler.o
	
CFS.o: scheduler.c scheduler.h
	gcc -c scheduler.c
