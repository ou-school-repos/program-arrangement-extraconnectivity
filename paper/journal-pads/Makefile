SHELL=/bin/bash

.PHONY: lint
lint:
	-mdl -r ~MD013,~MD025,~MD033,~MD034,~MD055,~MD056,~MD057 *.md
	-flake8 $(shell git ls-files \*.py)

.PHONY: format
format:
	shfmt -w $(shell git ls-files \*.sh)
	black $(shell git ls-files \*.py)


loc ?=

.PHONY: archive
archive:
	# usage: loc=. make archive
	bash etc/archive.sh "${loc}"


.PHONY: unarchive
unarchive:
	# usage: loc=<path> make archive
	test "${loc}"
	bash etc/unarchive.sh "${loc}"


BUILD_FILE_FULL ?= potd-and-solutions_sta-2226.md
BUILD_FILE_BASE ?= $(shell basename ${BUILD_FILE_FULL} .md)

.PHONY: build
build:
	pandoc "${BUILD_FILE_FULL}" \
	  -V geometry:margin=0.75in \
	  -V documentclass=extarticle \
	  -V colorlinks=true \
	  -V title="$$BUILD_FILE_BASE" \
	  -o "${BUILD_FILE_BASE}".pdf


clean_locs ?= $(shell find . -type d -wholename '**out/xml')

.PHONY: clean
clean:
	find ${clean_locs} -name \*.xopp | xargs rm
