CC = clang
CXX = clang++
CFLAGS = -g -Wall


####################			ALGO			#####################
ALGO_DIR = ./
ALGO_OUTPUT_DIR = ./output
ALGO_TEST_DIR = ./test


# 定义对象文件，手动指定存放路径（建议与源文件夹结构对应）
ALGO_OBJECTS = 	$(ALGO_OUTPUT_DIR)/segment_tree.o \
								$(ALGO_OUTPUT_DIR)/tools/msg_print_algo.o\
								$(ALGO_OUTPUT_DIR)/flags.o

ALGO_CPP_TO_C_OBJECTS =	$(ALGO_OUTPUT_DIR)/tools/discrete.o

ALGO_TEST_OBJECTS =	$(ALGO_TEST_DIR)/segment_tree.test

lib-all: $(ALGO_OBJECTS) $(ALGO_TEST_OBJECTS) $(ALGO_CPP_TO_C_OBJECTS)

# 通用编译规则：使用 % 匹配并自动创建目标目录
# 例如，makefile将$(LIB_OUTPUT_DIR)/net/daemon.o改写为$(LIB_DIR)/net/daemon.c
$(ALGO_OUTPUT_DIR)/%.o: $(ALGO_DIR)/%.c 
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(ALGO_OUTPUT_DIR)/%.o: $(ALGO_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CFLAGS) -c $< -o $@

$(ALGO_TEST_DIR)/segment_tree.test: $(ALGO_TEST_DIR)/segment_tree_test.c $(ALGO_OBJECTS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -DDEBUG $^ -o $@

clean:
	rm -rf $(ALGO_OUTPUT_DIR) $(ALGO_TEST_DIR)/*.test

###################				ALGO end 			#############################
