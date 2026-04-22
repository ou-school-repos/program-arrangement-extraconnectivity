SHELL:=/bin/bash
.DEFAULT_GOAL := _help
.SHELLFLAGS = -ec

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Variables
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic
LDFLAGS  ?=
SRC       = cheng/arrangement.cpp
BIN       = arrangement

SRC_OPT   = arrangementoptimized.cpp
BIN_OPT   = arrangementoptimized
R         ?= 8
DOCS_SRC  = README.md
DOCS_OUT  = README.pdf
BUNDLE_OUT = bundle.zip
SITE_OUT   = site.zip

SRC_PRED  = predict.cpp
BIN_PRED  = predict

# Build modes
OPTFLAGS  ?= -O2
DBGFLAGS  ?= -g -O0 -fsanitize=address,undefined

# nauty (canonical graph labeling)
NAUTY_CFLAGS = -I/usr/include/nauty
NAUTY_LIBS   = -lnauty

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Help
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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
.PHONY: build
build:	##H @Build Compile original with optimizations (-O2)
	@$(call print_info,Building $(BIN))
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) $(LDFLAGS) -o $(BIN) $(SRC)
	@$(call print_success,Build complete.)

.PHONY: build/opt
build/opt:	##H @Build Compile optimized variant (-O2)
	@$(call print_info,Building $(BIN_OPT))
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) $(NAUTY_CFLAGS) $(LDFLAGS) -o $(BIN_OPT) $(SRC_OPT) $(NAUTY_LIBS)
	@$(call print_success,Build complete.)

.PHONY: build/predict
build/predict:	##H @Build Compile Hamming ball predictor
	@$(call print_info,Building $(BIN_PRED))
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) $(LDFLAGS) -o $(BIN_PRED) $(SRC_PRED)
	@$(call print_success,Build complete.)

.PHONY: debug
debug:	##H @Build Compile original with debug symbols and sanitizers
	@$(call print_info,Building $(BIN) (debug))
	$(CXX) $(CXXFLAGS) $(DBGFLAGS) $(LDFLAGS) -o $(BIN) $(SRC)
	@$(call print_success,Debug build complete.)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Run
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.PHONY: run
run: build	##H @Run Build and run original
	@$(call print_info,Running $(BIN))
	./$(BIN)

.PHONY: run/opt
run/opt: build/opt	##H @Run Build and run optimized (R=$(R))
	@$(call print_info,Running $(BIN_OPT) R=$(R))
	./$(BIN_OPT) $(R)

.PHONY: run/debug
run/debug: debug	##H @Run Build (debug) and run
	@$(call print_info,Running $(BIN) (debug))
	./$(BIN)

.PHONY: benchmark
benchmark: build/opt	##H @Run Benchmark optimized for R=2..$(R)
	@$(call print_info,Benchmarking $(BIN_OPT) R=2..$(R))
	for i in $$(seq 2 $(R)); do ./$(BIN_OPT) $$i; echo ""; done

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Test
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
EXPECTED_OUTPUT := "(5nk-4) (n-k)-10, EX: ABCDE FBCDE AGCDE ABHDE ABCIE \n(5nk-5) (n-k)-7, EX: ABCDE FBCDE AGCDE ABHDE FGCDE "

.PHONY: test
test: build	##H @Dev Verify original output matches expected
	@$(call print_info,Testing $(BIN))
	actual=$$(./$(BIN)); \
	expected=$$(printf $(EXPECTED_OUTPUT)); \
	if [ "$$actual" = "$$expected" ]; then \
		$(call print_success,All tests passed.); \
	else \
		$(call print_err,Test FAILED — output mismatch:); \
		echo "--- expected ---"; \
		printf $(EXPECTED_OUTPUT); echo; \
		echo "--- actual ---"; \
		echo "$$actual"; \
		exit 1; \
	fi

.PHONY: test/opt
test/opt: build build/opt	##H @Dev Verify optimized output matches original
	@$(call print_info,Testing $(BIN_OPT) against $(BIN))
	expected=$$(./$(BIN) | tr -d ' '); \
	actual=$$(./$(BIN_OPT) 2>/dev/null | tr -d ' '); \
	echo "--- original ---"; echo "$$expected"; \
	echo "--- optimized ---"; echo "$$actual"; \
	if [ "$$actual" = "$$expected" ]; then \
		$(call print_success,Optimized matches original.); \
	else \
		$(call print_err,Output mismatch:); \
		exit 1; \
	fi

.PHONY: test/predict
test/predict: build/opt build/predict	##H @Dev Verify predictor matches search for R=2..9
	@$(call print_info,Testing $(BIN_PRED) against $(BIN_OPT))
	@fail=0; \
	for r in $$(seq 2 9); do \
		expected=$$(./$(BIN_OPT) $$r 2>/dev/null | tail -1 | tr -d ' '); \
		actual=$$(./$(BIN_PRED) $$r 2>/dev/null | tr -d ' '); \
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

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Lint & Format
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.PHONY: lint
lint:	##H @Dev Lint C++ sources (cppcheck + clang-tidy)
	@$(call print_info,Linting)
	-cppcheck --std=c++17 --enable=warning,style,performance --quiet $(SRC_OPT) $(SRC_PRED) | tee lint.log
	-clang-tidy $(SRC_OPT) $(SRC_PRED) --checks='*,-llvmlibc-*,-fuchsia-*,-altera-*,-boost-*,-llvm-*' -- $(CXXFLAGS) $(NAUTY_CFLAGS) | tee -a lint.log
	@$(call print_success,Lint complete.)

.PHONY: format
format:	##H @Dev Format C++ sources (clang-format)
	@$(call print_info,Formatting)
	find . -name '*.md' -exec sed -i 's/[[:space:]]*$$//' {} +
	-prettier -w .
	-pre-commit run --all-files
	clang-format -i $(SRC_OPT) $(SRC_PRED)
	@$(call print_success,Format complete.)


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Lean 4 Proofs
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.PHONY: lean
lean:	##H @Build Build Lean 4 proofs (proofs/)
	@$(call print_info,Building Lean proofs)
	cd proofs && lake build
	@$(call print_success,Lean proofs verified.)

.PHONY: lean/cache
lean/cache:	##H @Build Download pre-built Mathlib cache
	@$(call print_info,Fetching Mathlib cache)
	cd proofs && lake exe cache get
	@$(call print_success,Mathlib cache downloaded.)

.PHONY: lean/docs/setup
lean/docs/setup:	##H @Build Fetch doc-gen4 dependency (run once)
	@$(call print_info,Fetching doc-gen4)
	cd proofs/docbuild && MATHLIB_NO_CACHE_ON_UPDATE=1 lake update doc-gen4
	@$(call print_success,doc-gen4 ready.)

.PHONY: lean/docs
lean/docs:	##H @Build Generate Lean documentation
	@$(call print_info,Generating Lean docs)
	cd proofs/docbuild && lake build Proofs:docs
	@$(call print_success,Lean docs generated in proofs/docbuild/.lake/build/doc/)

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
bundle:	##H @General Create a zip archive of the project sources
	@$(call print_info,Creating $(BUNDLE_OUT))
	rm -f $(BUNDLE_OUT)
	zip -rv9 $(BUNDLE_OUT) \
		README.md README.pdf arrangementoptimized.cpp predict.cpp \
		cheng/arrangement.cpp proofs/*.lean \
		-x proofs/lakefile.lean
	# Uncomment to include
	# zip -rv9 $(BUNDLE_OUT) docs/
	# zip -rv9 $(BUNDLE_OUT) .git/
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
	rm -f $(BIN) $(BIN_OPT) $(BIN_PRED) *.o *.d *.gch *.class $(DOCS_OUT) $(BUNDLE_OUT) $(SITE_OUT)
	@$(call print_success,Clean complete.)

.PHONY: vars
vars:	##H @General Debug: Print project variables
	@$(foreach v,$(sort $(.VARIABLES)), \
		$(if $(filter file command line override,$(origin $(v))), \
			$(info $(v) = $($(v))) \
		) \
	)
