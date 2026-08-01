#ifndef DOWNLOAD_FILE_H
#define DOWNLOAD_FILE_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <ESP32_FTPClient.h>

// ================= MACRO CẤU HÌNH HỆ THỐNG =================
#define WIFI_SSID           "202 H30"
#define WIFI_PASS           "68686868"

#define FTP_SERVER          "ftpupload.net"
#define FTP_USER            "if0_42484011"
#define FTP_PASS            "Sang03022004"
#define FTP_PATH            "/htdocs/Firmware"
#define FTP_FILE_TARGET     "App.hex"

// Dung lượng tối đa bộ nhớ đệm cho file HEX (VD: 50KB)
#define MAX_HEX_BUF_SIZE    (50 * 1024) 

// ================= BIẾN TOÀN CỤC (GLOBAL VARS) =================
// Mảng lưu dữ liệu file HEX được host_bootloader.h gọi tới
char file_hex[MAX_HEX_BUF_SIZE];

// Đối tượng FTP Client
ESP32_FTPClient ftp(FTP_SERVER, FTP_USER, FTP_PASS);

// ================= HÀM XỬ LÝ (FUNCTIONS) =====================

/**
 * @brief Hàm kết nối Wi-Fi có Timeout
 */
bool wifi_connect(void) 
{
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("[FTP] Connecting Wifi...");
  uint8_t timeout = 0;
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (++timeout > 30) { // Timeout 15 giây
      Serial.println("\n[FTP] WiFi Connection Failed!");
      return false;
    }
  }
  
  Serial.println("\n[FTP] WiFi Connected!");
  Serial.print("[FTP] IP address: ");
  Serial.println(WiFi.localIP());
  return true;
}

/**
 * @brief Kết nối FTP Server và kéo dữ liệu file.hex về mảng file_hex
 */
bool dowload_file_fetch(void) 
{
  if (WiFi.status() != WL_CONNECTED) {
    if (!wifi_connect()) return false;
  }

  Serial.printf("[FTP] Free Heap before download: %d bytes\n", ESP.getFreeHeap());
  Serial.println("[FTP] Connecting to FTP Server...");
  
  ftp.OpenConnection();
  ftp.ChangeWorkDir(FTP_PATH);

  size_t fileSize = 0;
  String list[128];

  ftp.InitFile("Type A");
  ftp.ContentList("", list);

  // Chuyển tên file target sang chữ thường để so sánh không phân biệt HOA/thường
  String targetLower = String(FTP_FILE_TARGET);
  targetLower.toLowerCase();

  for (uint8_t i = 0; i < 128; i++) {
    if (list[i].length() == 0) break;

    String lineLower = list[i];
    lineLower.toLowerCase();

    if (lineLower.indexOf(targetLower) > -1) {
      uint8_t indexSize = lineLower.indexOf("size") + 5;
      uint8_t indexMod  = lineLower.indexOf("modify") - 1;
      fileSize = lineLower.substring(indexSize, indexMod).toInt();
      break;
    }
  }

  Serial.printf("[FTP] Target File: %s | Size: %d bytes\n", FTP_FILE_TARGET, fileSize);

  // Kiểm tra lỗi điều kiện
  if (fileSize == 0) {
    Serial.println("[FTP] ERROR: File not found on server or file size is 0!");
    ftp.CloseConnection();
    return false;
  }

  if (fileSize >= MAX_HEX_BUF_SIZE) {
    Serial.println("[FTP] ERROR: File size exceeds MAX_HEX_BUF_SIZE buffer!");
    ftp.CloseConnection();
    return false;
  }

  // Tải file nhị phân vào mảng file_hex
  ftp.InitFile("Type I");
  Serial.println("[FTP] Downloading file from cloud...");
  
  ftp.DownloadFile(FTP_FILE_TARGET, (unsigned char*)file_hex, fileSize, false);
  file_hex[fileSize] = '\0'; // Đánh dấu kết thúc chuỗi NULL cho các hàm cắt chuỗi

  ftp.CloseConnection();
  Serial.println("[FTP] Download complete! Connection Closed.");
  return true;
}

/**
 * @brief Hàm khởi tạo chính của module Download File
 */
bool dowload_file_init(void) 
{
  memset(file_hex, 0, sizeof(file_hex));
  
  if (!wifi_connect()) {
    return false;
  }
  
  return dowload_file_fetch();
}

#endif // DOWNLOAD_FILE_H