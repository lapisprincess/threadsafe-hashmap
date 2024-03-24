all: main.c ts_hashmap.o rtclock.o
	gcc -Wall -g -o ts_hashmap main.c ts_hashmap.o rtclock.o -lpthread -lm

ts_hashmap.o: ts_hashmap.h ts_hashmap.c
	gcc -Wall -g -c ts_hashmap.c

rtclock.o: rtclock.h rtclock.c
	gcc -Wall -g -c rtclock.c

clean:
	rm -f *.o