@echo off
chcp 65001 >nul 2>&1
title 上位机网络配置工具

:: ============================================
:: 自动为有线网卡配置静态IP以连接下位机(NUC)
:: 下位机IP: 192.168.1.123
:: 上位机IP: 192.168.1.100 (自动分配)
:: ============================================

:: 检查管理员权限
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [提示] 需要管理员权限，正在请求提升...
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

set NUC_IP=192.168.1.123
set PC_IP=192.168.1.100
set SUBNET=255.255.255.0
set ADAPTER_NAME=以太网

echo ============================================
echo   上位机网络自动配置工具
echo ============================================
echo.
echo   下位机(NUC) IP : %NUC_IP%
echo   上位机 IP      : %PC_IP%
echo   子网掩码       : %SUBNET%
echo   网卡名称       : %ADAPTER_NAME%
echo.

:: 检查网卡是否存在
netsh interface show interface name="%ADAPTER_NAME%" >nul 2>&1
if %errorLevel% neq 0 (
    echo [错误] 找不到网卡 "%ADAPTER_NAME%"
    echo.
    echo 当前可用网卡:
    netsh interface show interface
    echo.
    set /p ADAPTER_NAME="请输入正确的有线网卡名称: "
)

:: 检查网卡连接状态
netsh interface show interface name="%ADAPTER_NAME%" | findstr /i "connected" >nul 2>&1
if %errorLevel% neq 0 (
    echo [警告] 网卡 "%ADAPTER_NAME%" 未连接，请检查网线是否插好
    echo.
    pause
    exit /b 1
)

:: 先ping一下看是否已经通了
echo [1/3] 检查当前连通性...
ping -n 1 -w 1000 %NUC_IP% | findstr "TTL=" >nul 2>&1
if %errorLevel% equ 0 (
    echo [OK] 已经可以连通 %NUC_IP%，无需配置
    echo.
    pause
    exit /b 0
)

:: 配置静态IP
echo [2/3] 正在配置 %ADAPTER_NAME% -> %PC_IP% ...
netsh interface ipv4 set address name="%ADAPTER_NAME%" static %PC_IP% %SUBNET%
if %errorLevel% neq 0 (
    echo [错误] 配置IP失败
    pause
    exit /b 1
)

:: 等待网卡生效
timeout /t 2 /nobreak >nul

:: 验证连通性
echo [3/3] 验证与下位机的连通性...
ping -n 3 -w 1000 %NUC_IP% | findstr "TTL=" >nul 2>&1
if %errorLevel% equ 0 (
    echo.
    echo ============================================
    echo   [成功] 网络配置完成！
    echo   上位机 %PC_IP% -> 下位机 %NUC_IP% 已连通
    echo ============================================
) else (
    echo.
    echo [警告] 配置完成但ping不通 %NUC_IP%
    echo 可能原因:
    echo   1. 网线未插好
    echo   2. 下位机未开机或IP不是 %NUC_IP%
    echo   3. 下位机防火墙阻止了ping
)

echo.
pause
