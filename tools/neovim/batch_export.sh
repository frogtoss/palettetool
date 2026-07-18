#!/usr/bin/env bash

set -euo pipefail

if (( $# != 0 )); then
    echo "usage: $0" >&2
    exit 2
fi

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
output_path="${PWD}/theme.pal.json"

PALETTETOOL_NVIM_RUNTIME="$script_dir" \
PALETTETOOL_THEME_OUTPUT="$output_path" \
    nvim --headless -i NONE \
    --cmd "lua vim.opt.runtimepath:append(vim.env.PALETTETOOL_NVIM_RUNTIME)" \
    --cmd "lua require('theme_exporter')" \
    -c "lua if vim.loader then vim.loader.enable(false) end" \
    -c "lua local ok, err = pcall(function() require('theme_exporter').export(vim.env.PALETTETOOL_THEME_OUTPUT) end); if not ok then vim.api.nvim_err_writeln(tostring(err)); vim.cmd('cquit 1') end" \
    -c "qa!"
