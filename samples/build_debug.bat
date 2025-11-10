@echo off
chcp 65001 >nul
echo Building sample with Debug SDK...
call "%~dp0build_sample.bat" Debug
