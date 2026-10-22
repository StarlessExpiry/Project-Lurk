SRC_DIR := ../../src
include ../test_config.mk

.PHONY: all clean

TARGETS := dir_table_control_test

all:
	@$(MAKE) $(TARGETS)

dir_table_control_test: dir_table_control_test.cpp $(wildcard $(addsuffix /$(OBJ_DIR)/*.o, $(SUB_DIR)))
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -rf $(TARGETS)
