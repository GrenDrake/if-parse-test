CC=gcc
CFLAGS=-g -Wall -ansi -pedantic -std=c99
TARGET=parse
OBJS=main.o objects.o data.o data_parse.o vocab.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

clean:
	$(RM) *.o $(TARGET)

.PHONY: clean