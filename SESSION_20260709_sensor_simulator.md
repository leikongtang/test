# 会话记录 - 传感器信号模拟器

**日期**: 2026-07-09

## 功能目标
开发 Qt C++ 桌面软件，模拟工业传感器（PNP/NPN）输出信号。

## 功能清单
- [x] PNP / NPN 传感器类型切换
- [x] 手动触发 / 释放信号
- [x] 电路示意图与电平状态显示
- [x] 自动脉冲模式（频率、占空比可调）
- [x] Release 编译运行验证

## 修改记录
- 新建 SensorSimulator 项目
  - `SensorSimulator.pro` - Qt 工程文件
  - `sensorsignal.h` - PNP/NPN 电平逻辑
  - `sensorcircuitwidget.cpp/h` - 电路示意图控件
  - `mainwindow.cpp/h` - 主界面与信号控制
  - `main.cpp` - 程序入口
- 可执行文件：`SensorSimulator/build-release/release/SensorSimulator.exe`

## 未完成事项
- 无
