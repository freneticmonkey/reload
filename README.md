# reload

Project to test C livecoding environment. TLDR: It works but versioning data and knowning when state is unrecoverable is difficult.

Note: Currently macOS / linux only due to lack of a windows implementation for <sys/inotify.h>. Might reimplement using libuv in future.

# Dependencies
- [Go](https://go.dev)
- [Mage](https://magefile.org)
- C build system (XCode on macOS)
- Premake (bundled in the tools folder)
- make (brew install make)

# How to use this

    make setup
    make build

This compiles the build scripts into a binary in the 'build' folder. The `make build` command then generates a project and builds the 'reload' executable and basic dynamic library that will be loaded on startup.

    make run

This will start the reload binary and load the basic library. Any code change in basic will trigger a recompilation of the basic library and the resulting code can then be seen executing in the raylib window.

# How this works
Using inotify and path standardisation, a 'module' is defined and mapped to a path.  inotify callbacks are registered by each module. If a callback is invoked, a recompile is trigged using `./build/build` with the module as a parameter. This regenerates the projects and rebuilds the binary. Once this is complete, the existing module is unmounted, and then remounted. Modules can register callbacks which can be used to pass memory between versions of the libraries, allowing for a very basic persistence.

# Limitations
Destructive memory modifications will crash the program. Versioning the data could be used to limit, or detect this scenario, but I've only implemented a very basic implementation.

# Future work
* I'd like to embed the tcc compiler, removing the need for an external build chain.
* Use a serious data serialisation system. Nothing complicated, maybe msgpack or something similar


