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
		-DCOREY_ENABLE_COVERAGE=$(ENABLE_COVERAGE)
	ln -sf $(BUILD_DIR)/compile_commands.json ./

clean:
	rm -rf $(BUILD_DIR)

test: build
	cd $(BUILD_DIR) && ctest

coverage: test
	cd $(BUILD_DIR) && lcov --gcov-tool $(GCOV_TOOL) --capture --directory . --output-file coverage.info
	cd $(BUILD_DIR) && lcov --remove coverage.info '/usr/*' --output-file coverage.info
	cd $(BUILD_DIR) && lcov --remove coverage.info '$(ABS_BUILD_DIR)/_deps/*' --output-file coverage.info
	cd $(BUILD_DIR) && lcov --remove coverage.info '$(ABS_BUILD_DIR)/lib/reactor/io/liburing-install/*' --output-file coverage.info
	cd $(BUILD_DIR) && genhtml coverage.info --output-directory coverage
