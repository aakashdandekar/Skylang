#!/usr/bin/env bash
set -e

SKYLANG_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "Uninstalling Skylang from: ${SKYLANG_DIR}"

LOCAL_BIN="$HOME/.local/bin"
rm -f "$LOCAL_BIN/skylang" "$LOCAL_BIN/sky" "$LOCAL_BIN/skylang.exe" "$LOCAL_BIN/sky.exe" "$LOCAL_BIN/sky.cmd" "$LOCAL_BIN/skylang.cmd" 2>/dev/null || true
echo "Removed binaries from ${LOCAL_BIN}"

echo "Cleaning build artifacts and binaries..."
make -C "$SKYLANG_DIR" clean 2>/dev/null || true
rm -rf "$SKYLANG_DIR/bin" "$SKYLANG_DIR/lib" "$SKYLANG_DIR/src/"*.o

echo "Removing VS Code extension..."
rm -rf "$HOME/.vscode/extensions/skylang"* "$HOME/.vscode/extensions/"*skylang* 2>/dev/null || true

echo "Cleaning temporary execution caches..."
rm -rf /tmp/skylang_* /tmp/sky_* /tmp/SkyJavaRunner_* 2>/dev/null || true
if [ -n "$TEMP" ] && [ -d "$TEMP" ]; then
    rm -rf "$TEMP"/skylang_* "$TEMP"/sky_* "$TEMP"/SkyJavaRunner_* 2>/dev/null || true
fi

echo ""
echo "  Skylang has been successfully uninstalled from your system."
echo ""
