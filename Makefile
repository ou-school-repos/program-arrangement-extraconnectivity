SHELL:=/bin/bash
.DEFAULT_GOAL := _help
.SHELLFLAGS = -ec

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Variables
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CXX      = g++
CXXFLAGS = -std=c++17 -O3 -march=native -Wall -Wextra -Wpedantic -fopenmp
LDFLAGS  =

# Machine-local configuration (such as an /opt OR-Tools installation) is
# supplied by the caller's environment (for example, through direnv/.envrc).

R         ?= 8
I         ?= 2
K         ?= 127
DOCS_SRC  = $(wildcard README.md docs/*.md)
DOCS_PDF  = $(DOCS_SRC:.md=.pdf)
BUNDLE_OUT = bundle.zip
SITE_OUT   = site.zip

# All ordinary standalone C/C++ programs are discovered automatically and
# written to bin/.  Adding a new source file therefore needs no Makefile edit.
TOOL_SOURCES = $(wildcard src/*.c src/*.cc src/*.cpp scripts/*.c scripts/*.cc scripts/*.cpp scripts/unstable/*.c scripts/unstable/*.cc scripts/unstable/*.cpp)
TOOL_SOURCES := $(filter-out src/arrangement.cpp \
	src/search_ghosts.cpp src/search_triples.cpp src/search_single.cpp \
	src/profile_telescope_milp.cpp scripts/a10_5_hunt.cpp \
	scripts/a10_5_boost.cpp scripts/a10_5_smt_oracle.cpp,$(TOOL_SOURCES))
TOOL_BINS = $(addprefix bin/,$(basename $(notdir $(TOOL_SOURCES))))

# These names are retained as lightweight aliases for scripts and muscle
# memory.  The actual files live under bin/.
BIN_OPT = bin/arrangement
BIN_PRED = bin/predict
BIN_UNIVERSAL = bin/universal_lower_bound

# Optional dependency: install OR-Tools/Z3 separately before using this group.
OPTIONAL_SOURCES = src/search_ghosts.cpp src/search_triples.cpp \
	src/search_single.cpp src/profile_telescope_milp.cpp \
	scripts/a10_5_hunt.cpp scripts/a10_5_boost.cpp scripts/a10_5_smt_oracle.cpp
OPTIONAL_BINS = $(addprefix bin/,$(basename $(notdir $(OPTIONAL_SOURCES))))

ALL_BINS = $(TOOL_BINS) $(OPTIONAL_BINS) $(BIN_OPT)
LEGACY_PROFILE_BINS = a10_5_profile_dp transfer_dp_prototype fiber_ordering transfer_dp_window
ORTOOLS_CFLAGS ?= $(shell pkg-config --cflags ortools 2>/dev/null)
ORTOOLS_LIBS ?= $(shell pkg-config --libs ortools 2>/dev/null || echo -lortools)
ORTOOLS_ISYSFLAGS = $(subst -I,-isystem ,$(ORTOOLS_CFLAGS))

SRCS ?= $$(git ls-files '*.cpp' '*.c' '*.cc' '*.h' '*.hpp')

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

CERTIFICATE_BUILD ?= /tmp/arrangement-certificates

.PHONY: build tools optional-tools certificates certificates-check fracture-arithmetic-check

# `tools` is the one extensible build target: every ordinary standalone
# source becomes bin/<basename>.  The generated aliases below keep
# `make <tool>` and Make's tab completion convenient without another rule per
# source file.
tools: $(TOOL_BINS) ##H @Build Build all standalone C/C++ tools into bin/

define TOOL_template
bin/$(notdir $(basename $(1))): $(1) $(ARRANGEMENT_HDRS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $$@ $$<

$(notdir $(basename $(1))): bin/$(notdir $(basename $(1)))
endef
$(foreach source,$(TOOL_SOURCES),$(eval $(call TOOL_template,$(source))))

# The nauty executable and the optional solver programs use non-default link
# flags, so they remain explicit, but are grouped rather than mixed into the
# ordinary tool list.
arrangement: $(BIN_OPT) ##H @Build Build the nauty arrangement search
predict: bin/predict ##H @Build Build the predictor
universal_check: bin/universal_lower_bound ##H @Dev Alias for universal_lower_bound

bin/arrangement: src/arrangement.cpp $(ARRANGEMENT_HDRS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(NAUTY_CFLAGS) $(LDFLAGS) -o $@ $< $(NAUTY_LIBS)

optional-tools: $(OPTIONAL_BINS) ##H @Build Build OR-Tools/Z3-dependent tools

define OPTIONAL_template
bin/$(notdir $(basename $(1))): $(1)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(ORTOOLS_ISYSFLAGS) $(LDFLAGS) -o $$@ $$< $(ORTOOLS_LIBS)

$(notdir $(basename $(1))): bin/$(notdir $(basename $(1)))
endef
$(foreach source,$(filter-out scripts/a10_5_smt_oracle.cpp,$(OPTIONAL_SOURCES)),$(eval $(call OPTIONAL_template,$(source))))

bin/a10_5_smt_oracle: scripts/a10_5_smt_oracle.cpp
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $$(shell pkg-config --cflags z3) -o $@ $< $$(shell pkg-config --libs z3)
a10_5_smt_oracle: bin/a10_5_smt_oracle

build: arrangement tools ##H @Build Build the core search and all standalone tools

certificates:	##H @Build Compile the finite certificate/oracle tools
	@mkdir -p $(CERTIFICATE_BUILD)
	$(CXX) -O2 -std=c++17 -Wall -Wextra certificates/fdp_certificate.cpp -o $(CERTIFICATE_BUILD)/fdp_certificate
	$(CXX) -O2 -std=c++17 -Wall -Wextra certificates/max_q_oracle.cpp -o $(CERTIFICATE_BUILD)/max_q_oracle
	$(CXX) -O2 -std=c++17 -Wall -Wextra certificates/audit_orbits.cpp -o $(CERTIFICATE_BUILD)/audit_orbits
	$(CXX) -O2 -std=c++17 -Wall -Wextra certificates/opt_orbits.cpp -o $(CERTIFICATE_BUILD)/opt_orbits

certificates-check: certificates	##H @Test Run the finite certificate/oracle regression suite
	CERT_BIN_DIR=$(CERTIFICATE_BUILD) certificates/soundness_check.sh

fracture-arithmetic-check:	##H @Test Check the finite fracture arithmetic regression
	$(CXX) -O2 -std=c++17 -Wall -Wextra scripts/unstable/fracture_arith_check.cpp -o /tmp/fracture_arith_check
	/tmp/fracture_arith_check 12

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
	-cppcheck --language=c++ --std=c++17 \
		--enable=information,performance,portability,style,unusedFunction,warning \
		--check-level=exhaustive --checkers-report=.cppcheck-checkers.txt \
		--quiet $(SRCS) | tee lint.log
	flake8 --jobs=1 $$(git ls-files '*.py')
	@$(call print_success,Lint complete.)

.PHONY: clang
clang: ##H @Dev Run clang-tidy lint only
	clang-tidy $(SRCS) --checks='*,-llvmlibc-*,-fuchsia-*,-altera-*,-boost-*,-llvm-*' -- $(CXXFLAGS) $(NAUTY_CFLAGS) | tee -a lint.log

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
	clang-format -i $(SRCS)
	@$(call print_success,Format complete.)


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Lean 4 Proofs
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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
	cd proofs && lake env lean Arrangement/ProofAudit.lean
	@$(call print_success,Lean proofs verified.)

.PHONY: cache lean/cache _lean/cache
cache: lean/cache
lean/cache: _lean/cache
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

%.pdf: %.md
	@$(call print_info,Generating $@ from $<)
	pandoc $< -o $@ \
		--pdf-engine=xelatex \
		-V geometry:margin=0.5in \
		-V monofont="DejaVu Sans Mono" \
		-V monofontoptions="Scale=0.8" \
		-V pagestyle=empty || \
	pandoc $< -o $@ \
		--pdf-engine=lualatex \
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
	rm -f $(ALL_BINS) $(LEGACY_PROFILE_BINS) *.o *.d *.gch *.class $(DOCS_PDF) $(BUNDLE_OUT) $(SITE_OUT)
	@$(call print_success,Clean complete.)

# .PHONY: list-unknown-binaries
# list-unknown-binaries:	##H @General List executable top-level files not managed by Make
# 	@find . -maxdepth 1 -type f -executable -printf '%f\n' | \
# 		sort | while read -r file; do \
# 			case " $(ALL_BINS) " in *" $$file "*) ;; \
# 			*) echo "$$file" ;; esac; \
# 		done

.PHONY: vars
vars:	##H @General Debug: Print project variables
	@$(foreach v,$(sort $(.VARIABLES)), \
		$(if $(filter file command line override,$(origin $(v))), \
			$(info $(v) = $($(v))) \
		) \
	)
