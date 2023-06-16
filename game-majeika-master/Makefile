# List of demo programs
# DEMOS = bounce gravity pacman nbodies damping spaceinvaders pegs breakout game
DEMOS = game
# List of C files in "libraries" that we provide
STAFF_LIBS = test_util sdl_wrapper
# List of C files in "libraries" that you will write.
# This also defines the order in which the tests are run.
STUDENT_LIBS = vector list polygon color body scene forces \
							 collision shapes portal connection platform button body_type

# find <dir> is the command to find files in a directory
# ! -name .gitignore tells find to ignore the .gitignore
# -type f only finds files
# -delete deletes all the files found
CLEAN_COMMAND = find out/ ! -name .gitignore -type f -delete && \
find bin/ ! -name .gitignore -type f -delete

# Compiling with asan (run 'make all' as normal)
ifndef NO_ASAN
  CFLAGS = -fsanitize=address
  ifeq ($(wildcard .debug),)
    $(shell $(CLEAN_COMMAND))
    $(shell touch .debug)
  endif
# Compiling without asan (run 'make NO_ASAN=true all')
else
  CFLAGS = -O3
  ifneq ($(wildcard .debug),)
    $(shell $(CLEAN_COMMAND))
    $(shell rm -f .debug)
  endif
endif

# Use clang as the C compiler
CC = clang
# Flags to pass to clang:
# -Iinclude tells clang to look for #include files in the "include" folder
# -Wall turns on all warnings
# -g adds filenames and line numbers to the executable for useful stack traces
# -fno-omit-frame-pointer allows stack traces to be generated
#   (take CS 24 for a full explanation)
CFLAGS += -Iinclude $(shell sdl2-config --cflags) -Wall -g -fno-omit-frame-pointer

# Emscripten compilation section
# Flags to pass to emcc:
# -s EXIT_RUNTIME=1 shuts the program down properly
# -s ALLOW_MEMORY_GROWTH=1 allows for dynamic memory usage
# -s INITIAL_MEMORY sets the initial amount of memory
# -s USE_SDL=2 ports the sdl library.
# Other SDL ports are also included, like image and mixer
# -s ASSERTIONS=1 enables runtime checks for allocation errors
# -O2 specifies to compile the code with full optimizations. When debugging, you can change to O0 to step through code
# -g enables DWARF support, for debugging purposes
# -gsource-map --source-map-base http://localhost:8000/bin/ creates a source map from the C file for debugging
EMCC = emcc
EMCC_FLAGS = -s EXIT_RUNTIME=1 -s ALLOW_MEMORY_GROWTH=1 -s INITIAL_MEMORY=655360000 -s USE_SDL=2 -s USE_SDL_GFX=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='["png"]' -s USE_SDL_TTF=2 -s USE_SDL_MIXER=2 -s ASSERTIONS=1 -O2 -g -gsource-map --use-preload-plugins --preload-file assets --source-map-base http://labradoodle.caltech.edu:$(shell cs3-port)/bin/

# Compiler flag that links the program with the math library
LIB_MATH = -lm
# Compiler flags that link the program with the math library
# Note that $(...) substitutes a variable's value, so this line is equivalent to
# LIBS = -lm
LIBS = $(LIB_MATH) $(shell sdl2-config --libs) -lSDL2 -lSDL2_gfx -lSDL2_ttf -lSDL2_mixer

# List of compiled .o files corresponding to STUDENT_LIBS, e.g. "out/vector.o".
# Don't worry about the syntax; it's just adding "out/" to the start
# and ".o" to the end of each value in STUDENT_LIBS.
STUDENT_OBJS = $(addprefix out/,$(STUDENT_LIBS:=.o))
# List of compiled wasm.o files corresponding to STUDENT_LIBS
# Similarly to above, we add .wasm.o to the end of each value in STUDENT_LIBS
WASM_STUDENT_OBJS = $(addprefix out/,$(STUDENT_LIBS:=.wasm.o))

# List of test suite executables, e.g. "bin/test_suite_vector"
TEST_BINS = $(addprefix bin/test_suite_,$(STUDENT_LIBS))
# List of demo executables, i.e. "bin/bounce.html".
DEMO_BINS = $(addsuffix .html, $(addprefix bin/,$(DEMOS)))

# The first Make rule. It is relatively simple
# It builds the files in TEST_BINS and DEMO_BINS, as well as making the server for the demos
# "To build 'all', make sure all files in TEST_BINS and DEMO_BINS are up to date."
# You can execute this rule by running the command "make all", or just "make".
# all: $(TEST_BINS) $(DEMO_BINS) server
all: $(DEMO_BINS) server

# Make the python server for your demos
# To run this, type 'make server'
server:
	@echo "Go to \033[0;32mhttp://labradoodle.caltech.edu:$(shell cs3-port)/bin/\033[0m to access your demo!" && \
	python3 -m http.server $(shell cs3-port) | grep -v "Serving HTTP"

# Any .o file in "out" is built from the corresponding C file.
# Although .c files can be directly compiled into an executable, first building
# .o files reduces the amount of work needed to rebuild the executable.
# For example, if only list.c was modified since the last build, only list.o
# gets recompiled, and clang reuses the other .o files to build the executable.
#
# "%" means "any string".
# Unlike "all", this target has a build command.
# "$^" is a special variable meaning "the source files"
# and $@ means "the target file", so the command tells clang
# to compile the source C file into the target .o file.
out/%.o: library/%.c # source file may be found in "library"
	$(CC) -c $(CFLAGS) $^ -o $@
out/%.o: demo/%.c # or "demo"
	$(CC) -c $(CFLAGS) $^ -o $@
out/%.o: tests/%.c # or "tests"
	$(CC) -c $(CFLAGS) $^ -o $@

# Emscripten compilation flags
# This is very similar to the above compilation, except for emscripten
out/%.wasm.o: library/%.c # source file may be found in "library"
	$(EMCC) -c $(CFLAGS) $^ -o $@
out/%.wasm.o: demo/%.c # or "demo"
	$(EMCC) -c $(CFLAGS) $^ -o $@
out/%.wasm.o: tests/%.c # or "tests"
	$(EMCC) -c $(CFLAGS) $^ -o $@

# Builds bin/%.html by linking the necessary .wasm.o files.
# Unlike the out/%.wasm.o rule, this uses the LIBS flags and omits the -c flag,
# since it is building a full executable. Also notice it uses our EMCC_FLAGS
bin/%.html: out/emscripten.wasm.o out/%.wasm.o out/sdl_wrapper.wasm.o $(WASM_STUDENT_OBJS)
		$(EMCC) $(EMCC_FLAGS) $(CFLAGS) $(LIBS) $^ -o $@

# Builds the test suite executables from the corresponding test .o file
# and the library .o files. The only difference from the demo build command
# is that it doesn't link the SDL libraries.
bin/test_suite_%: out/test_suite_%.o out/test_util.o out/sdl_wrapper.o $(STUDENT_OBJS) $(STAFF_OBJS)
	$(CC) $(CFLAGS) $(LIBS) $^ -o $@

# Builds the test suite executable for the student tests
bin/student_tests: out/student_tests.o out/test_util.o $(STUDENT_OBJS)
	$(CC) $(CFLAGS) $(LIB_MATH) $^ -o $@

# Runs the tests. "$(TEST_BINS)" requires the test executables to be up to date.
# The command is a simple shell script:
# "set -e" configures the shell to exit if any of the tests fail
# "for f in $(TEST_BINS); do ...; done" loops over the test executables,
#    assigning the variable f to each one
# "echo $$f" prints the test suite
# "$$f" runs the test; "$$" escapes the $ character,
#   and "$f" tells the shell to substitute the value of the variable f
# "echo" prints a newline after each test's output, for readability
test: $(TEST_BINS)
	set -e; for f in $(TEST_BINS); do echo $$f; $$f; echo; done

# Removes all compiled files.
clean:
	$(CLEAN_COMMAND)

# This special rule tells Make that "all", "clean", and "test" are rules
# that don't build a file.
.PHONY: all clean test
# Tells Make not to delete the .o files after the executable is built
.PRECIOUS: out/%.o
# Tells Make not to delete the wasm.o files after the executable is built
.PRECIOUS: out/%.wasm.o
