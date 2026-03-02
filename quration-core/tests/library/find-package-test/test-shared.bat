@echo off

set VCPKG=C:\vcpkg\scripts\buildsystems\vcpkg.cmake

set TEST_DIR=%~dp0
set TEST_BUILD_DIR=%TEST_DIR%\build-test
set QRET_DIR=%TEST_DIR%\..\..\..\..
set QRET_BUILD_DIR=%TEST_DIR%\build-library
set LIB_DIR=%TEST_DIR%\libqret

set BOOST_INCLUDE_DIR=C:\vcpkg\installed\x64-windows\include
set FMT_INCLUDE_DIR=C:\vcpkg\installed\x64-windows\include
set NLOHMANN_JSON_INCLUDE_DIR=C:\vcpkg\installed\x64-windows\include

echo Delete cache
rmdir /s /q %TEST_BUILD_DIR%
rmdir /s /q %QRET_BUILD_DIR%
rmdir /s /q %LIB_DIR%

echo Build qret library
cmake -S %QRET_DIR% -B %QRET_BUILD_DIR% -DCMAKE_TOOLCHAIN_FILE=%VCPKG% ^
    -DBUILD_SHARED_LIBS=ON -DQRET_DEV_MODE=OFF ^
    -DQRET_BUILD_APPLICATION=OFF -DQRET_BUILD_EXAMPLE=OFF -DQRET_BUILD_TEST=OFF -DQRET_BUILD_PYTHON=OFF -DQRET_MEASURE_COVERAGE=OFF ^
    -DQRET_USE_PEGTL=ON -DQRET_USE_QULACS=OFF
if %errorlevel% neq 0 exit /b %errorlevel%
cmake --build %QRET_BUILD_DIR% --config Release
if %errorlevel% neq 0 exit /b %errorlevel%
cmake --install %QRET_BUILD_DIR% --prefix %LIB_DIR%
if %errorlevel% neq 0 exit /b %errorlevel%

echo Build test using qret library

echo Use CMake
cmake -S %TEST_DIR% -B %TEST_BUILD_DIR% -DCMAKE_TOOLCHAIN_FILE=%VCPKG% -DCMAKE_PREFIX_PATH=%LIB_DIR%
if %errorlevel% neq 0 exit /b %errorlevel%
cmake --build %TEST_BUILD_DIR% --config Release
if %errorlevel% neq 0 exit /b %errorlevel%

echo Copy runtime DLLs
copy /y "%LIB_DIR%\bin\*.dll" "%TEST_BUILD_DIR%\Release\"
if %errorlevel% neq 0 exit /b %errorlevel%
%TEST_BUILD_DIR%\Release\library-test.exe
if %errorlevel% neq 0 exit /b %errorlevel%

exit /b 0
