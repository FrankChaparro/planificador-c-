hostd: planificador.c 
	gcc planificador.c -o planificador


run: planificador 
	./planificador lista_procesos.txt

clean: 
	rm -f hostd *.o *~


