# DEBUG_FLAG ?= 0

# ifdef DEBUG
# 	include ../../tests/test_config.mk
# else
# 	include ../../config.mk
# endif


SRCS_CXX := $(wildcard *.cpp)
OBJS_CXX := $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(SRCS_CXX))
SRCS := $(wildcard *.c)
OBJS := $(patsubst %.c, $(OBJ_DIR)/%.o, $(SRCS))


# CXXFLAGS += -I$(SRC_DIR)
# CFLAGS += -I$(SRC_DIR)

.PHONY: all clean
all: $(OBJS_CXX) $(OBJS)

$(OBJS_CXX): $(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJS): $(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

# clean:
# 	rm -rf $(OBJ_DIR)
