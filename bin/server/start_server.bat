@echo off

call "kill_server.bat"

start logger.exe --cid=9000 --gus=11
start login.exe --cid=9001 --gus=11
start dbmgr.exe --cid=9002 --gus=11
start cellmgr.exe --cid=9003 --gus=11
start basemgr.exe --cid=9004 --gus=11
start baseapp.exe --cid=9005 --gus=11
start cellapp.exe --cid=9006 --gus=11