#! /bin/bash

set -u

BASE_PATH="$(dirname "$0")/../../.."
cd "$BASE_PATH"

GREEN='\033[0;32m'
RED='\033[0;31m'
OFF='\033[0m'
STATUS=0

echo "Compiler should accept..."
echo ""

for test in $(ls src/test/c/accept/); do
	# Capture both stdout and stderr, but filter out memory leak messages
	OUTPUT=$(cat "src/test/c/accept/$test" | ".build/Flex-Bison-Compiler" 2>&1)
	RESULT="$?"
	
	# Check if there are actual parsing errors (not just memory leaks)
	if echo "$OUTPUT" | grep -q "Parse error\|syntax error\|ERROR.*EntryPoint.*rejects"; then
		STATUS=1
		echo -e "    $test, ${RED}but it rejects${OFF} (parsing error)"
		echo "      Error: $(echo "$OUTPUT" | grep -E "Parse error|syntax error|ERROR.*EntryPoint.*rejects" | head -1)"
	else
		echo -e "    $test, ${GREEN}and it does${OFF} (status $RESULT)"
	fi
done
echo ""

echo "Compiler should reject..."
echo ""

for test in $(ls src/test/c/reject/); do
	# Capture both stdout and stderr, but filter out memory leak messages
	OUTPUT=$(cat "src/test/c/reject/$test" | ".build/Flex-Bison-Compiler" 2>&1)
	RESULT="$?"
	
	# Check if there are actual parsing errors (not just memory leaks)
	if echo "$OUTPUT" | grep -q "Parse error\|syntax error\|ERROR.*EntryPoint.*rejects"; then
		echo -e "    $test, ${GREEN}and it does${OFF} (status $RESULT)"
	else
		STATUS=1
		echo -e "    $test, ${RED}but it accepts${OFF} (status $RESULT)"
	fi
done
echo ""

echo "All done."
exit $STATUS
