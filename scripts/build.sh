if [ $1 = "force" ]; then
    echo "Removing build directory..."
    rm -rf build
fi

cmake -S . -B build

cmake --build build -j12
