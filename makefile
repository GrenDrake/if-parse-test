CC=gcc
CFLAGS=-g -Wall -ansi -pedantic -std=c99
TARGET=parse
OBJS=src/main.o src/io.o src/objects.o src/data.o src/data_parse.o src/data_tokenize.o src/data_lists.o src/verblib.o src/vocab.o src/function.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

clean:
	$(RM) src/*.o $(TARGET)

.PHONY: clean
