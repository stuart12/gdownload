CC=${CXX}
objs = main.o context.o config.o
executable = a.out
${executable} : ${objs}
	g++ -o $@ ${objs} -lgphoto2

context.o : CFLAGS += -fpermissive

clean:
	rm -f ${objs} ${excutable}
