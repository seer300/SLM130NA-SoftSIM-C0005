# SLM130NA-SoftSIM-C0005
[中文版](https://github.com/seer300/SLM130NA-SoftSIM-C0005/blob/main/README-Chinese.md)  

## Prepare for first compilation
Go to the [Release page](https://github.com/seer300/SLM130NA-SoftSIM-C0005/releases/tag/buildtools) Download compilation kit (provided by XinYi)  
Unzip it to the `1200_SDK` directory

## Start compiling
### Windows platform
Go to the `1200_SDK` directory  
Run `xybuild.bat all` to start full compilation  
Run `xybuild.bat clean` to clean up the build environment

After successful compilation, the `1200_SDK/Allbins` directory will be the build product

## Package firmware
In the `SLM130NA-C0005/1200_SDK` directory, run the `cp_AllbinsF.bat` script

This script will copy the `arm.img cp.img loginfo.info` files from the `Allbins` to the`SLM130NA-C0005\V1200FB10002R00C0005\allbins` directory

Start the LogView software and execute the packaging process. The path is `SLM130NA-C0005\V1200FB10002R00C0005\allbins`