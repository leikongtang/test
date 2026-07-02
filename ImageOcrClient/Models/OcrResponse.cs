using System;
using System.Collections.Generic;

namespace ImageOcrClient.Models
{
    public class OcrResponse
    {
        public bool bOK { get; set; }
        public string fileName { get; set; }
        public string filePath { get; set; }
        public int id { get; set; }
        public List<string> ocrResults { get; set; }
        public string resultName { get; set; }
        public string sendTime { get; set; }
    }
}
