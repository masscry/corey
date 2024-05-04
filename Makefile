.PHONY: configure build clean

BUILD_TYPE ?= Debug
CXX_COMPILER ?= clang++-15
C_COMPILER ?= clang-15
GCOV_TOOL ?= gcov-12

BUILD_DIR ?= ./out/$(CXX_COMPILER)-$(BUILD_TYPE)

GENERATOR ?= Ninja

MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
MKFILE_DIR := $(patsubst %/,%,$(dir $(MKFILE_PATH)))
ABS_BUILD_DIR := $(abspath $(BUILD_DIR))

ENABLE_COVERAGE ?= FALSE

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
		-DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE \
		-DCOREY_ENABLE_COVERAGE=$(ENABLE_COVERAGE) \
		-DCOREY_GCOV_TOOL=$(GCOV_TOOL)
	ln -sf $(BUILD_DIR)/compile_commands.json ./

clean:
	rm -rf $(BUILD_DIR)

test: build
	cd $(BUILD_DIR) && ctest

coverage: build
	cd $(BUILD_DIR) && cmake --build . --target coverage
