SRC_DIR := ../../src
include ../test_config.mk

.PHONY: all clean

TARGETS := dir_sync_recv_test dir_sync_send_test
TARGETS_SRC := lurk_client_single_file_deliver lurk_server_single_file_deliver lurk_server_init lurk_client_init
all:
	$(MAKE) -C ..
	@$(MAKE) $(OBJ_DIR)/fake_file_deliver.o
	@$(MAKE) $(TARGETS)
	@$(MAKE) $(TARGETS_SRC)

$(OBJ_DIR)/fake_file_deliver.o : fake_file_deliver.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $^ -o $@

OBJ := $(wildcard $(addsuffix /$(OBJ_DIR)/*.o, $(SUB_DIR)))
OBJ := $(filter-out %/file_deliver_recv.o %/file_deliver_send.o, $(OBJ))
OBJ += $(OBJ_DIR)/fake_file_deliver.o

$(TARGETS): %: %.cpp $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(TARGETS_SRC): %: $(SRC_DIR)/%.cpp $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -rf $(TARGETS)
	rm -rf $(TARGETS_SRC)
	rm -rf $(OBJ_DIR)
