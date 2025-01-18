TARGET = clox
CC = clang
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

default: $(TARGET)
	./$(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: default clean