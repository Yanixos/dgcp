SOURCES = header.c neighborsController.c dgcp_handler.c commandline.c main.c
OBJS = $(SOURCES:%.c=%.o)
INCLUDES = -lpthread -lm -lreadline

dgcp : $(OBJS)
	gcc -o $@ $(OBJS) $(INCLUDES) -Wall
		
%.o : %.c
	@echo "building $(@:%.o=%)..."
	gcc -c $< -o $@
	
header.o: header.c header.h
neighborsController.o: neighborsController.c neighborsController.h header.h
dgcp_handler.o: dgcp_handler.c dgcp_handler.h hexdump.h neighborsController.h header.h
commandline.o: commandline.c commandline.h dgcp_handler.h neighborsController.h
main.o: main.c commandline.h dgcp_handler.h

clean:
	rm dgcp $(OBJS)
