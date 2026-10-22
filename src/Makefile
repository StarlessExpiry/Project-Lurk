SRC_DIR := .
BIN_DIR = ../deploy/bin

include config.mk

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

TARGETS := $(addprefix $(BIN_DIR)/, $(basename $(wildcard *.cpp)))
# TARGETS := $(BIN_DIR)/cli $(BIN_DIR)/serv $(BIN_DIR)/update $(BIN_DIR)/deliver_server $(BIN_DIR)/lurk_show_dirs

# cancel multithreading, make sure the sequence is 1.SUB_DIR, 2.TARGETS
all:
	@$(MAKE) exec_module
	@$(MAKE) $(TARGETS)
	cp $(SRC_DIR)/lurk-client.sh $(BIN_DIR)/lurk-client
	cp $(SRC_DIR)/lurk-server.sh $(BIN_DIR)/lurk-server
	chmod u+x $(BIN_DIR)/lurk-client
	chmod u+x $(BIN_DIR)/lurk-server


exec_module:
	@for dir in $(SUB_DIR); do \
		$(MAKE) -C $$dir all; \
	done

$(TARGETS): $(BIN_DIR)/%: $(SRC_DIR)/%.cpp $(wildcard $(addsuffix /$(OBJ_DIR)/*.o, $(SUB_DIR)))
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

# $(BIN_DIR)/cli: client.cpp $(wildcard $(addsuffix /$(OBJ_DIR)/*.o, $(SUB_DIR)))
# 	@mkdir -p $(dir $@)
# 	$(CXX) $(CXXFLAGS) $^ -o $@
#
# $(BIN_DIR)/serv: server.cpp $(wildcard $(addsuffix /$(OBJ_DIR)/*.o, $(SUB_DIR)))
# 	@mkdir -p $(dir $@)
# 	$(CXX) $(CXXFLAGS) $^ -o $@
#
# $(BIN_DIR)/update: update.cpp $(wildcard $(addsuffix /$(OBJ_DIR)/*.o, $(SUB_DIR)))
# 	@mkdir -p $(dir $@)
# 	$(CXX) $(CXXFLAGS) $^ -o $@
#
# $(BIN_DIR)/deliver_server: deliver_server.cpp $(wildcard $(addsuffix /$(OBJ_DIR)/*.o, $(SUB_DIR)))
# 	@mkdir -p $(dir $@)
# 	$(CXX) $(CXXFLAGS) $^ -o $@
# $(BIN_DIR)/lurk_show_dirs: lurk_show_dirs.cpp $(wildcard $(addsuffix /$(OBJ_DIR)/*.o, $(SUB_DIR)))
# 	@mkdir -p $(dir $@)
# 	$(CXX) $(CXXFLAGS) $^ -o $@


clean:
	rm -rf $(TARGETS) $(BIN_DIR)/lurk-client $(BIN_DIR)/lurk-client
	@for dir in $(SUB_DIR); do \
		$(MAKE) -C $$dir clean; \
	done
