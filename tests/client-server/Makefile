SRC_DIR := ../../src
include ../test_config.mk

.PHONY: all clean

TARGETS := client server deliver_server

all:
	$(MAKE) -C ..
	@$(MAKE) $(OBJ_DIR)/fake_dir_sync.o
	@$(MAKE) $(TARGETS)


$(OBJ_DIR)/fake_dir_sync.o : fake_dir_sync.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $^ -o $@

OBJ := $(wildcard $(addsuffix /$(OBJ_DIR)/*.o, $(SUB_DIR)))
OBJ := $(filter-out %/dir_sync.o, $(OBJ))
OBJ += $(OBJ_DIR)/fake_dir_sync.o

# $(TARGETS): %: %.cpp $(OBJ)
# 	$(CXX) $(CXXFLAGS) $^ -o $@

client: $(SRC_DIR)/lurk_client.cpp $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

server: $(SRC_DIR)/lurk_server.cpp $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

lurk_: $(SRC_DIR)/deliver_server.cpp $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@


clean:
	rm -rf $(TARGETS)
	rm -rf $(OBJ_DIR)
