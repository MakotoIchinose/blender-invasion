@echo off
echo Starting Blender with Oculus OpenXR support. This assumes the Oculus runtime
echo is installed in the default location. If this is not the case, please adjust
echo the path inside oculus.json.
echo.
pause
set XR_RUNTIME_JSON=%~dp0oculus.json
blender 
