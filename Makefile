CC=${CXX}
objs = main.o context.o config.o
a.out: ${objs}
	g++ -o $@ ${objs} -lgphoto2

context.o : CFLAGS += -fpermissive
