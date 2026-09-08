CC = gcc
PY_CFLAGS = $(shell pkg-config --cflags python3-embed 2>/dev/null || pkg-config --cflags python3 2>/dev/null || echo "-I/usr/include/python3.14")
PY_LIBS = $(shell pkg-config --libs python3-embed 2>/dev/null || pkg-config --libs python3 2>/dev/null || echo "-lpython3.14")
GC_CFLAGS = $(shell pkg-config --cflags bdw-gc 2>/dev/null || echo "")
GC_LIBS = $(shell pkg-config --libs bdw-gc 2>/dev/null || echo "-lgc")

CFLAGS = -Wall -Wextra -O2 -Iinclude $(GC_CFLAGS) $(PY_CFLAGS)
LDFLAGS = $(GC_LIBS) $(PY_LIBS) -lm -ldl

CLI_SRC = src/main.c src/codegen.c
CLI_OBJ = $(CLI_SRC:.c=.o)
BIN = bin/skylang

RT_SRC = src/runtime.c src/sky_stdlib.c src/lexer.c src/parser.c src/compiler.c src/vm.c src/sky_python.c src/sky_js.c src/sky_cpp.c src/sky_java.c src/sky_go.c src/sky_rust.c
RT_OBJ = $(RT_SRC:.c=.o)
RT_LIB = lib/libskylang_rt.a

all: dirs $(BIN) $(RT_LIB)

dirs:
	@mkdir -p bin lib

$(BIN): $(CLI_OBJ) $(RT_OBJ)
	$(CC) $(CLI_OBJ) $(RT_OBJ) $(LDFLAGS) -o $(BIN)
	@ln -sf skylang bin/sky

$(RT_LIB): $(RT_OBJ)
	ar rcs $(RT_LIB) $(RT_OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(CLI_OBJ) $(RT_OBJ) bin lib /tmp/skylang_*.c /tmp/skylang_*.bin /tmp/sky_*.cpp /tmp/sky_*.so /tmp/sky_*.go /tmp/sky_*.rs /tmp/sky_*.bin /tmp/SkyJavaRunner_*

test: all
	@echo "Running test suite..."
	./bin/skylang run tests/test_all.sky

vscode:
	@mkdir -p ~/.vscode/extensions
	@rm -rf ~/.vscode/extensions/skylang* ~/.vscode/extensions/*skylang* ~/.vscode/extensions/.obsolete
	@ln -sf $(CURDIR)/editors/vscode ~/.vscode/extensions/skylang.skylang-1.0.0
	@python3 -c "import json, os; p = os.path.expanduser('~/.config/Code/User/settings.json'); os.makedirs(os.path.dirname(p), exist_ok=True); data = json.load(open(p)) if os.path.exists(p) else {}; data['workbench.iconTheme'] = 'skylang-icons'; data.setdefault('files.associations', {})['*.sky'] = 'skylang'; data['files.associations']['*.skylang'] = 'skylang'; json.dump(data, open(p, 'w'), indent=4)" 2>/dev/null || true
	@echo "Skylang VS Code extension & default icon theme installed!"

.PHONY: all dirs clean test vscode
