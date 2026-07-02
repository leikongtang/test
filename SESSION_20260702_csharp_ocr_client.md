# 会话记录 - C# 远程图片发送客户端

## 功能目标
- WinForms 客户端，选择图片并发送到 OCR 服务器
- 发送端口 55513，接收端口 55555
- JSON 格式：`{"image":"base64"}`

## 修改记录
- 2026-07-02：创建 ImageOcrClient 项目（.NET Framework 4.8 WinForms）
- 2026-07-02：修复发送 JSON 缺少换行结束符；改用原文件 Base64，避免 BMP 体积过大

## 未完成事项
- 55555 端口 OCR 结果接收与展示
