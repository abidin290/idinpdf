@echo off
echo Mengkonfigurasi project dengan CMake...
mkdir build
cd build
cmake ..
if %errorlevel% neq 0 (
    echo Gagal menjalankan CMake. Pastikan CMake sudah terinstall.
    pause
    exit /b %errorlevel%
)

echo.
echo Mengkompilasi program...
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo Gagal mengkompilasi.
    pause
    exit /b %errorlevel%
)

echo.
echo Selesai! Jika menggunakan MSVC, program ada di folder Release\idinpdf.exe
echo Jika menggunakan MinGW, program ada di build\idinpdf.exe
pause
