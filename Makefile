default:
	gcc -o krusej.buildrooms krusej.buildrooms.c
	gcc -o krusej.adventure krusej.adventure.c -lpthread
clean:
	rm -f krusej.buildrooms *.o *~
	rm -f krusej.adventure *.o *~
