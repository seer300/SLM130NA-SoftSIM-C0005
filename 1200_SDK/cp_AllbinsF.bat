 copy .\Allbins\arm.img 		..\V1200FB10002R00C0005\allbins\ /B /Y
 copy .\Allbins\cp.img 		..\V1200FB10002R00C0005\allbins\ /B /Y
 copy .\Allbins\loginfo.info 	..\V1200FB10002R00C0005\allbins\ /B /Y
@REM  复制elf
 xcopy .\Allbins\xinyiNBIot_AP\elf ..\V1200FB10002R00C0005\elf\ap-elf /E /I /Y
 xcopy .\Allbins\xinyiNBIot_CP\elf ..\V1200FB10002R00C0005\elf\cp-elf /E /I /Y
 del ..\V1200FB10002R00C0005\elf\ap-elf\xinyiNBSoC.asm
 del ..\V1200FB10002R00C0005\elf\cp-elf\xinyiNBSoC.asm
   