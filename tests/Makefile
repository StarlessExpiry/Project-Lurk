SRC_DIR := ../src
include test_config.mk

DEBUG := 1
export DEBUG
export CC CFLAGS CXX CXXFLAGS OBJ_DIR

.PHONY: all clean exec_module

SUB_DIR := 	$(SRC_DIR)/core \
			$(SRC_DIR)/deliver \
			$(SRC_DIR)/deps/sqlite \
			$(SRC_DIR)/deps/yaml-cpp \
			$(SRC_DIR)/net \
			$(SRC_DIR)/servs \
			$(SRC_DIR)/sys \
			$(SRC_DIR)/tools

# TARGETS_DIR := ./execute
# TARGETS := client server update deliver_server lurk_show_dirs lurk_set_dirs lurk_init
# TARGETS := $(addprefix $(TARGETS_DIR)/, $(TARGETS))

# cancel multithreading, make sure the sequence is 1.SUB_DIR, 2.TARGETS
all:
	@$(MAKE) exec_module
	# @$(MAKE) $(TARGETS)

exec_module:
	@for dir in $(SUB_DIR); do \
    	echo "== make dir '$$dir' =="; \
		$(MAKE) -C $$dir all; \
	done



# $(TARGETS): $(TARGETS_DIR)/%: $(SRC_DIR)/%.cpp $(wildcard $(addsuffix /$(OBJ_DIR)/*.o, $(SUB_DIR)))
# 	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	# rm -rf $(TARGETS_DIR)
	# rm -rf $(TARGETS)
	@for dir in $(SUB_DIR); do \
    	echo "== clean dir '$$dir' =="; \
		$(MAKE) -C $$dir clean; \
	done
