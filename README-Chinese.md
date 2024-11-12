# SLM130NA-SoftSIM-C0005
美格NB-iot模块 SLM130NA   
使用芯翼XY1200S平台开发

## 首次编译前准备
请前往[Release页面](https://github.com/seer300/SLM130NA-SoftSIM-C0005/releases/tag/buildtools)下载编译工具包（芯翼提供）  
解压到`1200_SDK`目录下

## 开始编译
### Windows平台
进入`1200_SDK`目录  
运行`xybuild.bat all`开始完整编译  
运行`xybuild.bat clean`清理编译环境  

编译成功后，`1200_SDK/Allbins`目录下即为编译产物

## 打包固件
在`SLM130NA-C0005/1200_SDK`目录下，运行`cp_AllbinsF.bat`脚本  

该脚本会将`Allbins`下的`arm.img cp.img loginfo.info`三个文件拷贝到`SLM130NA-C0005\V1200FB10002R00C0005\allbins`目录下  

启动LogView软件，执行打包流程。路径选择`SLM130NA-C0005\V1200FB10002R00C0005\allbins`目录即可
