# Double build systems, you say?
# Consider this the documentation file that contains the authoritative
# instructions for testing, via cmake (the primary build system).

UNAME := $(shell uname)

CMAKE_OPTS :=

# set MAKEOPTS with correct number of cpu threads on linux
ifeq ($(UNAME), Linux)
MAKEOPTS := -j$(shell nproc)
endif

# set MAKEOPTS with correct number of cpu threads on osx/darwin
ifeq ($(UNAME), Darwin)
MAKEOPTS := -j$(shell sysctl machdep.cpu.thread_count | awk '{print $$2}')
endif

.PHONY: docker build test install submodule_init clean

default: defaultbuild

submodule_init:
	git submodule update --init --recursive

docker_build_container:
	docker build \
		--rm=false \
		-t steemitinc/ci-test-environment:latest \
		-f tests/scripts/Dockerfile.testenv \
		.

# docker_build_container is not a prereq here on this target in the file
# (even though it is an actual prerequisite) because on circleci it caches
# that via the setup script called from circle.yml so it may already exist
# on the CI box now without explicitly building it. if it doesn't, it will
# call the docker_build_container rule above to create it.
dockertest:
	docker build \
		--rm=false \
		-t steemitinc/steem-test:latest \
		-f ./Dockerfile.test \
		.

dockerimage:
	docker build \
		--rm=false \
		-t steemitinc/steem:latest \
		.

build/Makefile: submodule_init
	mkdir build && cd build && cmake $(CMAKE_OPTS) ..

# default, vanilla build
defaultbuild: CMAKE_OPTS += -DLOW_MEMORY_NODE=ON
defaultbuild: CMAKE_OPTS += -DENABLE_CONTENT_PATCHING=OFF
defaultbuild: CMAKE_OPTS += -DCLEAR_VOTES=ON
defaultbuild: CMAKE_OPTS += -DCMAKE_BUILD_TYPE=Release
defaultbuild: build/Makefile
	cd build && make $(MAKEOPTS)

# web-frontend supporting build
webbuild: CMAKE_OPTS += -DLOW_MEMORY_NODE=OFF
webbuild: CMAKE_OPTS += -DENABLE_CONTENT_PATCHING=ON
webbuild: CMAKE_OPTS += -DCLEAR_VOTES=OFF
webbuild: CMAKE_OPTS += -DCMAKE_BUILD_TYPE=Release
webbuild: build/Makefile
	cd build && make $(MAKEOPTS)

# testing build
testbuild: CMAKE_OPTS += -DLOW_MEMORY_NODE=OFF
testbuild: CMAKE_OPTS += -DENABLE_CONTENT_PATCHING=ON
testbuild: CMAKE_OPTS += -DBUILD_STEEM_TESTNET=On
testbuild: CMAKE_OPTS += -DCMAKE_BUILD_TYPE=Debug
testbuild: build/Makefile
	cd build && make $(MAKEOPTS) chain_test

test: testbuild
	cd build && ./tests/chain_test

# one-touch testing on osx with virtualbox/docker-machine/docker
# attempts to mirror CircleCI setup in /circle.yml as closely as possible
osxtest:
	bash tests/scripts/osx-run-tests.sh

install: build
	cd build && make install

# remove build directory
clean:
	rm -rf build
