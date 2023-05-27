COMMIT=$(shell git rev-parse --short HEAD)
BRANCH=$(shell git rev-parse --abbrev-ref HEAD)
BUILD_TIME=$(shell date)
VERSION_H=src/version.h

PROJECT_NAME=reload
THIS_OS=windows
THIS_ARCH=x86

TOOL_PATH=.\tools\bin\windows\x86
TOOL_BUILD=vs2019
TOOL_PLATFORM=windows

ifeq ($(OS),Windows_NT)
	THIS_OS=windows
else
	UNAME_S := $(shell uname -s)
	THIS_ARCH=$(shell uname -m)
	ifeq ($(UNAME_S),Linux)
		THIS_OS=linux
	endif
	ifeq ($(UNAME_S),Darwin)
		THIS_OS=darwin
	endif
endif

ifeq (${THIS_OS},darwin)
	TOOL_BUILD=xcode4
	TOOL_PATH=./tools/bin/osx/x86
	TOOL_PLATFORM=macosx
	ifeq (${THIS_ARCH},arm64)
		TOOL_PATH=./tools/bin/osx/arm
		TOOL_PLATFORM=macosxARM64
	endif
endif
ifeq (${THIS_OS},linux)
	TOOL_BUILD=gmake
	TOOL_PATH=./tools/bin/unix/x86
	TOOL_PLATFORM=linux64
	ifeq (${THIS_ARCH},aarch64)
		TOOL_PATH=./tools/bin/unix/arm
		TOOL_PLATFORM=linuxARM64
	endif
endif

details:
	@echo "Detected OS: ${THIS_OS}"
	@echo "Detected Arch: ${THIS_ARCH}"
	@echo "Tool Path: ${TOOL_PATH}"
	@echo "Tool Build: ${TOOL_BUILD}"
	@echo "Tool Platform: ${TOOL_PLATFORM}"
	
	@echo "Commit: ${COMMIT}"
	@echo "Branch: ${BRANCH}"
	@echo "-----"

install-deps:
	./install.sh

run:
	./build/${PROJECT_NAME} ${SCRIPT}

run-test:
	./build/test

gen: details
	@echo "Setting Versions: Commit: ${COMMIT} Branch: ${BRANCH}"

# Updating the commit info in version.h
	git checkout ${VERSION_H}
	${TOOL_PATH}/version -commit=${COMMIT} -branch=${BRANCH} -display=true -version_file=${VERSION_H}
# Generating build projects
	@echo "Tool Platform: ${TOOL_PLATFORM}"
	${TOOL_PATH}/premake5 ${TOOL_BUILD} platform=${TOOL_PLATFORM}
	@echo "Gen Finished: Commit: ${COMMIT} Branch: ${BRANCH}"

post-build:
	git checkout src/version.h

open:
	start .\projects\${PROJECT_NAME}.sln

binary-debug:
ifeq (${THIS_OS},windows)
	msbuild.exe ./projects/${PROJECT_NAME}.sln -p:Platform="windows";Configuration=Debug -target:${PROJECT_NAME}
endif
ifeq (${THIS_OS},darwin)
	xcodebuild -configuration "Debug" ARCHS="${THIS_ARCH}" -destination 'platform=macOS' -project "projects/${PROJECT_NAME}.xcodeproj" -target ${PROJECT_NAME}
endif
ifeq (${THIS_OS},linux)
	make -C projects ${PROJECT_NAME} config=debug_linux64
endif

binary-release: gen
ifeq (${THIS_OS},windows)
	msbuild.exe ./projects/${PROJECT_NAME}.sln -p:Platform="windows";Configuration=Release -target:${PROJECT_NAME}
endif
ifeq (${THIS_OS},darwin)
	xcodebuild -configuration "Release" ARCHS="x86_64" -destination 'platform=macOS' -project "projects/${PROJECT_NAME}.xcodeproj" -target ${PROJECT_NAME}
endif
ifeq (${THIS_OS},linux)
	make -C projects ${PROJECT_NAME} config=release_linux64
endif

build-test: gen
ifeq (${THIS_OS},windows)
	msbuild.exe ./projects/${PROJECT_NAME}.sln -p:Platform="windows";Configuration=Debug -target:test
endif
ifeq (${THIS_OS},darwin)
	xcodebuild -configuration "Debug" ARCHS="x86_64" -destination 'platform=macOS' -project "projects/test.xcodeproj" -target test
endif
ifeq (${THIS_OS},linux)
	make -C projects test config=debug_linux64
endif

build-release: gen binary-release post-build
build-debug: gen binary-debug post-build

# by default build a debug binary
build: build-debug

test: build-test post-build
ifeq (${THIS_OS},windows)
	.\build\test.exe
endif
ifeq (${THIS_OS},darwin)
	./build/test
endif
ifeq (${THIS_OS},linux)
	./build/test
endif

clean-${PROJECT_NAME}:
ifeq (${THIS_OS},windows)
	rm -r build/${PROJECT_NAME}.exe
	rm -r projects/obj/Windows/Debug/${PROJECT_NAME}
	rm -r projects/obj/Windows/Release/${PROJECT_NAME}
else
	rm -f ./build/${PROJECT_NAME}
ifeq (${THIS_OS},linux)
	rm -Rf ./projects/obj/linux64/Debug/${PROJECT_NAME}
	rm -Rf ./projects/obj/linux64/Release/${PROJECT_NAME}
endif
ifeq (${THIS_OS},darwin)
	rm -f ./build/${PROJECT_NAME}
	rm -Rf ./projects/obj/macosx/Debug/${PROJECT_NAME}
	rm -Rf ./projects/obj/macosx/Release/${PROJECT_NAME}
endif
endif

clean:
ifeq (${THIS_OS},windows)
	rm -r build
	rm -r projects
else
	rm -Rf ./build/
	rm -Rf ./projects
endif

build-tools:
	make -C tools/ all
