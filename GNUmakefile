# Build proxy: every build goes through CMake into $(BUILD_DIR).
# Run `make help` for the list of targets.

BUILD_DIR  ?= build
BUILD_TYPE ?= Release
PREFIX     ?= /usr/local
CMAKE      ?= cmake
CTEST      ?= ctest
CPACK      ?= cpack
GENERATOR  ?= $(if $(shell command -v ninja 2>/dev/null),Ninja,Unix Makefiles)
CMAKE_ARGS ?=

SOURCES := $(shell find sources -name '*.cpp' -o -name '*.h')
CPP_FILES := $(filter %.cpp,$(SOURCES))

.DEFAULT_GOAL := all
.PHONY: all configure build rebuild run test docs install package \
        format format-check tidy clean help

all: build

$(BUILD_DIR)/CMakeCache.txt:
	$(CMAKE) -S . -B $(BUILD_DIR) -G "$(GENERATOR)" \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DCMAKE_INSTALL_PREFIX=$(PREFIX) $(CMAKE_ARGS)

configure: ## (Re)configure the build directory
	$(CMAKE) -S . -B $(BUILD_DIR) -G "$(GENERATOR)" \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DCMAKE_INSTALL_PREFIX=$(PREFIX) $(CMAKE_ARGS)

build: $(BUILD_DIR)/CMakeCache.txt ## Build everything (default)
	$(CMAKE) --build $(BUILD_DIR) --parallel

rebuild: clean build ## Clean, then build from scratch

run: build ## Build and play the sample world
	cd $(BUILD_DIR)/bin && ./tinyadv

test: build ## Build and run the tests
	cd $(BUILD_DIR) && $(CTEST) --output-on-failure

docs: $(BUILD_DIR)/CMakeCache.txt ## Generate API docs with Doxygen (build/docs)
	$(CMAKE) --build $(BUILD_DIR) --target docs

install: build ## Install to PREFIX (default /usr/local; use DESTDIR for staging)
	DESTDIR=$(DESTDIR) $(CMAKE) --install $(BUILD_DIR) --prefix $(PREFIX)

package: build ## Build installer packages (tar.gz, and .deb if dpkg exists)
	cd $(BUILD_DIR) && $(CPACK)

format: ## Reformat all sources with clang-format
	clang-format -i $(SOURCES)

format-check: ## Fail if any source is not clang-format clean
	clang-format --dry-run --Werror $(SOURCES)

tidy: $(BUILD_DIR)/CMakeCache.txt ## Run clang-tidy on all C++ sources
	clang-tidy -p $(BUILD_DIR) --quiet $(CPP_FILES)

clean: ## Remove the build directory
	rm -rf $(BUILD_DIR)

help: ## Show this help
	@echo "Targets:"
	@grep -E '^[a-z-]+:.*## ' $(MAKEFILE_LIST) | \
		awk -F':.*## ' '{ printf "  %-13s %s\n", $$1, $$2 }'
	@echo
	@echo "Variables: BUILD_DIR=$(BUILD_DIR) BUILD_TYPE=$(BUILD_TYPE) PREFIX=$(PREFIX)"
	@echo "           GENERATOR=\"$(GENERATOR)\" CMAKE_ARGS=..."
	@echo "Example:   make test BUILD_TYPE=Debug CMAKE_ARGS=-DTINYADV_SANITIZE=ON"
