rem #bundle_zip_append.bat
rem if this is just clicked in explorer, copy will do "The syntax of the command is incorrect." and nothing happens

cd bundle
del engine.zip
rem powershell Compress-Archive engine engine.zip
copy c:\quak\id1\engine\form1.txt engine
dir engine

"C:\Program Files (x86)\WinRAR\WinRAR.exe" a -r engine.zip engine
dir
cd ..

rem cd c:\quak
echo %1
rem dir %1
rem dir #bundle_zip_append.bat
rem dir bundle\engine.zip

echo copy /B %1 + bundle\engine.zip %1
rem copy /B zircon_beta_linux_sdl + bundle\engine.zip zircon_beta_linux_sdl 
copy /B %1 + bundle\engine.zip %1

rem /B is "bare summary"
dir /B %1
rem pause (this gets the build process "stuck" in CodeBlocks, but not Visual Studio) so commenting.


