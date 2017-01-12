# Expected location for this project is in <CSaruEnv>/src/<this-project>
# CSaruEnv is expected to have a makefiles directory for handy selection of what
# this directory will turn into.
#
# For CSaruEnv, see https://github.com/Akaito/codesaru-environ

# Special variable used to build ld (linker) -l and -L args.
# Space-separated list of CSaruEnv modules.
CSARU_DEPENDS = csaru-core-cpp
PROJ_VERSION = 0.2

include ../../makefiles/Makefile-staticlib

