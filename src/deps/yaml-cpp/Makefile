# DEBUG_FLAG ?= 0

# ifdef DEBUG
# 	include ../../../tests/test_config.mk
# else
# 	include ../../../config.mk
# endif

# SRCS := $(wildcard *.c)
# OBJS := $(patsubst %.c, $(OBJ_DIR)/%.o, $(SRCS))

.PHONY: all clean exec_module
all: exec_module

exec_module:
	mkdir -p ./build/ && \
	cd ./build && \
	cmake .. 1> /dev/null && \
	make 1> /dev/null 2>&1 && \
	mkdir -p ../$(OBJ_DIR) && \
	ar -x libyaml-cpp.a && \
	mv *.o ../$(OBJ_DIR)/;
# $(OBJS): $(OBJ_DIR)/%.o: %.c
# 	@mkdir -p $(dir $@)
# 	$(CC) $(CFLAGS) -c $< -o $@



clean:
	rm -rf ./build
	rm -rf $(OBJ_DIR)
