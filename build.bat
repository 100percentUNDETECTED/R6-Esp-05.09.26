@echo off
echo Building R6-External-ESP...
cl /EHsc /std:c++17 /O2 /I. /Iimgui /Fe:R6-External-ESP.exe main.cpp imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp imgui/backends/imgui_impl_win32.cpp imgui/backends/imgui_impl_dx11.cpp /link d3d11.lib d3dcompiler.lib dwmapi.lib user32.lib advapi32.lib gdi32.lib
if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
) else (
    echo Build successful.
)
