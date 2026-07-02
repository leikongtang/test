using System;
using System.Drawing;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
using ImageOcrClient.Services;

namespace ImageOcrClient
{
    public partial class MainForm : Form
    {
        private readonly OcrClientService _ocrClientService = new OcrClientService();
        private string _selectedImagePath;

        public MainForm()
        {
            InitializeComponent();
        }

        private void btnSelectAndSend_Click(object sender, EventArgs e)
        {
            using (var dialog = new OpenFileDialog())
            {
                dialog.Title = "选择图片";
                dialog.Filter = "图片文件|*.bmp;*.jpg;*.jpeg;*.png;*.gif;*.tif;*.tiff|所有文件|*.*";
                dialog.Multiselect = false;

                if (dialog.ShowDialog() != DialogResult.OK)
                {
                    return;
                }

                _selectedImagePath = dialog.FileName;
                lblSelectedFile.Text = Path.GetFileName(_selectedImagePath);
                LoadPreview(_selectedImagePath);
                _ = SendImageAsync();
            }
        }

        private void LoadPreview(string imagePath)
        {
            picturePreview.Image?.Dispose();
            picturePreview.Image = Image.FromFile(imagePath);
        }

        private async Task SendImageAsync()
        {
            btnSelectAndSend.Enabled = false;
            AppendLog("开始发送图片...");

            try
            {
                string serverIp = txtServerIp.Text.Trim();
                string base64 = ImageHelper.FileToBase64(_selectedImagePath);

                AppendLog(string.Format("图片已编码（原文件），Base64 长度: {0}", base64.Length));

                using (var cts = new CancellationTokenSource(TimeSpan.FromSeconds(30)))
                {
                    await _ocrClientService.SendImageAsync(serverIp, base64, cts.Token);
                }

                AppendLog("图片发送成功。");
            }
            catch (Exception ex)
            {
                AppendLog("发送失败: " + ex.Message);
                MessageBox.Show(this, ex.Message, "错误", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            finally
            {
                btnSelectAndSend.Enabled = true;
            }
        }

        private void AppendLog(string message)
        {
            if (InvokeRequired)
            {
                BeginInvoke(new Action<string>(AppendLog), message);
                return;
            }

            txtLog.AppendText(string.Format("[{0:HH:mm:ss}] {1}{2}", DateTime.Now, message, Environment.NewLine));
        }

        protected override void OnFormClosed(FormClosedEventArgs e)
        {
            picturePreview.Image?.Dispose();
            base.OnFormClosed(e);
        }
    }
}
