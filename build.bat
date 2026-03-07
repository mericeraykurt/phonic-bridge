@echo off
setlocal

:: ==========================================
:: PhonicBridge MSVC Build Script
:: ==========================================

:: 1. Initialize MSVC Environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1

:: 2. Compile Resource File (App Icon)
rc.exe /nologo version.rc

:: 3. Compile the C++ Core and Mongoose Web Server
:: We link against standard Windows Libraries (WS2_32, WinMM, Ole32)
cl.exe /EHsc /std:c++17 /O2 /MD /D NDEBUG ^
    main.cpp mongoose.c version.res ^
    /link /out:app.exe user32.lib ws2_32.lib gdi32.lib winmm.lib ole32.lib kernel32.lib advapi32.lib shell32.lib

if %ERRORLEVEL% equ 0 (
    echo.
    echo ==============================================
    echo [SUCCESS] MSVC Hybrid Compilation Complete.
    echo Launch app.exe to run the Dual-Boot Interface.
    echo ==============================================
) else (
    echo.
    echo [ERROR] MSVC Compilation Failed! Error Code: %ERRORLEVEL%
)

endlocal
