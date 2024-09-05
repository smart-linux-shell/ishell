#!/bin/bash

PROJECT_DIR="tui-tux"
DEPENDENCIES=(
    "libncurses5-dev"
    "libncursesw5-dev"
    "libreadline-dev"
    "libcurl4-openssl-dev"
)

JSON_DIR="nlohmann"

is_package_installed() {
    dpkg -s "$1" &> /dev/null
    return $?
}

install_package() {
    echo "Installing $1..."
    sudo apt-get update -qq
    sudo apt-get install -y "$1"
}

if ! [ -f "$JSON_DIR/json.hpp" ]; then
    echo "Downloading nlohmann/json.hpp..."
    mkdir -p "$JSON_DIR"
    wget https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp -O "$JSON_DIR/json.hpp"
fi

if [ -d "$PROJECT_DIR" ]; then
    cd "$PROJECT_DIR" || exit
else
    echo "Error: Directory '$PROJECT_DIR' does not exist."
    exit 1
fi

echo "Checking dependencies..."
for pkg in "${DEPENDENCIES[@]}"; do
    if is_package_installed "$pkg"; then
        echo "$pkg is already installed."
    else
        echo "$pkg is not installed."
        install_package "$pkg"
        if ! is_package_installed "$pkg"; then
            echo "Error: Failed to install $pkg."
            exit 1
        fi
    fi
done

echo "Running Makefile..."
make clean
if make; then
    echo "Build successful."
else
    echo "Error: Build failed."
    exit 1
fi

if [ -x "./main" ]; then
    echo "Executable 'main' found."
else
    echo "Error: Executable 'main' not found. Build might have failed."
    exit 1
fi

# Run the main executable
echo "Running the program..."
./main
