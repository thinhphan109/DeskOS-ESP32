@echo off
cd /d "%~dp0"
py -3 tray_agent.py --config "%~dp0agent_config.json"
if errorlevel 1 pause
