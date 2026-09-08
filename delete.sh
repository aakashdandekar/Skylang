#!/usr/bin/env bash
set -e

SKYLANG_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "Uninstalling Skylang from: ${SKYLANG_DIR}"

LOCAL_BIN="$HOME/.local/bin"
if [ -L "$LOCAL_BIN/skylang" ] || [ -f "$LOCAL_BIN/skylang" ]; then
    rm -f "$LOCAL_BIN/skylang"
    echo "Removed ${LOCAL_BIN}/skylang"
fi

if [ -L "$LOCAL_BIN/sky" ] || [ -f "$LOCAL_BIN/sky" ]; then
    rm -f "$LOCAL_BIN/sky"
    echo "Removed ${LOCAL_BIN}/sky"
fi

echo "Cleaning build artifacts and binaries..."
make -C "$SKYLANG_DIR" clean 2>/dev/null || true
rm -rf "$SKYLANG_DIR/bin" "$SKYLANG_DIR/lib" "$SKYLANG_DIR/src/"*.o

echo "Removing VS Code extension..."
rm -rf "$HOME/.vscode/extensions/skylang"* "$HOME/.vscode/extensions/"*skylang* 2>/dev/null || true

echo "Cleaning temporary execution caches..."
rm -rf /tmp/skylang_* /tmp/skylang_*.c /tmp/skylang_*.bin /tmp/sky_* /tmp/SkyJavaRunner_* 2>/dev/null || true

echo ""
echo "  Skylang has been successfully uninstalled from your system."
echo ""
