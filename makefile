CC=gcc
CFLAGS=-g -Wall -ansi -pedantic -std=c99
TARGET=parse
OBJS=src/main.o src/objects.o src/data.o src/data_parse.o src/data_lists.o src/verblib.o src/vocab.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

clean:
	$(RM) *.o $(TARGET)

.PHONY: clean