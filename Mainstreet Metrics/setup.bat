@echo off
REM Simple CGI Server Setup Script for Windows

echo === Business Directory CGI Server ===
echo.

REM Compile the CGI program
echo Compiling main.cpp...
echo NOTE: You need to have MongoDB C++ driver installed
echo.

g++ -std=c++17 main.cpp -o business.exe ^
    -I"C:\mongo-cxx-driver\include\mongocxx\v_noabi" ^
    -I"C:\mongo-cxx-driver\include\bsoncxx\v_noabi" ^
    -I"C:\mongo-c-driver\include\libmongoc-1.0" ^
    -I"C:\mongo-c-driver\include\libbson-1.0" ^
    -L"C:\mongo-cxx-driver\lib" ^
    -L"C:\mongo-c-driver\lib" ^
    -lmongocxx -lbsoncxx -lmongoc-1.0 -lbson-1.0

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo Compilation failed! Please update the include and library paths.
    echo Edit setup.bat and set the correct paths for your MongoDB installation.
    pause
    exit /b 1
)

echo Compiled successfully: business.exe
echo.

REM Create a simple Python HTTP server with CGI support
echo Creating Python CGI server script...
(
echo import http.server
echo import socketserver
echo import os
echo import subprocess
echo.
echo PORT = 8000
echo.
echo class WindowsCGIHTTPRequestHandler^(http.server.CGIHTTPRequestHandler^):
echo     cgi_directories = ['/']
echo.    
echo     def is_cgi^(self^):
echo         path = self.path.split^('?'^)[0]
echo         if path.endswith^('.exe'^):
echo             self.cgi_info = '', path.lstrip^('/'^)
echo             return True
echo         return False
echo.
echo os.chdir^(os.path.dirname^(os.path.abspath^(__file__^)^)^)
echo.
echo with socketserver.TCPServer^(^("", PORT^), WindowsCGIHTTPRequestHandler^) as httpd:
echo     print^(f"Server running at http://localhost:{PORT}/"^)
echo     print^(f"Access the business directory at: http://localhost:{PORT}/business.exe"^)
echo     print^("Press Ctrl+C to stop the server"^)
echo     httpd.serve_forever^(^)
) > server.py

echo Setup complete!
echo.
echo To start the server, run:
echo   python server.py
echo.
echo Then open your browser to:
echo   http://localhost:8000/business.exe
echo.
pause
