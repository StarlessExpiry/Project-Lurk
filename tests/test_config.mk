HEADERS := 	-I. \
          	-I.. \
           	-I../src \
            -I../src/deps \
            -I../src/deps/yaml-cpp/include \
            -I../../src \
            -I../../src/deps \
            -I../../src/deps/yaml-cpp/include


CC = clang
CFLAGS = -g -Wall -DDEBUG -fsanitize=address -fno-omit-frame-pointer $(HEADERS)
CXX = clang++
CXXFLAGS = -g -Wall -DDEBUG -std=c++11 -Wno-deprecated -fsanitize=address -fno-omit-frame-pointer $(HEADERS)

OBJ_DIR = obj-test

SUB_DIR := 	$(SRC_DIR)/core \
			$(SRC_DIR)/deliver \
			$(SRC_DIR)/deps/sqlite \
			$(SRC_DIR)/deps/yaml-cpp \
			$(SRC_DIR)/net \
			$(SRC_DIR)/servs \
			$(SRC_DIR)/sys \
			$(SRC_DIR)/tools
