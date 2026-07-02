using System.Drawing;
using System.Drawing.Imaging;
using System.IO;

namespace ImageOcrClient.Services
{
    public static class ImageHelper
    {
        public static string BitmapToBase64(Bitmap bitmap)
        {
            // 使用 MemoryStream 来处理 Bitmap 数据
            using (var ms = new MemoryStream())
            {
                // 将 Bitmap 保存到 MemoryStream 中
                bitmap.Save(ms, ImageFormat.Bmp);

                byte[] imageBytes = ms.ToArray();

                // 将字节数组转换为 Base64 字符串
                return System.Convert.ToBase64String(imageBytes);
            }
        }

        public static string FileToBase64(string filePath)
        {
            byte[] imageBytes = File.ReadAllBytes(filePath);
            return System.Convert.ToBase64String(imageBytes);
        }
    }
}
