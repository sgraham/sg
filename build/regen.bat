@echo off
build\bin\win\gn gen out\Debug --args="is_debug=true"
build\bin\win\gn gen out\Release --args="is_debug=false"
