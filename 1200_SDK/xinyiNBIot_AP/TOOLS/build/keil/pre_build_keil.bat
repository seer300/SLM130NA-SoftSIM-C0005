@echo off

if exist %1 (
    if exist %1\%2 (
        rd /s /q %1\%2
    )
) else (
    md %1
)