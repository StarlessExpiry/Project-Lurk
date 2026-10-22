SRC_DIR := ../../src
include ../test_config.mk

.PHONY: all clean

TARGETS := $(notdir $(basename $(wildcard $(SRC_DIR)/*.cpp)))

all:
	$(MAKE) -C $(SRC_DIR)
	@$(MAKE) $(TARGETS)
	# cp $(SRC_DIR)/lurk.sh ./lurk
	# chmod u+x ./lurk

$(TARGETS): %: $(SRC_DIR)/%.cpp $(wildcard $(addsuffix /$(OBJ_DIR)/*.o, $(SUB_DIR)))
	$(CXX) $(CXXFLAGS) $^ -o $@


clean:
	rm -rf $(TARGETS)
