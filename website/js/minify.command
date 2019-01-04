#!/bin/sh
rm minified.js
uglifyjs $(find . -name '*.js' -a ! -wholename './old/*' | sort) --beautify > minified.js