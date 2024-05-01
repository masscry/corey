.PHONY: configure build clean

BUILD_TYPE ?= Debug
BUILD_DIR ?= ./out/$(BUILD_TYPE)

GENERATOR ?= Ninja
CXX_COMPILER ?= clang++-15
C_COMPILER ?= clang-15

MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
MKFILE_DIR := $(patsubst %/,%,$(dir $(MKFILE_PATH)))


build: configure
	cmake --build $(BUILD_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

configure: $(BUILD_DIR)
	cmake \
		-S $(MKFILE_DIR) \
		-B $(BUILD_DIR) \
		-G $(GENERATOR) \
		-DCMAKE_C_COMPILER=$(C_COMPILER) \
		-DCMAKE_CXX_COMPILER=$(CXX_COMPILER) \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE
	ln -sf $(BUILD_DIR)/compile_commands.json ./

clean:
	rm -rf $(BUILD_DIR)

test: build
	cd $(BUILD_DIR) && ctest