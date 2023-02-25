SCRIPT_DIR=$(dirname "$0")
RUN_BIN=$SCRIPT_DIR/example.exe
# TODO(khvorov) Remove -lm
clang -g -Wall -Wextra $SCRIPT_DIR/example.c -o $RUN_BIN -lX11 -lm && $RUN_BIN $@
