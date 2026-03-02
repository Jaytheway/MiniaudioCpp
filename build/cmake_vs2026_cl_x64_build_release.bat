@echo off
pushd ..
python build/_build_util.py "Visual Studio 18 2026" "build/VS2026_CL_x64" x64 -DBUILD_TESTING=OFF --config Release %*
popd
pause