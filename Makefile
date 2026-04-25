SHELL:=/bin/bash
.DEFAULT_GOAL := _help
.SHELLFLAGS = -ec

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Variables
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CXX      = g++
CXXFLAGS = -std=c++17 -O3 -march=native -Wall -Wextra -Wpedantic -fopenmp
LDFLAGS  =

SRC_OPT   = src/arrangement.cpp
BIN_OPT   = arrangement
R         ?= 8
I         ?= 2
K         ?= 127
DOCS_SRC  = README.md
DOCS_OUT  = README.pdf
BUNDLE_OUT = bundle.zip
SITE_OUT   = site.zip

SRC_PRED  = src/predict.cpp
BIN_PRED  = predict

SRCS      = $(SRC_OPT) $(SRC_PRED)

# Build modes (set once, below in Build section)
DBGFLAGS  ?= -g -O0 -fsanitize=address,undefined

# nauty (canonical graph labeling)
NAUTY_CFLAGS = -I/usr/include/nauty
NAUTY_LIBS   = -lnauty

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Help
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.PHONY: all
all: format build lean docs lint bundle	##H @Build Run all targets

.PHONY: _help help
help: _help
_help:
	@printf "\nUsage: make <command>, valid commands:\n\n"
	@awk 'BEGIN {FS = ":.*?##H "}; \
		/##H/ && !/@awk.*?##H/ { \
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
# Build
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Header dependencies for arrangement
ARRANGEMENT_HDRS = $(wildcard src/*.h)

.PHONY: build
build: $(BIN_OPT) $(BIN_PRED)	##H @Build Compile all binaries

$(BIN_OPT): EXTRA_CFLAGS = $(NAUTY_CFLAGS)
$(BIN_OPT): EXTRA_LIBS   = $(NAUTY_LIBS)
$(BIN_OPT): $(ARRANGEMENT_HDRS)

$(BIN_OPT) $(BIN_PRED): %: src/%.cpp
	@$(call print_info,Building $@)
	$(CXX) $(CXXFLAGS) $(EXTRA_CFLAGS) $(LDFLAGS) -o $@ $< $(EXTRA_LIBS)
	@$(call print_success,Build complete.)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Run
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.PHONY: run
run: build	##H @Run Build and run optimized (R=$(R))
	@$(call print_info,Running $(BIN_OPT) R=$(R))
	./$(BIN_OPT) $(R)

.PHONY: benchmark
benchmark: build	##H @Run Benchmark search for R=2..$(R)
	@$(call print_info,Benchmarking $(BIN_OPT) R=2..$(R))
	for i in $$(seq 2 $(R)); do ./$(BIN_OPT) $$i; echo ""; done

.PHONY: run/predict
run/predict: build	##H @Run Predict extraconnectivity for R=$(R)
	./$(BIN_PRED) $(R)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Test
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.PHONY: test/predict
test/predict: build	##H @Dev Verify predictor matches search for R=2..$(R)
	@$(call print_info,Testing $(BIN_PRED) against $(BIN_OPT))
	@fail=0; \
	for r in $$(seq 2 $(R)); do \
		expected=$$(./$(BIN_OPT) $$r 2>/dev/null | tail -1 | sed 's/,.*//' | tr -d ' '); \
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

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Lint & Format
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.PHONY: lint
lint:	##H @Dev Lint C++ sources (cppcheck + clang-tidy)
	@$(call print_info,Linting)
	-cppcheck --std=c++17 --enable=warning,style,performance --quiet $(SRCS) | tee lint.log
	#-clang-tidy $(SRCS) --checks='*,-llvmlibc-*,-fuchsia-*,-altera-*,-boost-*,-llvm-*' -- $(CXXFLAGS) $(NAUTY_CFLAGS) | tee -a lint.log
	@$(call print_success,Lint complete.)

.PHONY: format
format:	##H @Dev Format C++ sources (clang-format)
	@$(call print_info,Formatting)
	find . -not -path '*/.lake/*' -name '*.md' -exec sed -i 's/[[:space:]]*$$//' {} +
	-prettier -w .
	-black docs/
	-isort docs/
	-pre-commit run --all-files
	clang-format -i $(SRCS)
	@$(call print_success,Format complete.)


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Lean 4 Proofs
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
LAKE_HOME ?= $(HOME)/.cache/lake
export LAKE_HOME

.PHONY: lean
lean:	##H @Build Build Lean 4 proofs (proofs/)
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
	@$(call print_success,Lean proofs verified.)

.PHONY: _lean/cache
_lean/cache:	##H @Build Download pre-built Mathlib cache
	@$(call print_info,Fetching Mathlib cache)
	cd proofs && lake exe cache get
	@$(call print_success,Mathlib cache downloaded.)

.PHONY: _lean/docs/setup
_lean/docs/setup:	##H @Build Fetch doc-gen4 dependency (run once)
	@$(call print_info,Fetching doc-gen4)
	cd proofs/docbuild && MATHLIB_NO_CACHE_ON_UPDATE=1 lake update doc-gen4
	@$(call print_success,doc-gen4 ready.)

.PHONY: _lean/docs
_lean/docs:	##H @Build Generate Lean documentation
	@$(call print_info,Generating Lean docs)
	cd proofs/docbuild && lake build Proofs:docs
	@$(call print_success,Lean docs generated in proofs/docbuild/.lake/build/doc/)

.PHONY: _lean/docs/clean
_lean/docs/clean:	##H @Build Clean project doc cache (fast targeted rebuild)
	@$(call print_info,Cleaning project doc artifacts)
	rm -rf proofs/docbuild/.lake/build/doc/Arrangement \
	       proofs/docbuild/.lake/build/doc/index.html \
	       proofs/docbuild/.lake/build/doc/style.css \
	       proofs/docbuild/.lake/build/doc/doc-manifest.json \
	       proofs/docbuild/.lake/build/api-docs.db
	find proofs/docbuild/.lake/build -path '*Arrangement*' -delete 2>/dev/null || true
	find proofs/docbuild/.lake/build -path '*Proofs*' -delete 2>/dev/null || true
	@$(call print_success,Project doc cache cleared. Run make lean/docs to rebuild.)

.PHONY: render
render: ##H Render all visual assets (.dot to .png)
	@$(call print_info,Rendering visual assets)
	python3 scripts/render_assets.py

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Clean & Misc
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.PHONY: docs
docs:	##H @General Generate PDF documentation from README
	@$(call print_info,Generating $(DOCS_OUT) from $(DOCS_SRC))
	pandoc $(DOCS_SRC) -o $(DOCS_OUT) \
		--pdf-engine=xelatex \
		-V geometry:margin=0.5in \
		-V monofont="DejaVu Sans Mono" \
		-V monofontoptions="Scale=0.8" \
		-V pagestyle=empty || \
	pandoc $(DOCS_SRC) -o $(DOCS_OUT) \
		--pdf-engine=lualatex \
		-V geometry:margin=0.5in \
		-V monofont="DejaVu Sans Mono" \
		-V monofontoptions="Scale=0.8" \
		-V pagestyle=empty
	@$(call print_success,Documentation generated.)

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
	rm -f $(BIN_OPT) $(BIN_PRED) *.o *.d *.gch *.class $(DOCS_OUT) $(BUNDLE_OUT) $(SITE_OUT)
	@$(call print_success,Clean complete.)

.PHONY: vars
vars:	##H @General Debug: Print project variables
	@$(foreach v,$(sort $(.VARIABLES)), \
		$(if $(filter file command line override,$(origin $(v))), \
			$(info $(v) = $($(v))) \
		) \
	)
