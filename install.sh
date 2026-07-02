#!/bin/bash

set -e

if [ -t 1 ]; then
    RED='\033[0;31m'
    GREEN='\033[0;32m'
    YELLOW='\033[1;33m'
    BLUE='\033[0;34m'
    BOLD='\033[1m'
    NC='\033[0m'
else
    RED=''
    GREEN=''
    YELLOW=''
    BLUE=''
    BOLD=''
    NC=''
fi

echo -e "${BLUE}${BOLD}🚀 Starting installation of Persian Calendar Xfce Plugin...${NC}"
echo -e "${BLUE}--------------------------------------------------------${NC}"

if [ "$EUID" -eq 0 ]; then
    echo -e "${RED}❌ Error: Please do not run this script as root or using sudo directly.${NC}"
    echo "   Run it as a normal user: ./install.sh"
    echo "   The script will request sudo privileges automatically when installing."
    exit 1
fi

if [ ! -f "meson.build" ] || [ ! -d "panel-plugin" ]; then
    echo -e "${RED}❌ Error: meson.build not found in the current directory.${NC}"
    echo "   Please run this script from the project root directory."
    echo "   Example: cd /path/to/xfce-persian-calendar && ./install.sh"
    exit 1
fi

echo -e "📦 Checking and installing necessary dependencies..."
if [ -f /etc/os-release ]; then
    . /etc/os-release
    
    OS_ID=$(echo "$ID" | tr '[:upper:]' '[:lower:]')
    OS_LIKE=$(echo "$ID_LIKE" | tr '[:upper:]' '[:lower:]')
    
    case "$OS_ID" in
        arch|endeavouros|manjaro|artix)
            echo -e "🐧 Arch Linux based system detected (${GREEN}$OS_ID${NC})."
            sudo pacman -S --needed --noconfirm meson ninja pkgconf xfce4-panel libxfce4ui libxfce4util gtk3 glib2 xfce4-dev-tools
            ;;
        debian|ubuntu|linuxmint|pop)
            echo -e "🐧 Debian/Ubuntu based system detected (${GREEN}$OS_ID${NC})."
            sudo apt-get update
            sudo apt-get install -y meson ninja-build pkg-config libxfce4panel-2.0-dev libxfce4ui-2-dev libgtk-3-dev libglib2.0-dev xfce4-dev-tools
            ;;
        fedora)
            echo -e "🐧 Fedora based system detected (${GREEN}$OS_ID${NC})."
            sudo dnf install -y meson ninja-build pkgconf xfce4-panel-devel libxfce4ui-devel gtk3-devel glib2-devel xfce4-dev-tools
            ;;
        *)
            if [[ "$OS_LIKE" == *"arch"* ]]; then
                echo -e "🐧 Arch Linux based system detected via ID_LIKE (${GREEN}$OS_LIKE${NC})."
                sudo pacman -S --needed --noconfirm meson ninja pkgconf xfce4-panel libxfce4ui libxfce4util gtk3 glib2 xfce4-dev-tools
            elif [[ "$OS_LIKE" == *"debian"* || "$OS_LIKE" == *"ubuntu"* ]]; then
                echo -e "🐧 Debian/Ubuntu based system detected via ID_LIKE (${GREEN}$OS_LIKE${NC})."
                sudo apt-get update
                sudo apt-get install -y meson ninja-build pkg-config libxfce4panel-2.0-dev libxfce4ui-2-dev libgtk-3-dev libglib2.0-dev xfce4-dev-tools
            elif [[ "$OS_LIKE" == *"fedora"* || "$OS_LIKE" == *"rhel"* || "$OS_LIKE" == *"centos"* ]]; then
                echo -e "🐧 Fedora/RHEL based system detected via ID_LIKE (${GREEN}$OS_LIKE${NC})."
                sudo dnf install -y meson ninja-build pkgconf xfce4-panel-devel libxfce4ui-devel gtk3-devel glib2-devel xfce4-dev-tools
            else
                echo -e "${YELLOW}⚠️ Unsupported distribution for automatic dependency installation.${NC}"
                echo "Please make sure meson, ninja, pkg-config and xfce development headers are installed."
            fi
            ;;
    esac
else
    echo -e "${YELLOW}⚠️ Cannot determine OS. Please install dependencies manually if needed.${NC}"
fi

echo -e "${BLUE}--------------------------------------------------------${NC}"

MISSING_TOOLS=0
if ! command -v meson &> /dev/null; then
    echo -e "${RED}❌ Error: 'meson' is not installed or could not be found.${NC}"
    MISSING_TOOLS=1
fi

NINJA_CMD=""
if command -v ninja &> /dev/null; then
    NINJA_CMD="ninja"
elif command -v ninja-build &> /dev/null; then
    NINJA_CMD="ninja-build"
else
    echo -e "${RED}❌ Error: 'ninja' (or 'ninja-build') is not installed or could not be found.${NC}"
    MISSING_TOOLS=1
fi

if ! command -v pkg-config &> /dev/null; then
    echo -e "${RED}❌ Error: 'pkg-config' is not installed or could not be found.${NC}"
    MISSING_TOOLS=1
fi

if [ "$MISSING_TOOLS" -eq 1 ]; then
    echo -e "${RED}❌ Please install the missing build tools above and run this script again.${NC}"
    exit 1
fi

PREFIX="/usr"
if pkg-config --exists libxfce4panel-2.0; then
    DETECTED_PREFIX=$(pkg-config --variable=prefix libxfce4panel-2.0)
    if [ -n "$DETECTED_PREFIX" ]; then
        PREFIX="$DETECTED_PREFIX"
    fi
else
    echo -e "${YELLOW}⚠️ Warning: libxfce4panel-2.0 pkg-config package not found.${NC}"
    echo "   Build/development headers for xfce4-panel might not be correctly installed."
fi

echo -e "📍 Detected installation prefix: ${GREEN}$PREFIX${NC}"

if [ -d "build" ]; then
    echo -e "🧹 Cleaning previous build directory..."
    rm -rf build
fi

echo -e "🛠️ Configuring the project with Meson..."
if ! meson setup build --prefix="$PREFIX"; then
    echo -e "\n${RED}❌ Meson configuration failed.${NC}"
    echo "Please ensure all required development dependencies are installed."
    exit 1
fi

echo -e "⚙️ Compiling the project..."
if ! "$NINJA_CMD" -C build; then
    echo -e "\n${RED}❌ Compilation failed.${NC}"
    exit 1
fi

echo -e "💾 Installing the plugin (requires sudo privileges)..."
if ! sudo "$NINJA_CMD" -C build install; then
    echo -e "\n${RED}❌ Installation failed. Please check permissions.${NC}"
    exit 1
fi

echo -e "🔄 Restarting XFCE panel to apply changes..."
if command -v xfce4-panel &> /dev/null; then
    xfce4-panel -r
else
    echo -e "${YELLOW}⚠️ xfce4-panel command not found. Please restart your panel manually.${NC}"
fi

echo -e "${BLUE}--------------------------------------------------------${NC}"
echo -e "${GREEN}✅ Installation completed successfully!${NC}"
echo -e "🎉 You can now add 'Persian Calendar' to your XFCE panel."
echo -e "   Right-click panel -> Panel -> Add New Items... -> Search for 'Persian Calendar'"
