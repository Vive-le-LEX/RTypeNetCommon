cmake -S . -B test-build -DIS_TESTING:BOOL=TRUE

cmake --build test-build -j12

GTEST_COLOR=0 ctest --test-dir test-build --output-on-failure -j12
