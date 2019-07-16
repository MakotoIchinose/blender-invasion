@echo off
echo Starting blender with oculus openXR support, this assumes the oculus runtime
echo is installed in the default location, if this is not the case please adjust the
echo path inside oculus.josn
echo.
echo please note that openXR support is EXTREMELY experimental at this point
echo.
pause
set XR_RUNTIME_JSON=%~dp0oculus.json
blender 