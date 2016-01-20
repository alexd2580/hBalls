# -----------------------------
# Make settings
# -----------------------------

MAKEFLAGS=

# -----------------------------
# Compiler settings
# -----------------------------

CC  = gcc
CXX = g++
LD  = g++
RM  = rm

# -----------------------------
# Project settings
# -----------------------------

PROJNAME         = RayTracer
PROJNAME_DEBUG   = $(PROJNAME)_debug
PROJNAME_RELEASE = $(PROJNAME)

# -----------------------------
# Compiler flags
# -----------------------------

WFLAGS       = -Wall -Wextra -Wdouble-promotion -Wformat=2 -Winit-self \
               -Wmissing-include-dirs -Wswitch-default -Wswitch-enum -Wunused-local-typedefs \
               -Wunused -Wuninitialized -Wsuggest-attribute=pure \
               -Wsuggest-attribute=const -Wsuggest-attribute=noreturn -Wfloat-equal \
               -Wundef -Wshadow -Wunsafe-loop-optimizations \
               -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings \
               -Wconversion -Wlogical-op \
               -Wmissing-field-initializers \
               -Wmissing-format-attribute -Wpacked -Wredundant-decls \
               -Wvector-operation-performance -Wdisabled-optimization \
               -Wstack-protector
CWFLAGS      = -Wc++-compat -Wbad-function-cast -Wstrict-prototypes \
               -Wnested-externs -Wunsuffixed-float-constants
UNUSED       = -Wdeclaration-after-statement -Wmissing-prototypes -Wstrict-overflow=5 -Winline
#-Wzero-as-null-pointer-constant -Wpadded
DEBUGFLAGS   = -g3 -O0 -pthread
RELEASEFLAGS = -g0 -O2 -pthread
CFLAGS       = -std=c14
CXXFLAGS     = -std=c++14

# -----------------------------
# Linker flags
# -----------------------------

LIBS = m OpenCL png SDL2

# -----------------------------
# Some automatic stuff
# -----------------------------

LFLAGS = $(foreach lib,$(LIBS),-l$(lib))

CSRCFILES := $(wildcard ./src/*.c) $(wildcard ./src/*/*.c)
COBJFILES := $(patsubst %.c,%.o,$(CSRCFILES))
CDEPFILES := $(patsubst %.c,%.d,$(CSRCFILES))
CXXSRCFILES := $(wildcard ./src/*.cpp) $(wildcard ./src/*/*.cpp)
CXXOBJFILES := $(patsubst %.cpp,%.o,$(CXXSRCFILES))
CXXDEPFILES := $(patsubst %.cpp,%.d,$(CXXSRCFILES))

SRCFILES = $(CSRCFILES) $(CXXSRCFILES)
OBJFILES = $(COBJFILES) $(CXXOBJFILES)
DEPFILES = $(CDEPFILES) $(CXXDEPFILES)

OBJFILES_DEBUG = $(patsubst %.o,debug/%.o,$(OBJFILES))
OBJFILES_RELEASE = $(patsubst %.o,release/%.o,$(OBJFILES))

DEPFILES_DEBUG = $(patsubst %.d,debug/%.d,$(DEPFILES))
DEPFILES_RELEASE = $(patsubst %.d,release/%.d,$(DEPFILES))

# -----------------------------
# Make targets
# -----------------------------

.PHONY: all debug relase clean

all: $(PROJNAME_DEBUG) $(PROJNAME_RELEASE) Makefile
	@echo "  [ Finished ]"

debug: $(PROJNAME_DEBUG) Makefile
	@echo "  [ Done ]"

release: $(PROJNAME_RELEASE) Makefile
	@echo "  [ Done ]"

rebuild:
	make clean && \
	make all

cleanall: clean cleangedit

-include $(DEPFILES_DEBUG)
-include $(DEPFILES_RELEASE)

$(PROJNAME_DEBUG): $(OBJFILES_DEBUG) $(SRCFILES) Makefile
	@echo "  [ Linking $@ ]" && \
	$(LD) $(OBJFILES_DEBUG) -o $@ $(LFLAGS)

$(PROJNAME_RELEASE): $(OBJFILES_RELEASE) $(SRCFILES) Makefile
	@echo "  [ Linking $@ ]" && \
	$(LD) $(OBJFILES_RELEASE) -o $@ $(LFLAGS)

debug/%.o: %.c Makefile
	@echo "  [ Compiling $< ]" && \
	mkdir debug/$(dir $<) -p && \
	$(CC) $(CFLAGS) $(WFLAGS) $(CWFLAGS) $(DEBUGFLAGS) -MMD -MP -c $< -o $@

debug/%.o: %.cpp Makefile
	@echo "  [ Compiling $< ]" && \
	mkdir debug/$(dir $<) -p && \
	$(CXX) $(CXXFLAGS) $(WFLAGS) $(DEBUGFLAGS) -MMD -MP -c $< -o $@

release/%.o: %.c Makefile
	@echo "  [ Compiling $< ]" && \
	mkdir release/$(dir $<) -p && \
	$(CC) $(CFLAGS) $(WFLAGS) $(CWFLAGS) $(RELEASEFLAGS) -MMD -MP -c $< -o $@

release/%.o: %.cpp Makefile
	@echo "  [ Compiling $< ]" && \
	mkdir release/$(dir $<) -p && \
	$(CXX) $(CXXFLAGS) $(WFLAGS) $(RELEASEFLAGS) -MMD -MP -c $< -o $@

clean:
	-@$(RM) -f $(wildcard $(OBJFILES_DEBUG) $(OBJFILES_RELEASE) $(DEPFILES_DEBUG) $(DEPFILES_RELEASE) $(PROJNAME_DEBUG) $(PROJNAME_RELEASE)) && \
	$(RM) -rf debug release && \
	echo "  [ Clean done ]"
