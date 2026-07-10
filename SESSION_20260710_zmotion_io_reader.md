# 会话记录 - 正运动 IO 板信号读取工具

**日期**: 2026-07-10

## 功能目标
创建 Qt 桌面程序，通过以太网连接正运动 IO 板（默认 192.168.0.11），实时读取数字输入/输出信号状态。

## 修改记录
- 新建项目 `ZMotionIoReader/`
  - 使用正运动 PC 函数库（zmotion.dll / zauxdll.dll）
  - `ZAux_OpenEth` 以太网连接 IO 板
  - `ZAux_Direct_GetInMulti` / `ZAux_Direct_GetOutMulti` 批量读取 IO 状态
  - 图形界面显示 IN/OUT 通道指示灯（默认 32 路）
  - 支持配置 IP、通道数、刷新间隔
  - QSettings 保存连接参数
- 增强连接失败提示
  - 状态栏显示「连接失败」
  - 弹窗明确提示连接失败及排查建议
- v1.1.0 信号转换输出
  - 接收 IN 信号后支持直通、取反、偏移映射三种转换模式
  - 转换结果预览面板
  - 可选自动写入 IO 板 OUT 通道（ZAux_Direct_SetOutMulti）

## 未完成事项
- 无（需连接真实 IO 板硬件验证）
