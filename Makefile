CC = gcc
COMPILE = -fPIC -O3 -Wall -Wpedantic -Werror -Wno-unused
LINKER = -fPIC -Wall
TARGET = libbson.so

SOURCES = $(wildcard *.c)
OBJECTS = $(patsubst %.c,%.o,$(SOURCES))

all: $(OBJECTS) $(TARGET) clean

$(OBJECTS): $(SOURCES)
	$(CC) -c $(SOURCES) $(COMPILE)

$(TARGET): $(OBJECTS)
	$(CC) -shared -o $(TARGET) $(OBJECTS) $(LINKER)

clean:
	rm *.o


