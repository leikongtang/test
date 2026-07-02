# 会话记录 - C++ Qt 远程图片发送客户端

## 功能目标
- Qt Widgets 客户端，选择图片并发送到 OCR 服务器
- 发送端口 55513
- JSON 格式：`{"image":"base64"}\n`
- 使用原文件 Base64，不做 BMP 转换

## 修改记录
- 2026-07-02：创建 ImageOcrClientCpp（Qt 5.15 + C++11）

## 未完成事项
- 55555 端口 OCR 结果接收与展示
