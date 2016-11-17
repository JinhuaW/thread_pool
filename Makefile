VERSION  = 1
RELEASE  = 0.0
NAME     = libthreadpool.so
SONAME   = $(NAME).$(VERSION)
LIB      = $(SONAME).$(RELEASE)
LIBOBJS  = thread_pool.o
INCLUDES = thread_pool.h 
EXE      = pool_test

override CFLAGS += -Wall -Werror

.PHONY: all clean

all: $(LIB)

$(LIBOBJS): override CFLAGS += -fPIC -lpthread

$(LIB): $(LIBOBJS)
	$(CC) $(LDFLAGS) -shared -Wl,-soname,$(SONAME) -o $@ $^
	ln -sf $(LIB) $(NAME)

clean:
	rm -f *~ *.o $(LIB) $(EXE)
