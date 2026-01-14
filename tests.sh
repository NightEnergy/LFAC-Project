#!/bin/bash

TOTAL=0
PASSED=0

echo -e "ALL should fail"

for testfile in error_tests/*.txt; do
    ((TOTAL++))
    
    filename=$(basename "$testfile")
    echo -n "Testing $filename... "
    echo -e ""
    ./laglang "$testfile"
    
    EXIT_CODE=$?

    if [ $EXIT_CODE -ne 0 ]; then
        echo -e "\033[0;32m[PASS]\033[0m (Error caught)"
        echo -e ""
        ((PASSED++))
    else
        echo -e "\033[0;31m[FAIL]\033[0m (No error detected!)"
    fi
done

echo "-----------------------------------------"
echo "Summary: $PASSED / $TOTAL tests passed."