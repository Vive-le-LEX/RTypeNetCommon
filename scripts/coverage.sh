rm -rf ./coverage

echo "Fetching coverage data..."
lcov -c -d ./build/ -o ./build/main_coverage.info > /dev/null
echo "Filtering coverage data..."

lcov -r ./build/main_coverage.info '/usr/include/*' -o ./build/main_coverage.info > /dev/null
lcov -r ./build/main_coverage.info '*gtest*' -o ./build/main_coverage.info > /dev/null
lcov -e ./build/main_coverage.info '*src/*' -o ./build/main_coverage.info > /dev/null

echo "Generating HTML report..."
genhtml ./build/main_coverage.info --output-directory coverage > /dev/null

# Open the coverage report in the browser
xdg-open ./coverage/index.html
