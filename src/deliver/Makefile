# DEBUG_FLAG ?= 0

# ifdef DEBUG
# 	include ../../tests/test_config.mk
# else
# 	include ../../config.mk
# endif

SRCS := $(wildcard *.c)
OBJS := $(patsubst %.c, $(OBJ_DIR)/%.o, $(SRCS))

.PHONY: all clean
all: $(OBJS)

$(OBJS): $(OBJ_DIR)/%.o: %.c file_deliver.h deliver.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -rf $(OBJ_DIR)
