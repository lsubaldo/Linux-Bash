CC = clang
CFLAGS = -g -Wall 
# main executable file first
TARGET = proj02
# object files next
OBJS = main.o 
# header files next
DEPS = 
.PHONY : clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

.c.o: $(DEPS)
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) $(TARGET) *~

