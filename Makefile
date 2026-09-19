SHELL:=/bin/bash
.DEFAULT_GOAL := _check/default
.SHELLFLAGS := -o pipefail -c

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Variables
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O3 -march=native -Wall -Wextra -Wpedantic -Werror=unknown-pragmas -Wconversion -Wshadow
LDFLAGS  ?=

VERSION   ?= 0.1.0
GIT_COMMIT ?= $(shell git describe --always --dirty --abbrev=12 2>/dev/null || echo unknown)
BUILD_ID  ?= $(VERSION) ($(GIT_COMMIT))
CPPFLAGS += -DBUILD_VERSION='"$(BUILD_ID)"'
$(info Build version: $(BUILD_ID))

# Machine-local configuration (such as an /opt OR-Tools installation) is
# supplied by the caller's environment (for example, through direnv/.envrc).

R          ?= 6  # up to 7 or 8 is fine. 9 and 10 are very slow on the old script.
I          ?= 2
K          ?= 127
DOCS_SRC   ?= $(wildcard README.md docs/*.md)
DOCS_PDF   ?= $(DOCS_SRC:.md=.pdf)
BUNDLE_OUT ?= bundle.zip
SITE_OUT   ?= site.zip

SRCS := $(wildcard src/*.cpp)
BINS := $(patsubst src/%.cpp,bin/%,$(SRCS))
HDRS := $(wildcard include/*.hpp)

# Build profiles are inferred from the source's own includes/pragmas.
H := \#
uses = $(patsubst src/%.cpp,bin/%,$(shell grep -lE '^[[:space:]]*$(H)[[:space:]]*($(1))' $(SRCS) /dev/null))
# OMP_DEPS: bfs_utils.hpp currently contains OpenMP-dependent operations.
OMP_BINS     := $(call uses,include[[:space:]]*omp[.]h|pragma[[:space:]]+omp|include.*bfs_utils[.]hpp)
NAUTY_BINS   := $(call uses,include.*nauty[a-z]*\.h)
ORTOOLS_BINS := $(call uses,include.*ortools/)
Z3_BINS      := $(call uses,include.*z3)

ORTOOLS_CFLAGS ?= $(shell pkg-config --cflags ortools 2>/dev/null)
ORTOOLS_LIBS   ?= $(shell pkg-config --libs ortools 2>/dev/null)
Z3_LIBS        ?= $(shell pkg-config --libs z3 2>/dev/null)

$(OMP_BINS):     DEP_CXXFLAGS += -fopenmp
$(NAUTY_BINS):   DEP_CPPFLAGS += -I/usr/include/nauty
$(NAUTY_BINS):   DEP_LIBS     += -lnauty
$(ORTOOLS_BINS): DEP_CPPFLAGS += $(patsubst -I%,-isystem %,$(ORTOOLS_CFLAGS))
$(ORTOOLS_BINS): DEP_LIBS     += $(ORTOOLS_LIBS)
$(Z3_BINS):      DEP_LIBS     += $(Z3_LIBS)

SKIP := $(if $(strip $(ORTOOLS_LIBS)),,$(ORTOOLS_BINS)) \
        $(if $(strip $(Z3_LIBS)),,$(Z3_BINS))

# Build modes (set once, below in Build section)
DBGFLAGS  ?= -g -O0 -fsanitize=address,undefined


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Help
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.PHONY: help
help:
	@printf '\nUsage: make <command>, valid commands:\n'
	@awk -f scripts/make-help.awk $(MAKEFILE_LIST)


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Print Helpers
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
define print_err
	printf "\033[1;31m%s\033[0m\n" "$(1)"
endef

define print_warn
	printf "\033[1;33m%s\033[0m\n" "$(1)"
endef

define print_success
	printf "\033[1;34m✓ %s\033[0m\n" "$(1)"
endef

define print_info
	printf "\033[1;36m%s\033[0m\n" "$(1)"
endef


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Dev
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.PHONY: build
build: $(filter-out $(SKIP),$(BINS)) ##H @Dev Build all tools (optional solver tools when configured)
	@if [ -n "$(strip $(SKIP))" ]; then \
		echo "Skipping unavailable solver tools: $(notdir $(SKIP))"; \
	fi

# NOTE: Generic targets
$(BINS): bin/%: src/%.cpp
	@mkdir -p $(@D)
	$(CXX) -I./include -MMD -MP -MF $@.d $(CPPFLAGS) $(DEP_CPPFLAGS) $(CXXFLAGS) $(DEP_CXXFLAGS) -o $@ $< $(LDFLAGS) $(LDLIBS) $(DEP_LIBS)

-include $(BINS:=.d)

TEST_BINS := bin/test_star_sweep_checkpoint \
	bin/test_validate_extra_cut_checkpoint bin/test_pattern_catalogue
bin/test_star_sweep_checkpoint: CXXFLAGS += -fopenmp
$(TEST_BINS): bin/test_%: tests/test_%.cpp
	@mkdir -p $(@D)
	$(CXX) -I./include $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -MF $@.d -o $@ $<
-include $(TEST_BINS:=.d)


LINT_SRCS_CPP ?= $(shell git ls-files '*.cpp' '*.c' '*.cc' '*.h' '*.hpp')
LINT_SRCS_CLANG ?= $(filter-out paper/predict_core_sample.cpp,$(LINT_SRCS_CPP))
LINT_SRCS_PY ?= $(shell git ls-files '*.py')
LINT_SRCS_PRETTIER ?= $(shell git ls-files .clang-format '*.json' '.*.y*ml' '*.md')
LINT_SRCS_SH ?= $(shell git ls-files '*.sh')

.PHONY: lint
lint:	##H @Dev Lint C++ sources (cppcheck + clang-tidy)
	@$(call print_info,Linting)
	mkdir -p .tmp/
	cppcheck --language=c++ --std=c++17 --quiet \
		--enable=information,performance,portability,style,unusedFunction,warning \
		--check-level=exhaustive --checkers-report=.tmp/cppcheck-checkers.txt \
		$(LINT_SRCS_CPP) | tee .tmp/out-lint-all.log
	flake8 --jobs=1 $(LINT_SRCS_PY)
	-shellcheck $(LINT_SRCS_SH)
	@$(call print_success,Lint complete.)

.PHONY: _lint/clang
_lint/clang: ##H @Dev Run clang-tidy lint only
	mkdir -p .tmp/
	clang-tidy $(LINT_SRCS_CLANG) --checks='*,-llvmlibc-*,-fuchsia-*,-altera-*,-boost-*,-llvm-*,-google-readability-braces-around-statements,-hicpp-braces-around-statements,-readability-braces-around-statements' -- -I./include $(CPPFLAGS) $(CXXFLAGS) -fopenmp -I/usr/include/nauty $(patsubst -I%,-isystem %,$(ORTOOLS_CFLAGS)) 2>&1 | tee .tmp/out-lint-clang.log

.PHONY: _lint/pylint
_lint/pylint:	##H @Dev Run pylint only
	pylint $(LINT_SRCS_PY)

.PHONY: _lint/mypy
_lint/mypy:	##H @Dev Run mypy only
	mypy $(LINT_SRCS_PY)


.PHONY: format
format:	##H @Dev Format C++ sources (clang-format)
	@$(call print_info,Formatting)
	find . -not -path '*/.lake/*' -name '*.md' -exec sed -i 's/[[:space:]]*$$//' {} +
	-prettier --prose-wrap always --print-width 80 --write -w $(LINT_SRCS_PRETTIER)
	-black $(LINT_SRCS_PY)
	-isort $(LINT_SRCS_PY)
	-ruff format $(LINT_SRCS_PY)
	-ruff check --fix $(LINT_SRCS_PY)
	-pre-commit run --all-files
	-shfmt -w $(LINT_SRCS_SH)
	clang-format -i $(LINT_SRCS_CPP)
	@$(call print_success,Format complete.)

# The default, most useful target (thumb/pinky saver)
.PHONY: _check/default
_check/default: format lint

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Test
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# .PHONY: benchmark
# benchmark: build	##H @Run Benchmark search for R: 2..$(R)
# 	@$(call print_info,Benchmarking $(BIN_OPT) R=2..$(R))
# 	for i in $$(seq 2 $(R)); do ./$(BIN_OPT) $$i; echo ""; done

# NOTE: run a sanitizer test with:
# make test CXXFLAGS="-std=c++17 -g -O0 -fsanitize=address,undefined -Wall -Wextra -Wpedantic"
.PHONY: test
test: bin/predict bin/arrangement bin/pattern_catalogue \
	bin/validate_extra_cut_naive bin/validate_extra_cut_bitmap \
	bin/exact_profile_naive $(TEST_BINS) ##H @Test Run fast test suites
	# Begin test
	python3 tests/test_predict.py --max-r $(R)
	# Begin test
	./bin/test_star_sweep_checkpoint
	# Begin test
	./bin/test_validate_extra_cut_checkpoint ./bin/validate_extra_cut_bitmap
	# Begin test
	# NOTE: run this without --oracle for a full (long ~10 minute) test.
	VALIDATOR_BIN=./bin/validate_extra_cut_naive \
		python3 scripts/test_regression.py --oracle tests/res/oracle_small.json
	# Begin test
	PROFILE_BIN=./bin/exact_profile_naive python3 tests/test_exact_profile.py
	# Begin test
	./bin/test_pattern_catalogue


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Lean
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.PHONY: lean
lean: _lean/cache	##H @Lean Build Lean 4 proofs (proofs/)
	@$(call print_info,Building Lean proofs)
	mkdir -p .tmp
	cd proofs && lake build 2>&1 | tee $(CURDIR)/.tmp/out-build-lean.log
	cd proofs && lake env lean Arrangement/ProofAudit.lean
	@$(call print_success,Lean proofs verified.)

.PHONY: _lean/cache
_lean/cache:	##H @Lean Download pre-built Mathlib cache
	@$(call print_info,Fetching Mathlib cache)
	cd proofs && lake exe cache get
	@$(call print_success,Mathlib cache downloaded.)

.PHONY: _lean/docs-setup
_lean/docs/setup:	##H @Lean Fetch doc-gen4 dependency (run once)
	@$(call print_info,Fetching doc-gen4)
	cd proofs/docbuild && MATHLIB_NO_CACHE_ON_UPDATE=1 lake update doc-gen4
	@$(call print_success,doc-gen4 ready.)

.PHONY: _lean/docs
_lean/docs:	##H @Lean Generate Lean documentation
	@$(call print_info,Generating Lean docs)
	cd proofs/docbuild && lake build Proofs:docs
	@$(call print_success,Lean docs generated in proofs/docbuild/.lake/build/doc/)

.PHONY: _lean/docs-clean
_lean/docs-clean:	##H @Lean Clean project doc cache (fast targeted rebuild)
	@$(call print_info,Cleaning project doc artifacts)
	rm -rf proofs/docbuild/.lake/build/doc/Arrangement \
	       proofs/docbuild/.lake/build/doc/index.html \
	       proofs/docbuild/.lake/build/doc/style.css \
	       proofs/docbuild/.lake/build/doc/doc-manifest.json \
	       proofs/docbuild/.lake/build/api-docs.db
	find proofs/docbuild/.lake/build -path '*Arrangement*' -delete 2>/dev/null || true
	find proofs/docbuild/.lake/build -path '*Proofs*' -delete 2>/dev/null || true
	@$(call print_success,Project doc cache cleared. Run make _lean/docs to rebuild.)


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Clean & Misc
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.PHONY: _paper/render-assets
_paper/render-assets: ##H Render all visual assets (.dot to .png)
	@$(call print_info,Rendering visual assets)
	python3 scripts/render_assets.py

.PHONY: paper
paper:	##H @General Build the LaTeX paper (paper/paper.tex)
	@$(call print_info,Building LaTeX paper)
	set -o pipefail; cd paper && \
		pdflatex -interaction=nonstopmode paper.tex && \
		bibtex paper && \
		pdflatex -interaction=nonstopmode paper.tex && \
		pdflatex -interaction=nonstopmode paper.tex
	@$(call print_success,Paper built: docs/paper/paper.pdf)

.PHONY: _paper/docs
_paper/docs: $(DOCS_PDF)	##H @General Generate PDF documentation from all Markdown files


PDF_ENGINE ?= xelatex

# NOTE: Generic target
%.pdf: %.md
	@$(call print_info,Generating $@ from $<)
	# NOTE: if fails, try with PDF_ENGINE=lualatex
	pandoc $< -o $@ \
		--pdf-engine=${PDF_ENGINE} \
		-V geometry:margin=0.5in \
		-V monofont="DejaVu Sans Mono" \
		-V monofontoptions="Scale=0.8" \
		-V pagestyle=empty
	@$(call print_success,Generated $@)


.PHONY: _bundle/default
_bundle/default: clean ##H @General Create a zip archive of the project sources
	@$(call print_info,Creating $(BUNDLE_OUT))
	rm -f $(BUNDLE_OUT)
	zip -rv9 $(BUNDLE_OUT) README.md $(SRCS) proofs/Arrangement/*.lean scripts/*.py assets/* Makefile
	@$(call print_success,Bundle created.)

.PHONY: _bundle/site
_bundle/site:	##H @General Create site.zip of Lean HTML documentation
	@$(call print_info,Creating $(SITE_OUT))
	rm -f $(SITE_OUT)
	cd proofs/docbuild/.lake/build/doc && zip -r9 ../../../../../$(SITE_OUT) .
	@$(call print_success,Site archive created.)


.PHONY: _csv/base
_csv/base: bin/predict	##H @General Generate docs/predictions.csv (R=2..1024)
	@$(call print_info,Generating predictions CSV)
	./bin/predict --csv 1024 | tee docs/predictions.csv
	@$(call print_success,docs/predictions.csv written.)

.PHONY: _csv/full
_csv/full: bin/predict	##H @General Verified CSV, set R: I..K
	@if [ ! -f docs/verifications.csv ]; then \
		./bin/predict --csv --verify-range $(I) $(K) | tee docs/verifications.csv; \
	else \
		last=$$(tail -1 docs/verifications.csv | cut -d, -f1); \
		next=$$((last + 1)); \
		if [ $$next -le $(K) ]; then \
			./bin/predict --csv --verify-range --no-header $$next $(K) | tee -a docs/verifications.csv; \
		else \
			echo "Already complete up to R=$$last"; \
		fi; \
	fi
	@$(call print_success,docs/verifications.csv — $$(wc -l < docs/verifications.csv) rows.)


GIT_CLEAN_FLAGS := -fdx

ifndef y
GIT_CLEAN_FLAGS += -n
endif

GIT_CLEAN_FLAGS += \
	-e 'proofs/.lake' \
	-e '*.bitmap' \
	-e 'state_*/' \
	-e '.env' \
	-e 'other/' \
	-e '.tmp/' \
	-e 'assets/out/' \
	-e 'paper/archive/' \
	-e 'paper/journal-pads/*.xopp' \
	-e '.agents/' \
	-e '.claude/' \
	-e '.codex/' \

.PHONY: clean
clean:	##H @General Remove build artifacts
	@$(call print_info,Cleaning)
	# Python
	rm -rf bin/ .ruff_cache/ .mypy_cache/
	find . -maxdepth 3 -name __pycache__ -prune -exec rm -rf {} +
	# Docs/paper
	rm -f $(DOCS_PDF) $(BUNDLE_OUT) $(SITE_OUT)
	# General git clean (BE CAREFUL, if you set the var: y)
	git clean $(GIT_CLEAN_FLAGS)
	@$(call print_success,Clean complete.)


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Debug
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.PHONY: _debug/vars
_debug/vars:	##H @General Debug: Print project variables
	@$(foreach v,$(sort $(.VARIABLES)), \
		$(if $(filter file command line override,$(origin $(v))), \
			$(info $(v) = $($(v))) \
		) \
	)

.PHONY: _debug/version
_debug/version:
	@printf '%s\n' '$(BUILD_ID)'
