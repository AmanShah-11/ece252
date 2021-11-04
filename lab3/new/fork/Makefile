
# Makefile, ECE252  
# Yiqing Huang, 2019/06/12

CC = gcc       # compiler
CFLAGS = -Wall -g  -std=gnu99# compilation flg 
LD = gcc       # linker
LDFLAGS = -g   # debugging symbols in build
LDLIBS = -lcurl -pthread -lz      # link with libz

# For students 
LIB_UTIL = 
SRCS   = forkN.c zombie.c shm_stack.c shm_simple.c shm_struct.c
OBJS1   = forkN.o $(LIB_UTIL)
OBJS2   = zombie.o $(LIB_UTIL)
OBJS3 =  shm_simple.o $(LIB_UTIL)
OBJS4 = shm_struct.c shm_stack.o $(LIB_UTIL)

TARGETS= forkN.out zombie.out shm_simple.out shm_struct.out

all: ${TARGETS}

forkN.out: $(OBJS1) 
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS) 

zombie.out: $(OBJS2) 
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS)

shm_simple.out: $(OBJS3)
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS)

shm_struct.out: $(OBJS4)
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS)

%.o: %.c 
	$(CC) $(CFLAGS) -c $< 

%.d: %.c
	gcc -MM -MF $@ $<

-include $(SRCS:.c=.d)

.PHONY: clean
clean:
	rm -f *.d *.o *~ $(TARGETS) 
