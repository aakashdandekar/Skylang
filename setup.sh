#!/usr/bin/env bash
set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

info()    { echo -e "${BLUE}[INFO]${NC}    $1"; }
success() { echo -e "${GREEN}[OK]${NC}      $1"; }
warn()    { echo -e "${YELLOW}[WARN]${NC}    $1"; }
error()   { echo -e "${RED}[ERROR]${NC}   $1"; }
step()    { echo -e "\n${CYAN}${BOLD}── $1 ──${NC}"; }

echo -e "${CYAN}${BOLD}"
echo "  ┌─────────────────────────────────────────┐"
echo "  │               Skylang Setup             │"
echo "  └─────────────────────────────────────────┘"
echo -e "${NC}"


SKYLANG_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
info "Repository Directory: ${SKYLANG_DIR}"

OS="$(uname -s)"
ARCH="$(uname -m)"
DISTRO="unknown"
PKG_MGR=""
INSTALL_CMD=""

case "$OS" in
    Linux)
        if [ -f /etc/os-release ]; then
            . /etc/os-release
            DISTRO="$ID"
            info "Operating System: Linux ($PRETTY_NAME, $ARCH)"
        else
            info "Operating System: Linux ($ARCH)"
        fi

        if command -v apt-get &>/dev/null; then
            PKG_MGR="apt"
            INSTALL_CMD="sudo apt-get install -y"
            PKGS_BUILD="build-essential gcc g++ make pkg-config libgc-dev"
            PKGS_PYTHON="python3 python3-dev"
            PKGS_NODE="nodejs npm"
            PKGS_GO="golang-go"
            PKGS_RUST="rustc cargo"
            PKGS_JAVA="default-jdk"
        elif command -v pacman &>/dev/null; then
            PKG_MGR="pacman"
            INSTALL_CMD="sudo pacman -S --noconfirm --needed"
            PKGS_BUILD="base-devel gcc make pkgconf gc"
            PKGS_PYTHON="python"
            PKGS_NODE="nodejs npm"
            PKGS_GO="go"
            PKGS_RUST="rust"
            PKGS_JAVA="jdk-openjdk"
        elif command -v dnf &>/dev/null; then
            PKG_MGR="dnf"
            INSTALL_CMD="sudo dnf install -y"
            PKGS_BUILD="gcc gcc-c++ make pkgconf gc-devel"
            PKGS_PYTHON="python3 python3-devel"
            PKGS_NODE="nodejs npm"
            PKGS_GO="golang"
            PKGS_RUST="rust cargo"
            PKGS_JAVA="java-latest-openjdk-devel"
        elif command -v zypper &>/dev/null; then
            PKG_MGR="zypper"
            INSTALL_CMD="sudo zypper install -y"
            PKGS_BUILD="gcc gcc-c++ make pkg-config gc-devel"
            PKGS_PYTHON="python3 python3-devel"
            PKGS_NODE="nodejs npm"
            PKGS_GO="go"
            PKGS_RUST="rust cargo"
            PKGS_JAVA="java-openjdk-devel"
        elif command -v apk &>/dev/null; then
            PKG_MGR="apk"
            INSTALL_CMD="sudo apk add --no-cache"
            PKGS_BUILD="build-base gcc g++ make pkgconf gc-dev"
            PKGS_PYTHON="python3 python3-dev"
            PKGS_NODE="nodejs npm"
            PKGS_GO="go"
            PKGS_RUST="rust cargo"
            PKGS_JAVA="openjdk17-jdk"
        else
            warn "No supported Linux package manager detected."
        fi
        ;;
    Darwin)
        info "Operating System: macOS ($ARCH)"
        if command -v brew &>/dev/null; then
            PKG_MGR="brew"
            INSTALL_CMD="brew install"
            PKGS_BUILD="gcc make pkg-config bdw-gc"
            PKGS_PYTHON="python3"
            PKGS_NODE="node"
            PKGS_GO="go"
            PKGS_RUST="rust"
            PKGS_JAVA="openjdk"
        else
            warn "Homebrew not found. Please install Homebrew from https://brew.sh"
        fi
        ;;
    MINGW*|MSYS*|CYGWIN*)
        info "Operating System: Windows ($OS, $ARCH)"
        if command -v pacman &>/dev/null; then
            PKG_MGR="pacman"
            INSTALL_CMD="pacman -S --noconfirm --needed"
            PKGS_BUILD="mingw-w64-x86_64-gcc mingw-w64-x86_64-make mingw-w64-x86_64-pkg-config mingw-w64-x86_64-gc"
            PKGS_PYTHON="mingw-w64-x86_64-python"
            PKGS_NODE="mingw-w64-x86_64-nodejs"
            PKGS_GO="mingw-w64-x86_64-go"
            PKGS_RUST="mingw-w64-x86_64-rust"
            PKGS_JAVA="mingw-w64-x86_64-openjdk"
        fi
        ;;
    *)
        error "Unsupported operating system: $OS"
        exit 1
        ;;
esac

INSTALL_LIST=()

if ! command -v gcc &>/dev/null || ! command -v make &>/dev/null || ! pkg-config --exists bdw-gc 2>/dev/null; then
    INSTALL_LIST+=($PKGS_BUILD)
fi

if ! command -v python3 &>/dev/null; then
    INSTALL_LIST+=($PKGS_PYTHON)
fi

if ! command -v node &>/dev/null || ! command -v npm &>/dev/null; then
    INSTALL_LIST+=($PKGS_NODE)
fi

if ! command -v go &>/dev/null; then
    INSTALL_LIST+=($PKGS_GO)
fi

if ! command -v rustc &>/dev/null || ! command -v cargo &>/dev/null; then
    INSTALL_LIST+=($PKGS_RUST)
fi

if ! command -v java &>/dev/null || ! command -v javac &>/dev/null; then
    INSTALL_LIST+=($PKGS_JAVA)
fi

if [ ${#INSTALL_LIST[@]} -gt 0 ]; then
    info "Packages to install: ${INSTALL_LIST[*]}"
    if [ -n "$PKG_MGR" ]; then
        if [ "$PKG_MGR" = "apt" ]; then
            sudo apt-get update -qq || true
        fi
        $INSTALL_CMD "${INSTALL_LIST[@]}" || warn "Some packages could not be installed automatically."
    else
        warn "Could not automatically install packages. Please ensure gcc, python3, nodejs, go, rustc/cargo, and java are installed."
    fi
else
    success "All language runtimes and build dependencies are satisfied"
fi

if command -v gcc &>/dev/null; then success "C/C++ Compiler: $(gcc --version | head -n1)"; fi
if command -v python3 &>/dev/null; then success "Python Runtime: $(python3 --version)"; fi
if command -v node &>/dev/null; then success "Node.js Runtime: $(node --version)"; fi
if command -v go &>/dev/null; then success "Go Runtime: $(go version)"; fi
if command -v rustc &>/dev/null; then success "Rust Runtime: $(rustc --version)"; fi
if command -v javac &>/dev/null; then success "Java Compiler: $(javac --version 2>&1 | head -n1)"; fi

info "Building bin/skylang, bin/sky, and lib/libskylang_rt.a..."
make -C "$SKYLANG_DIR" clean all

LOCAL_BIN="$HOME/.local/bin"
mkdir -p "$LOCAL_BIN"
ln -sf "$SKYLANG_DIR/bin/skylang" "$LOCAL_BIN/skylang"
ln -sf "$SKYLANG_DIR/bin/skylang" "$LOCAL_BIN/sky"
success "Installed binaries to ${LOCAL_BIN}"

NEEDS_PATH_UPDATE=false
if ! echo "$PATH" | tr ':' '\n' | grep -qx "$LOCAL_BIN"; then
    NEEDS_PATH_UPDATE=true
    PATH_EXPORT='export PATH="$HOME/.local/bin:$PATH"'
    CURRENT_SHELL="$(basename "$SHELL" 2>/dev/null || echo "bash")"
    
    SHELL_CONFIGS=()
    if [ "$CURRENT_SHELL" = "zsh" ] && [ -f "$HOME/.zshrc" ]; then
        SHELL_CONFIGS+=("$HOME/.zshrc")
    elif [ "$CURRENT_SHELL" = "bash" ]; then
        if [ -f "$HOME/.bashrc" ]; then
            SHELL_CONFIGS+=("$HOME/.bashrc")
        elif [ -f "$HOME/.bash_profile" ]; then
            SHELL_CONFIGS+=("$HOME/.bash_profile")
        else
            SHELL_CONFIGS+=("$HOME/.bashrc")
        fi
    fi

    [ -f "$HOME/.zshrc" ] && [[ ! " ${SHELL_CONFIGS[*]} " =~ " $HOME/.zshrc " ]] && SHELL_CONFIGS+=("$HOME/.zshrc")
    [ -f "$HOME/.bashrc" ] && [[ ! " ${SHELL_CONFIGS[*]} " =~ " $HOME/.bashrc " ]] && SHELL_CONFIGS+=("$HOME/.bashrc")

    for config_file in "${SHELL_CONFIGS[@]}"; do
        if ! grep -qF '.local/bin' "$config_file" 2>/dev/null; then
            echo "" >> "$config_file"
            echo "$PATH_EXPORT" >> "$config_file"
            success "Added PATH to $config_file"
        fi
    done

    if [ "$CURRENT_SHELL" = "fish" ]; then
        FISH_CONFIG="$HOME/.config/fish/config.fish"
        mkdir -p "$HOME/.config/fish"
        if ! grep -qF '.local/bin' "$FISH_CONFIG" 2>/dev/null; then
            echo "" >> "$FISH_CONFIG"
            echo 'set -gx PATH $HOME/.local/bin $PATH' >> "$FISH_CONFIG"
            success "Added PATH to $FISH_CONFIG"
        fi
    fi
    export PATH="$LOCAL_BIN:$PATH"
fi

make -C "$SKYLANG_DIR" vscode 2>/dev/null || true

step "Step 6/6: Verifying Installation with Test Suite"

make -C "$SKYLANG_DIR" test

echo ""
echo -e "${GREEN}${BOLD}  ✓ Skylang has been successfully installed!${NC}"
echo ""
echo -e "  ${BOLD}Usage:${NC}"
echo -e "    ${CYAN}sky run ${NC}<file.sky>           Compile and run a Skylang program"
echo -e "    ${CYAN}sky build ${NC}<file.sky> -o app   Compile to a standalone binary"
echo ""
echo -e "  ${BOLD}Multi-Language Interop Available:${NC}"
echo -e "    - Python 3     (import python)"
echo -e "    - JavaScript   (import js)"
echo -e "    - C++ JIT      (import cpp)"
echo -e "    - Java JVM     (import java)"
echo -e "    - Golang JIT   (import go)"
echo -e "    - Rust cdylib  (import rust)"
echo ""

if [ "$NEEDS_PATH_UPDATE" = true ]; then
    echo -e "  ${YELLOW}${BOLD}Note:${NC} Restart your terminal or run ${CYAN}source ~/.bashrc${NC} to use 'sky' anywhere."
    echo ""
fi
