# 会话记录 - CDiskCleaner 软件列表显示修复

**日期**: 2026-07-07  
**标签**: CDiskCleaner v1.3.0

## 功能目标

- 修复软件卸载页：状态栏显示数量但表格为空
- 增加软件图标列（读取注册表 DisplayIcon）
- 安装路径列加大字号、点击打开
- **新增「软件缓存」独立标签页，扩展常见软件缓存清理**

## 功能目标

- 修复软件卸载页：状态栏显示数量但表格为空
- 增加软件图标列（读取注册表 DisplayIcon）

## 修改记录

| 版本 | 修改内容 |
|------|----------|
| v1.2.4 | 注册 `InstalledApp` / `QVector<InstalledApp>` 元类型，修复跨线程信号 |
| v1.2.5 | 显式 `Qt::QueuedConnection`；新增图标列；读取 DisplayIcon |
| v1.2.6 | 安装路径列加大字号；蓝色链接样式；点击打开资源管理器 |
| v1.3.0 | 新增「软件缓存」标签页；扩展 12 款软件缓存；Chrome/Edge 多配置文件扫描 |
| v1.3.1 | （已回退）AppX 商店应用扫描 |

## 涉及文件

- `installedapp.h` - displayIcon 字段、QMetaType
- `main.cpp` - qRegisterMetaType、版本号
- `appuninstaller.cpp` - 读取 DisplayIcon
- `applistmodel.cpp/h` - 6 列含图标 DecorationRole
- `mainwindow.cpp` - 表格列宽、QueuedConnection

## 未完成事项

- [x] 编译通过并 push
- [ ] 用户验证图标与列表显示
