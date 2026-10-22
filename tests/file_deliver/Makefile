SRC_DIR := ../../src
include ../test_config.mk

.PHONY: all clean server client

TARGETS := lurk_client_single_file_deliver lurk_server_single_file_deliver

all:
	$(MAKE) -C ..
	@$(MAKE) $(TARGETS)

$(TARGETS): %: $(SRC_DIR)/%.cpp $(wildcard $(addsuffix /$(OBJ_DIR)/*.o, $(SUB_DIR)))
	$(CXX) $(CXXFLAGS) $^ -o $@

server:
	./lurk_server_single_file_deliver ST_COMMON 20201 /Users/starless/Projects/Sync-Files-System/tests/file_deliver/recv /Users/starless/Projects/Sync-Files-System/tests/file_deliver/recv/verify

client:
	./lurk_client_single_file_deliver ST_COMMON 20201 ./send/hello.txt 6 4bd1f03845e2d0f387930042ac7928b517d2db37048f4f681885053bf0fd1c7b ./send/verify
clean:
	rm -rf $(TARGETS)
