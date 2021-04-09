mkdir build
Set-Location build
cmake -G "Visual Studio 16 2019" -A Win32 -DCMAKE_CONFIGURATION_TYPES:STRING="MinSizeRel" -S ..
cmake --build . --config MinSizeRel
Set-Location ..
Set-Location CFGEditor
nuget install Newtonsoft.Json -Version 10.0.3 -OutputDirectory packages
Set-Location "CFG Editor"
dotnet restore
msbuild "CFG Editor Project.csproj" -p:Configuration=Release
Set-Location ..
Set-Location ..
