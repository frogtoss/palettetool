#!/usr/bin/env bash

set -euo pipefail

if (( $# > 1 )); then
    echo "usage: $0 [theme-name]" >&2
    exit 2
fi

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
output_directory="$PWD"
theme_name="${1:-}"
result_file="$(mktemp "${TMPDIR:-/tmp}/palettetool-theme-export.XXXXXX")"

cleanup() {
    rm -f -- "$result_file"
}
trap cleanup EXIT

PALETTETOOL_NVIM_RUNTIME="$script_dir" \
PALETTETOOL_THEME_OUTPUT_DIRECTORY="$output_directory" \
PALETTETOOL_THEME_NAME="$theme_name" \
PALETTETOOL_THEME_RESULT_FILE="$result_file" \
    nvim --headless -i NONE \
    --cmd "lua vim.opt.runtimepath:append(vim.env.PALETTETOOL_NVIM_RUNTIME)" \
    --cmd "lua require('theme_exporter')" \
    -c "lua if vim.loader then vim.loader.enable(false) end" \
    -c "lua local ok, result = pcall(function() local exported = require('theme_exporter').export_theme(vim.env.PALETTETOOL_THEME_OUTPUT_DIRECTORY, vim.env.PALETTETOOL_THEME_NAME); if vim.fn.writefile({ exported.filename }, vim.env.PALETTETOOL_THEME_RESULT_FILE) ~= 0 then error('failed to record generated filename') end; return exported end); if not ok then vim.api.nvim_err_writeln(tostring(result)); vim.cmd('cquit 1') end" \
    -c "qa!"

IFS= read -r generated_filename < "$result_file"
if [[ -z "$generated_filename" ]]; then
    echo "failed to determine generated filename" >&2
    exit 1
fi
echo "$generated_filename"
