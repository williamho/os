a: bin/schedtest.o bin/sched.o bin/savectx.o bin/adjstack.o
	gcc -g -o a bin/schedtest.o bin/sched.o bin/savectx.o bin/adjstack.o
	
bin/schedtest.o: schedtest.c
	gcc -c schedtest.c -o bin/schedtest.o -O0
	
bin/sched.o: sched.c sched.h
	gcc -c sched.c -o bin/sched.o
	
bin/savectx.o: savectx64.S
	gcc -c savectx64.S -o bin/savectx.o
	
bin/adjstack.o: adjstack.c
	gcc -c adjstack.c -o bin/adjstack.o
	
clean:
	rm -f *.o