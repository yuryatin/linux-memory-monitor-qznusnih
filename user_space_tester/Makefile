CC ?= $(CROSS_COMPILE)gcc
CFLAGS += $(TARGET_CFLAGS)
LDFLAGS += $(TARGET_LDFLAGS)

SRC = src/main.c src/sysfs_writer.c
OBJ = $(SRC:.c=.o)
TARGET = qznusnih_user_space_tester

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) -Iinclude $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJ)
