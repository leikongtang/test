using System;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Web.Script.Serialization;

namespace ImageOcrClient.Services
{
    public class OcrClientService
    {
        private const int SendPort = 55513;
        private const int TimeoutMs = 30000;

        private readonly JavaScriptSerializer _serializer;

        public OcrClientService()
        {
            _serializer = new JavaScriptSerializer
            {
                MaxJsonLength = int.MaxValue
            };
        }

        public async Task SendImageAsync(string serverIp, string base64Image, CancellationToken cancellationToken)
        {
            if (string.IsNullOrWhiteSpace(serverIp))
            {
                throw new ArgumentException("服务器 IP 不能为空。", nameof(serverIp));
            }

            if (string.IsNullOrWhiteSpace(base64Image))
            {
                throw new ArgumentException("图片 Base64 数据不能为空。", nameof(base64Image));
            }

            var payload = _serializer.Serialize(new { image = base64Image }) + "\n";
            byte[] data = Encoding.UTF8.GetBytes(payload);

            using (var client = new TcpClient())
            {
                var connectTask = client.ConnectAsync(serverIp, SendPort);
                var completed = await Task.WhenAny(connectTask, Task.Delay(TimeoutMs, cancellationToken));
                if (completed != connectTask)
                {
                    throw new TimeoutException(string.Format("连接发送端口 {0} 超时。", SendPort));
                }

                await connectTask;
                client.SendTimeout = TimeoutMs;

                using (NetworkStream stream = client.GetStream())
                {
                    await stream.WriteAsync(data, 0, data.Length, cancellationToken);
                    await stream.FlushAsync(cancellationToken);
                    await Task.Delay(200, cancellationToken);
                }
            }
        }
    }
}
