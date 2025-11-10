@echo off
chcp 65001 >nul
echo Building sample with Release SDK...
call "%~dp0build_sample.bat" Release
