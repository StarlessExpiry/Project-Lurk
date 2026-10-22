SRC_DIR := ../../src
include ../test_config.mk

.PHONY: all clean

TARGETS := lurk_update_dir lurk_set_dir

all:
	$(MAKE) -C ..
	@$(MAKE) $(TARGETS)

$(TARGETS): %: $(SRC_DIR)/%.cpp $(wildcard $(addsuffix /$(OBJ_DIR)/*.o, $(SUB_DIR)))
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -rf $(TARGETS)
