SCRIPT_DIR=$(dirname "$0")
RUN_BIN=$SCRIPT_DIR/example.exe
clang -g -Wall -Wextra $SCRIPT_DIR/example.c -o $RUN_BIN -lpthread && $RUN_BIN $@
