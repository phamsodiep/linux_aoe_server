EXE_FNAME = aoeserver
OBJS = $(EXE_FNAME).o rawsock.o aoeprotocol.o ataio.o
CC = gcc
CCFLAG = -c -o
LFLAG = -o


.PHONY: all clean

all: $(EXE_FNAME)

clean:
	rm -f $(EXE_FNAME) $(OBJS)

$(EXE_FNAME): $(OBJS)
	$(CC) $(LFLAG) $@ $^

%.o: %.c %.h
	$(CC) $(CCFLAG) $@ $<
