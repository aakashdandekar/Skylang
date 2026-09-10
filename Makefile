CC ?= gcc
AR ?= ar

# Detect OS
ifeq ($(OS),Windows_NT)
    TARGET_OS := Windows
    EXE := .exe
    LIB_EXT := .a
    SO_EXT := .dll
    PY_CFLAGS := $(shell pkg-config --cflags python3-embed 2>/dev/null || pkg-config --cflags python3 2>/dev/null || echo "")
    PY_LIBS := $(shell pkg-config --libs python3-embed 2>/dev/null || pkg-config --libs python3 2>/dev/null || echo "-lpython3")
    GC_CFLAGS := $(shell pkg-config --cflags bdw-gc 2>/dev/null || echo "")
    GC_LIBS := $(shell pkg-config --libs bdw-gc 2>/dev/null || echo "-lgc")
    PLATFORM_LDFLAGS := -lm
    RM := rm -rf
    MKDIR := mkdir -p
    CP := cp -f
else
    UNAME_S := $(shell uname -s 2>/dev/null || echo Linux)
    ifeq ($(UNAME_S),Darwin)
        TARGET_OS := macOS
        EXE :=
        LIB_EXT := .a
        SO_EXT := .dylib
        BREW_PREFIX := $(shell brew --prefix 2>/dev/null || echo /usr/local)
        PY_CFLAGS := $(shell pkg-config --cflags python3-embed 2>/dev/null || pkg-config --cflags python3 2>/dev/null || echo "-I$(BREW_PREFIX)/include")
        PY_LIBS := $(shell pkg-config --libs python3-embed 2>/dev/null || pkg-config --libs python3 2>/dev/null || echo "-L$(BREW_PREFIX)/lib -lpython3")
        GC_CFLAGS := $(shell pkg-config --cflags bdw-gc 2>/dev/null || echo "-I$(BREW_PREFIX)/include")
        GC_LIBS := $(shell pkg-config --libs bdw-gc 2>/dev/null || echo "-L$(BREW_PREFIX)/lib -lgc")
        PLATFORM_LDFLAGS := -lm -ldl -pthread
    else
        TARGET_OS := Linux
        EXE :=
        LIB_EXT := .a
        SO_EXT := .so
        PY_CFLAGS := $(shell pkg-config --cflags python3-embed 2>/dev/null || pkg-config --cflags python3 2>/dev/null || echo "-I/usr/include/python3.14")
        PY_LIBS := $(shell pkg-config --libs python3-embed 2>/dev/null || pkg-config --libs python3 2>/dev/null || echo "-lpython3.14")
        GC_CFLAGS := $(shell pkg-config --cflags bdw-gc 2>/dev/null || echo "")
        GC_LIBS := $(shell pkg-config --libs bdw-gc 2>/dev/null || echo "-lgc")
        PLATFORM_LDFLAGS := -lm -ldl -pthread
    endif
    RM := rm -rf
    MKDIR := mkdir -p
    CP := cp -f
endif

CFLAGS = -Wall -Wextra -O2 -Iinclude -DGC_THREADS $(GC_CFLAGS) $(PY_CFLAGS)
LDFLAGS = $(GC_LIBS) $(PY_LIBS) $(PLATFORM_LDFLAGS)

CLI_SRC = src/main.c src/codegen.c
CLI_OBJ = $(CLI_SRC:.c=.o)
BIN = bin/skylang$(EXE)
BIN_SKY = bin/sky$(EXE)

RT_SRC = src/runtime.c src/sky_stdlib.c src/lexer.c src/parser.c src/compiler.c src/vm.c src/sky_python.c src/sky_js.c src/sky_cpp.c src/sky_java.c src/sky_go.c src/sky_rust.c
RT_OBJ = $(RT_SRC:.c=.o)
RT_LIB = lib/libskylang_rt$(LIB_EXT)

all: dirs $(BIN) $(BIN_SKY) $(RT_LIB)

dirs:
	@$(MKDIR) bin lib

$(BIN): $(CLI_OBJ) $(RT_OBJ)
	$(CC) $(CLI_OBJ) $(RT_OBJ) $(LDFLAGS) -o $(BIN)

$(BIN_SKY): $(BIN)
ifeq ($(TARGET_OS),Windows)
	@$(CP) $(BIN) $(BIN_SKY)
else
	@ln -sf skylang bin/sky
endif

$(RT_LIB): $(RT_OBJ)
	$(AR) rcs $(RT_LIB) $(RT_OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(CLI_OBJ) $(RT_OBJ) bin lib /tmp/skylang_* /tmp/sky_* /tmp/SkyJavaRunner_*

test: all
	@echo "Running test suite on $(TARGET_OS)..."
	./$(BIN) run tests/test_all.sky

win-installer: dirs $(BIN) $(BIN_SKY) $(RT_LIB)
ifeq ($(TARGET_OS),Windows)
	$(CC) -Wall -Wextra -O2 tools/installer/installer_win.c -ladvapi32 -lshell32 -luser32 -lshlwapi -o bin/skylang-installer.exe
	@echo "Windows installer successfully generated: bin/skylang-installer.exe"
else
	@echo "Note: Building Windows installer requires MinGW-w64 or Windows environment."
endif

vscode:
	@mkdir -p ~/.vscode/extensions
	@rm -rf ~/.vscode/extensions/skylang* ~/.vscode/extensions/*skylang* ~/.vscode/extensions/.obsolete
	@ln -sf $(CURDIR)/editors/vscode ~/.vscode/extensions/skylang.skylang-1.0.0
	@python3 -c "import json, os; p = os.path.expanduser('~/.config/Code/User/settings.json'); os.makedirs(os.path.dirname(p), exist_ok=True); data = json.load(open(p)) if os.path.exists(p) else {}; data['workbench.iconTheme'] = 'skylang-icons'; data.setdefault('files.associations', {})['*.sky'] = 'skylang'; data['files.associations']['*.skylang'] = 'skylang'; json.dump(data, open(p, 'w'), indent=4)" 2>/dev/null || true
	@echo "Skylang VS Code extension & default icon theme installed!"

manual:
	@python3 tools/generate_pdf_manual.py

.PHONY: all dirs clean test win-installer vscode manual
