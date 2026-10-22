# CC = clang
# CFLAGS = -g
# CXX = clang++
# CXXFLAGS = -g -std=c++11
#
#
# OBJ_DIR = obj
#

HEADERS := 	-I. \
			-I.. \
			-Ideps \
			-I../deps \
      		-Ideps/yaml-cpp/include \
        	-I../deps/yaml-cpp/include

CC := clang
CFLAGS := -g -Wall $(HEADERS)
CXX := clang++
CXXFLAGS := -g -Wall -std=c++11 $(HEADERS)

OBJ_DIR = obj
