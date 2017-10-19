CC=gcc
CFLAGS=-g -Wall -std=c99
TARGET=parse
OBJS=parse.o objects.o data.o data_parse.o vocab.o

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

clean:
	$(RM) *.o $(TARGET)

.PHONY: clean