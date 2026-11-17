@echo off
cd /d "%~dp0"
where py >nul 2>nul && (py -3 flash.py %* & goto fin)
where python >nul 2>nul && (python flash.py %* & goto fin)
echo Python 3 est introuvable. Installez-le depuis https://www.python.org/downloads/ (cochez "Add to PATH").
:fin
pause
