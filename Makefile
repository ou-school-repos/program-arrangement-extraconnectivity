SHELL:=/bin/bash
.DEFAULT_GOAL := _help
.SHELLFLAGS = -ec

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Variables
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O3 -march=native -Wall -Wextra -Wpedantic -Werror=unknown-pragmas
LDFLAGS  ?=

VERSION   ?= 0.1.0
GIT_COMMIT ?= $(shell git describe --always --dirty --abbrev=12 2>/dev/null || echo unknown)
BUILD_ID  ?= $(VERSION) ($(GIT_COMMIT))
CPPFLAGS += -DBUILD_VERSION='"$(BUILD_ID)"'
$(info Build version: $(BUILD_ID))

# Machine-local configuration (such as an /opt OR-Tools installation) is
# supplied by the caller's environment (for example, through direnv/.envrc).

R          ?= 8
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
OMP_BINS     := $(call uses,include[[:space:]]*[<"]omp\.h|pragma[[:space:]]+omp)
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

.PHONY: version
version:
	@printf '%s\n' '$(BUILD_ID)'

.PHONY: _help
_help:
	@printf "\nUsage: make <command>, valid commands:\n\n"
	@awk 'BEGIN {FS = ":.*?##H "}; \
		/##H/ && !/@awk.*?##H/ && $$0 !~ /^[[:space:]]*#/ { \
			target=$$1; doc=$$2; \
			category="General"; \
			if (doc ~ /^@/) { \
				category=substr(doc, 2, index(doc, " ")-2); \
				doc=substr(doc, index(doc, " ")+1); \
			} \
			if (length(target) > max) max = length(target); \
			targets[NR] = target; docs[NR] = doc; cats[NR] = category; \
		} \
		END { \
			last_cat = ""; \
			for (i = 1; i <= NR; i++) { \
				if (cats[i] != "") { \
					if (cats[i] != last_cat) { \
						printf "\n\033[1;36m%s Commands:\033[0m\n", cats[i]; \
						last_cat = cats[i]; \
					} \
					printf "  \033[1;34m%-*s\033[0m  %s\n", max, targets[i], docs[i]; \
				} \
			} \
			print ""; \
		}' $(MAKEFILE_LIST)

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

$(BINS): bin/%: src/%.cpp
	@mkdir -p $(@D)
	$(CXX) -I./include -MMD -MP -MF $@.d $(CPPFLAGS) $(DEP_CPPFLAGS) $(CXXFLAGS) $(DEP_CXXFLAGS) -o $@ $< $(LDFLAGS) $(LDLIBS) $(DEP_LIBS)

-include $(BINS:=.d)

LINT_SRCS := $(shell git ls-files '*.cpp' '*.c' '*.cc' '*.h' '*.hpp')

.PHONY: lint
lint:	##H @Dev Lint C++ sources (cppcheck + clang-tidy)
	@$(call print_info,Linting)
	mkdir -p .tmp/
	cppcheck --language=c++ --std=c++17 \
		--enable=information,performance,portability,style,unusedFunction,warning \
		--check-level=exhaustive --checkers-report=.tmp/cppcheck-checkers.txt \
		--quiet $(LINT_SRCS) | tee lint.log
	flake8 --jobs=1 $$(git ls-files '*.py')
	@$(call print_success,Lint complete.)

.PHONY: clang
clang: ##H @Dev Run clang-tidy lint only
	clang-tidy $(LINT_SRCS) --checks='*,-llvmlibc-*,-fuchsia-*,-altera-*,-boost-*,-llvm-*' -- -I./include $(CPPFLAGS) $(CXXFLAGS) -I/usr/include/nauty $(patsubst -I%,-isystem %,$(ORTOOLS_CFLAGS)) | tee -a lint.log

.PHONY: pylint
pylint:	##H @Dev Run pylint only
	pylint $$(git ls-files '*.py')

.PHONY: mypy
mypy:	##H @Dev Run mypy only
	mypy $$(git ls-files '*.py')

.PHONY: format
format:	##H @Dev Format C++ sources (clang-format)
	@$(call print_info,Formatting)
	find . -not -path '*/.lake/*' -name '*.md' -exec sed -i 's/[[:space:]]*$$//' {} +
	-prettier -w $$(git ls-files .clang-format '*.json' '.*.y*ml' '*.md')
	-black $$(git ls-files '*.py')
	-isort $$(git ls-files '*.py')
	-ruff format $$(git ls-files '*.py')
	-ruff check --fix $$(git ls-files '*.py')
	-pre-commit run --all-files
	-shfmt -w $$(git ls-files '*.sh')
	clang-format -i $(LINT_SRCS)
	@$(call print_success,Format complete.)


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Test
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# .PHONY: benchmark
# benchmark: build	##H @Run Benchmark search for R=2..$(R)
# 	@$(call print_info,Benchmarking $(BIN_OPT) R=2..$(R))
# 	for i in $$(seq 2 $(R)); do ./$(BIN_OPT) $$i; echo ""; done

.PHONY: test/predict
test/predict: build	##H @Test Verify predictor matches search for R=2..$(R)
	@$(call print_info,Testing $(BIN_PRED) against $(BIN_OPT))
	@fail=0; \
	for r in $$(seq 2 $(R)); do \
		expected=$$(./$(BIN_OPT) $$r 2>/dev/null | grep 'EX:' | tail -1 | sed 's/,.*//' | tr -d ' '); \
		actual=$$(./$(BIN_PRED) $$r 2>/dev/null | sed 's/,.*//' | tr -d ' '); \
		if [ "$$actual" = "$$expected" ]; then \
			$(call print_success,R=$$r: prediction matches search.); \
		else \
			$(call print_err,R=$$r: mismatch); \
			echo "  search:  $$expected"; \
			echo "  predict: $$actual"; \
			fail=1; \
		fi; \
	done; \
	if [ $$fail -eq 1 ]; then exit 1; fi

.PHONY: test/validate_extra_cut
test/validate_extra_cut: bin/validate_extra_cut	##H @Test Compare validator output with tests/oracle_baseline.json
	python3 scripts/test_regression.py


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Lean
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.PHONY: lean
lean:	##H @Lean Build Lean 4 proofs (proofs/)
	@$(call print_info,Building Lean proofs)
	set -o pipefail; cd proofs && lake build | tee lean.log
	@printf "\n\033[1;32m--- Verification Complete ---\033[0m\n"
	@printf "\033[1;36mMapped Theorems & Definitions:\033[0m\n"
	@awk 'BEGIN {last_file=""} \
		/^(theorem|lemma|def|axiom|class|instance|structure) / { \
			if (in_decl) process_buf(); \
			buf = $$0; in_decl = 1; \
			if (buf ~ /(:=|:= by|by|where|=>)/) process_buf(); \
			next; \
		} \
		in_decl { \
			gsub(/^[[:space:]]+/, " ", $$0); \
			buf = buf $$0; \
			if ($$0 ~ /(:=|:= by|by|where|=>)/) process_buf(); \
		} \
		function process_buf() { \
			gsub(/[[:space:]]+/, " ", buf); \
			file = FILENAME; sub(/^proofs\//, "", file); \
			if (file != last_file) { \
				printf "\n\033[1;33m%s:\033[0m\n", file; \
				last_file = file; \
			} \
			printf "  %s\n", buf; \
			buf = ""; in_decl = 0; \
		} \
		END { if (in_decl) process_buf(); }' \
		proofs/Arrangement/*.lean proofs/Arrangement/unstable/*.lean 2>/dev/null || true
	@printf "\033[1;32m--------------------------------\033[0m\n"
	cd proofs && lake env lean Arrangement/ProofAudit.lean
	@$(call print_success,Lean proofs verified.)

.PHONY: lean/cache
lean/cache:	##H @Lean Download pre-built Mathlib cache
	@$(call print_info,Fetching Mathlib cache)
	cd proofs && lake exe cache get
	@$(call print_success,Mathlib cache downloaded.)

.PHONY: _lean/docs/setup
_lean/docs/setup:	##H @Lean Fetch doc-gen4 dependency (run once)
	@$(call print_info,Fetching doc-gen4)
	cd proofs/docbuild && MATHLIB_NO_CACHE_ON_UPDATE=1 lake update doc-gen4
	@$(call print_success,doc-gen4 ready.)

.PHONY: _lean/docs
_lean/docs:	##H @Lean Generate Lean documentation
	@$(call print_info,Generating Lean docs)
	cd proofs/docbuild && lake build Proofs:docs
	@$(call print_success,Lean docs generated in proofs/docbuild/.lake/build/doc/)

.PHONY: _lean/docs/clean
_lean/docs/clean:	##H @Lean Clean project doc cache (fast targeted rebuild)
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

.PHONY: render
render: ##H Render all visual assets (.dot to .png)
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

.PHONY: docs
docs: $(DOCS_PDF)	##H @General Generate PDF documentation from all Markdown files


.PHONY: csv
csv: build	##H @General Generate docs/predictions.csv (R=2..1024)
	@$(call print_info,Generating predictions CSV)
	./$(BIN_PRED) --csv 1024 | tee docs/predictions.csv
	@$(call print_success,docs/predictions.csv written.)

.PHONY: csv/full
csv/full: build	##H @General Verified CSV R=I..K → docs/verifications.csv (I=$(I) K=$(K))
	@if [ ! -f docs/verifications.csv ]; then \
		./$(BIN_PRED) --csv --verify-range $(I) $(K) | tee docs/verifications.csv; \
	else \
		last=$$(tail -1 docs/verifications.csv | cut -d, -f1); \
		next=$$((last + 1)); \
		if [ $$next -le $(K) ]; then \
			./$(BIN_PRED) --csv --verify-range --no-header $$next $(K) | tee -a docs/verifications.csv; \
		else \
			echo "Already complete up to R=$$last"; \
		fi; \
	fi
	@$(call print_success,docs/verifications.csv — $$(wc -l < docs/verifications.csv) rows.)


PDF_ENGINE ?= xelatex

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

.PHONY: bundle
bundle: clean ##H @General Create a zip archive of the project sources
	@$(call print_info,Creating $(BUNDLE_OUT))
	rm -f $(BUNDLE_OUT)
	zip -rv9 $(BUNDLE_OUT) README.md $(SRCS) proofs/Arrangement/*.lean scripts/*.py assets/* Makefile
	@$(call print_success,Bundle created.)

.PHONY: site
site:	##H @General Create site.zip of Lean HTML documentation
	@$(call print_info,Creating $(SITE_OUT))
	rm -f $(SITE_OUT)
	cd proofs/docbuild/.lake/build/doc && zip -r9 ../../../../../$(SITE_OUT) .
	@$(call print_success,Site archive created.)

.PHONY: clean
clean:	##H @General Remove build artifacts
	@$(call print_info,Cleaning)
	rm -rf .ruff_cache/ .mypy_cache/
	find . -maxdepth 3 -name __pycache__
	rm -f $(BINS) $(BINS:=.d) *.o *.d *.gch *.class $(DOCS_PDF) $(BUNDLE_OUT) $(SITE_OUT)
	@$(call print_success,Clean complete.)


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Debug
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.PHONY: vars
vars:	##H @General Debug: Print project variables
	@$(foreach v,$(sort $(.VARIABLES)), \
		$(if $(filter file command line override,$(origin $(v))), \
			$(info $(v) = $($(v))) \
		) \
	)
