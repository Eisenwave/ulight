#!/bin/bash

node scripts/theme-to-css.js \
  -i themes \
  -o www/css/theme.css \
  -t
# node scripts/theme-to-css.js \
#   -i themes/ulight.json \
#   -o www/css/code.css \
#   -B
