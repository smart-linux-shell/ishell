#!/bin/bash
# to be refactored

PROJECT_DIR="tui-tux"
DEPENDENCIES=(
    "libncurses5-dev"
    "libncursesw5-dev"
    "libreadline-dev"
    "libcurl4-openssl-dev"
)

JSON_DIR="tui-tux/nlohmann"
RUN_TESTS=false

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

for arg in "$@"; do
    case $arg in
        -t|--test)
        RUN_TESTS=true
        shift
        ;;
    esac
done

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

# Check if the user wants to run tests
if [ "$RUN_TESTS" = true ]; then
    echo "Running tests..."
    if make run_test; then
        echo "Tests ran successfully."
    else
        echo "Error: Tests failed."
        exit 1
    fi
else
    if [ -x "./ishell" ]; then
        echo "Executable 'ishell' found."
    else
        echo "Error: Executable 'ishell' not found. Build might have failed."
        exit 1
    fi

    echo "Running the program..."
    ./ishell
fi
