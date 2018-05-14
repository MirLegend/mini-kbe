@echo off

call "kill_server.bat"

start logger.exe --cid=9000 --gus=11
start cellapp.exe --cid=9006 --gus=11