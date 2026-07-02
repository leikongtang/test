namespace ImageOcrClient
{
    partial class MainForm
    {
        private System.ComponentModel.IContainer components = null;

        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        private void InitializeComponent()
        {
            this.lblServerIp = new System.Windows.Forms.Label();
            this.txtServerIp = new System.Windows.Forms.TextBox();
            this.btnSelectAndSend = new System.Windows.Forms.Button();
            this.lblSelectedFile = new System.Windows.Forms.Label();
            this.picturePreview = new System.Windows.Forms.PictureBox();
            this.lblLog = new System.Windows.Forms.Label();
            this.txtLog = new System.Windows.Forms.TextBox();
            ((System.ComponentModel.ISupportInitialize)(this.picturePreview)).BeginInit();
            this.SuspendLayout();
            // 
            // lblServerIp
            // 
            this.lblServerIp.AutoSize = true;
            this.lblServerIp.Location = new System.Drawing.Point(12, 15);
            this.lblServerIp.Name = "lblServerIp";
            this.lblServerIp.Size = new System.Drawing.Size(65, 12);
            this.lblServerIp.TabIndex = 0;
            this.lblServerIp.Text = "服务器 IP";
            // 
            // txtServerIp
            // 
            this.txtServerIp.Location = new System.Drawing.Point(83, 12);
            this.txtServerIp.Name = "txtServerIp";
            this.txtServerIp.Size = new System.Drawing.Size(160, 21);
            this.txtServerIp.TabIndex = 1;
            this.txtServerIp.Text = "127.0.0.1";
            // 
            // btnSelectAndSend
            // 
            this.btnSelectAndSend.Location = new System.Drawing.Point(259, 10);
            this.btnSelectAndSend.Name = "btnSelectAndSend";
            this.btnSelectAndSend.Size = new System.Drawing.Size(130, 25);
            this.btnSelectAndSend.TabIndex = 2;
            this.btnSelectAndSend.Text = "选择图片并发送";
            this.btnSelectAndSend.UseVisualStyleBackColor = true;
            this.btnSelectAndSend.Click += new System.EventHandler(this.btnSelectAndSend_Click);
            // 
            // lblSelectedFile
            // 
            this.lblSelectedFile.AutoSize = true;
            this.lblSelectedFile.Location = new System.Drawing.Point(395, 15);
            this.lblSelectedFile.Name = "lblSelectedFile";
            this.lblSelectedFile.Size = new System.Drawing.Size(77, 12);
            this.lblSelectedFile.TabIndex = 3;
            this.lblSelectedFile.Text = "未选择文件";
            // 
            // picturePreview
            // 
            this.picturePreview.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.picturePreview.Location = new System.Drawing.Point(14, 45);
            this.picturePreview.Name = "picturePreview";
            this.picturePreview.Size = new System.Drawing.Size(658, 280);
            this.picturePreview.SizeMode = System.Windows.Forms.PictureBoxSizeMode.Zoom;
            this.picturePreview.TabIndex = 4;
            this.picturePreview.TabStop = false;
            // 
            // lblLog
            // 
            this.lblLog.AutoSize = true;
            this.lblLog.Location = new System.Drawing.Point(12, 335);
            this.lblLog.Name = "lblLog";
            this.lblLog.Size = new System.Drawing.Size(29, 12);
            this.lblLog.TabIndex = 5;
            this.lblLog.Text = "日志";
            // 
            // txtLog
            // 
            this.txtLog.Location = new System.Drawing.Point(14, 353);
            this.txtLog.Multiline = true;
            this.txtLog.Name = "txtLog";
            this.txtLog.ReadOnly = true;
            this.txtLog.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.txtLog.Size = new System.Drawing.Size(658, 100);
            this.txtLog.TabIndex = 6;
            // 
            // MainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 12F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(684, 465);
            this.Controls.Add(this.txtLog);
            this.Controls.Add(this.lblLog);
            this.Controls.Add(this.picturePreview);
            this.Controls.Add(this.lblSelectedFile);
            this.Controls.Add(this.btnSelectAndSend);
            this.Controls.Add(this.txtServerIp);
            this.Controls.Add(this.lblServerIp);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.MaximizeBox = false;
            this.Name = "MainForm";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "远程图片发送客户端";
            ((System.ComponentModel.ISupportInitialize)(this.picturePreview)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();
        }

        private System.Windows.Forms.Label lblServerIp;
        private System.Windows.Forms.TextBox txtServerIp;
        private System.Windows.Forms.Button btnSelectAndSend;
        private System.Windows.Forms.Label lblSelectedFile;
        private System.Windows.Forms.PictureBox picturePreview;
        private System.Windows.Forms.Label lblLog;
        private System.Windows.Forms.TextBox txtLog;
    }
}
