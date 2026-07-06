# 会话记录 - Qt 平台插件启动失败修复

**日期**: 2026-07-06

## 问题

启动 Qt 程序报错：`no Qt platform plugin could be initialized`

## 原因

部分项目 release 目录仅有 exe/obj，未执行 `windeployqt`，缺少 `platforms/qwindows.dll` 及 `Qt5*.dll`。

## 功能/修改记录

- [x] 对 DragonDeskPet、ImageDistanceMeasure、ImageOcrClientCpp 执行 windeployqt
- [x] 在各 .pro 增加 release 构建后自动 windeployqt
- [x] 验证 DragonDeskPet.exe 可正常启动

## 未完成事项

- 无

## 使用说明

release 构建完成后会自动部署 Qt 依赖。若手动复制 exe，请在 release 目录执行：

```bat
"D:\Qt\5.15.2\msvc2019_64\bin\windeployqt.exe" --no-translations YourApp.exe
```

确保 `platforms\qwindows.dll` 与 `Qt5Core.dll` 等与 exe 同目录（或在子目录 platforms/）。
