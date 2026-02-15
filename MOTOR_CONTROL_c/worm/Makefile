TARGET := worm
BUILD_DIR := build

SRCS := \
    main.c \
    neuron.c \
    neural_init.c \
    libsoul/mem/arena.c \
    libsoul/sched.c

OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

CC ?= gcc
CFLAGS ?= -O2 -g -Wall -Wextra -Wpedantic
CPPFLAGS += -Iinclude
LDLIBS += -lm

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

-include $(DEPS)
