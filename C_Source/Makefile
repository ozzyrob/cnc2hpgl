all: cnc2hpgl

cnc2hpgl: cnc2hpgl.c
	gcc -Wall -lm cnc2hpgl.c -o cnc2hpgl

install: cnc2hpgl
	cp cnc2hpgl /usr/local/bin

clean:
	rm cnc2hpgl 
