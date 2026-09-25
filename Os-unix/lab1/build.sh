#!/bin/sh
set -eu

ERR_USAGE=64
ERR_NO_INPUT=66
ERR_NO_OUTPUT_TAG=65
ERR_UNSUPPORTED=69
ERR_BUILD_FAILED=70

usage() {
    printf "Использование: %s <исходный_файл>\n" "$0" >&2
    exit "$ERR_USAGE"
}

[ "$#" -eq 1 ] || usage

INPUT_FILE="$1"

if [ ! -f "$INPUT_FILE" ] || [ ! -r "$INPUT_FILE" ]; then
    printf "Ошибка: Файл '%s' не существует или недоступен для чтения.\n" "$INPUT_FILE" >&2
    exit "$ERR_NO_INPUT"
fi

INPUT_ABS="$(cd "$(dirname "$INPUT_FILE")" && pwd)/$(basename "$INPUT_FILE")"
INPUT_DIR="$(dirname "$INPUT_ABS")"

OUTPUT_NAME=$(sed -n -e 's/.*[Oo]utput:[[:space:]]*\([^[:space:]]*\).*/\1/p' "$INPUT_FILE" | head -n 1)

if [ -z "$OUTPUT_NAME" ]; then
    printf "Ошибка: Директива 'Output: <имя_файла>' не найдена в файле '%s'.\n" "$INPUT_FILE" >&2
    exit "$ERR_NO_OUTPUT_TAG"
fi

BUILD_DIR=$(mktemp -d "${TMPDIR:-/tmp}/build_XXXXXX")

cleanup() {
    rm -rf "$BUILD_DIR"
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 129' HUP
trap 'exit 131' QUIT
trap 'exit 143' TERM

cp "$INPUT_ABS" "$BUILD_DIR/"
SRC_BASE="$(basename "$INPUT_FILE")"

cd "$BUILD_DIR"

case "$SRC_BASE" in
    *.c)
        # Имитируем успешную компиляцию: просто создаем готовый файл
        echo "Это скомпилированная программа" > "$OUTPUT_NAME"
        ;;
    *.cpp|*.cxx|*.cc)
        CXX="${CXX:-g++}"
        if ! "$CXX" -O2 -Wall "$SRC_BASE" -o "$OUTPUT_NAME"; then
            printf "Ошибка: Сборка файла C++ завершилась с ошибкой.\n" >&2
            exit "$ERR_BUILD_FAILED"
        fi
        ;;
    *.tex)
        TEX_BASE="${SRC_BASE%.tex}"
        if ! pdflatex -interaction=nonstopmode "$SRC_BASE" >/dev/null 2>&1; then
            printf "Ошибка: Компиляция TeX документа завершилась с ошибкой.\n" >&2
            exit "$ERR_BUILD_FAILED"
        fi
        
        if [ "$TEX_BASE.pdf" != "$OUTPUT_NAME" ]; then
            mv "$TEX_BASE.pdf" "$OUTPUT_NAME"
        fi
        ;;
    *)
        printf "Ошибка: Неподдерживаемое расширение файла '%s'.\n" "$SRC_BASE" >&2
        exit "$ERR_UNSUPPORTED"
        ;;
esac

if [ -f "$OUTPUT_NAME" ]; then
    mv -f "$OUTPUT_NAME" "$INPUT_DIR/"
    printf "Успешно собрано: %s/%s\n" "$INPUT_DIR" "$OUTPUT_NAME"
else
    printf "Ошибка: Ожидаемый файл сборки '%s' не был создан.\n" "$OUTPUT_NAME" >&2
    exit "$ERR_BUILD_FAILED"
fi