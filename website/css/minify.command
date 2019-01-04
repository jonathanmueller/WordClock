rm minified.css
uglifycss $(find . -name '*.css' -a ! -wholename './old/*' | sort) > minified.css