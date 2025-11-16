#!/usr/bin/env bash

# Variables
VERSION="2.38.0"
USE_EMSDK=false
CMAKE_EXTRA_ARGS=()

parse_arguments() {
    TEMP=$(getopt -o '' \
                  --long version:,use-emsdk,help \
                  -n 'setup_angelscript.sh' -- "$@")

    if [ $? != 0 ] ; then echo "Terminating..." >&2 ; exit 1 ; fi

    eval set -- "$TEMP"

    while true; do
        case "$1" in
            --version)
                VERSION="$2"
                shift 2
                ;;
            --use-emsdk)
                USE_EMSDK=true
                shift
                ;;
            --)
                shift
                # Remained arguments are extra args for CMake
                CMAKE_EXTRA_ARGS=("$@")
                break
                ;;
            *)
                echo "Internal error!"
                exit 1
                ;;
        esac
    done
}

clone_angelscript() {
    echo Cloning AS $VERSION
    if [ $VERSION == "latest" ]; then
        git clone --depth 1 https://github.com/anjo76/angelscript.git
        cd angelscript
        git log -1 # Print the latest commit info
        cd ..
    elif [ $VERSION == "2.37.0" ]; then
        git clone https://github.com/anjo76/angelscript.git
        # Official GitHub repo only has tag for version 2.38+
        # The --branch is for v2.37.0 commit
        cd angelscript
        git reset --hard 0beef466371f36cae4e39d84505bb5a19804ed08
        cd ..
    else
        git clone --depth 1 --branch v$VERSION https://github.com/anjo76/angelscript.git
    fi
}

parse_arguments "$@"

cd /tmp
clone_angelscript

cd angelscript
cd sdk/angelscript/projects/cmake
if [ "$USE_EMSDK" = true ]; then
    emcmake cmake -GNinja -DCMAKE_BUILD_TYPE=Release -S . -B build "${CMAKE_EXTRA_ARGS[@]}"
else
    cmake -GNinja -DCMAKE_BUILD_TYPE=Release -S . -B build "${CMAKE_EXTRA_ARGS[@]}"
fi

cmake --build build
sudo cmake --install build
