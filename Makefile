CC = gcc
CFLAGS = -Wall -Wextra

all: build

build:
	$(CC) $(CFLAGS) backend/main.c -o openconta_engine

run: build
	./openconta_engine

clean:
	rm -f openconta_engine
