SRC_DIR := ./src
include config.mk

.PHONY: all clean exec_module

SUB_DIR := 	$(SRC_DIR)/core \
			$(SRC_DIR)/deliver \
			$(SRC_DIR)/deps/sqlite \
			$(SRC_DIR)/deps/yaml-cpp \
			$(SRC_DIR)/net \
			$(SRC_DIR)/servs \
			$(SRC_DIR)/sys \
			$(SRC_DIR)/tools


# all: $(SUB_DIR) send/cli recv/serv

# TARGETS := cli serv update deliver_server
TARGETS :=
# cancel multithreading, make sure the sequence is 1.SUB_DIR, 2.TARGETS
all:
	@$(MAKE) exec_module

exec_module:
	@for dir in $(SUB_DIR); do \
		echo "== travese $$dir =="; \
		$(MAKE) -C $$dir all; \
	done

# cli: src/client.cpp $(wildcard $(addsuffix /obj/*.o, $(SUB_DIR)))
# 	@mkdir -p $(dir $@)
# 	$(CXX) $(CXXFLAGS) $^ -o $@
# 
# serv: src/server.cpp $(wildcard $(addsuffix /obj/*.o, $(SUB_DIR)))
# 	@mkdir -p $(dir $@)
# 	$(CXX) $(CXXFLAGS) $^ -o $@
# 
# update: src/update.cpp $(wildcard $(addsuffix /obj/*.o, $(SUB_DIR)))
# 	@mkdir -p $(dir $@)
# 	$(CXX) $(CXXFLAGS) $^ -o $@
# 
# deliver_server: src/deliver_server.cpp $(wildcard $(addsuffix /obj/*.o, $(SUB_DIR)))
# 	@mkdir -p $(dir $@)
# 	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	# rm -rf cli serv update deliver_server
	@for dir in $(SUB_DIR); do \
		$(MAKE) -C $$dir clean; \
	done
