sync.out: sync.o sem.o fifo.o tas64.o
	gcc sync.o sem.o fifo.o tas64.o -o sync.out

sync.o: sync.c sem.h fifo.h
	gcc -c sync.c
	
sem.o: sem.c sem.h
	gcc -c sem.c

fifo.o: fifo.c fifo.h
	gcc -c fifo.c
	
tas64.o: tas64.S
	gcc -c tas64.S

clean:
	rm *.exe *.out *.o *.stackdump *~
