#ifndef DOWNLOAD_FILE_H
#define DOWNLOAD_FILE_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <ESP32_FTPClient.h>
#include <Preferences.h> // Thư viện đọc/ghi Flash NVS chống mất điện

// ================= MACRO CẤU HÌNH HỆ THỐNG =================
#define WIFI_SSID           "202 H30"
#define WIFI_PASS           "68686868"

#define FTP_SERVER          "ftpupload.net"
#define FTP_USER            "if0_42484011"
#define FTP_PASS            "Sang03022004"
#define FTP_PATH            "/htdocs/Firmware"

#define FILE_VERSION_TARGET "Version.txt"
#define FILE_HEX_TARGET     "App.hex"

// Dung lượng tối đa bộ nhớ đệm cho file HEX (VD: 50KB)
#define MAX_HEX_BUF_SIZE    (50 * 1024) 

// Thời gian định kỳ kiểm tra update (Mặc định: 5 phút = 300,000 ms)
#define OTA_CHECK_INTERVAL  (2 * 60 * 1000UL) 

// ================= BIẾN TOÀN CỤC =================
char file_hex[MAX_HEX_BUF_SIZE];
String current_version = "0.0.0"; // Version hiện tại đang chạy trên STM32
String new_version = "";          // Version đọc từ Cloud về

ESP32_FTPClient ftp(FTP_SERVER, FTP_USER, FTP_PASS);
Preferences prefs;
unsigned long last_check_time = 0;

// ================= HÀM XỬ LÝ (FUNCTIONS) =====================

/**
 * @brief Kết nối Wi-Fi có Timeout
 */
bool wifi_connect(void) 
{
  if (WiFi.status() == WL_CONNECTED) return true;

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("[FTP] Connecting Wifi...");
  uint8_t timeout = 0;
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (++timeout > 30) { // Timeout 15s
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
 * @brief Tải duy nhất file version.txt từ Cloud về dạng chuỗi String
 */
String fetch_cloud_version(void) 
{
  ftp.OpenConnection();
  ftp.ChangeWorkDir(FTP_PATH);

  String version_str = "";
  ftp.InitFile("Type A");
  
  // Tải nội dung version.txt
  ftp.DownloadString(FILE_VERSION_TARGET, version_str);
  ftp.CloseConnection();

  // Xóa khoảng trắng / xuống dòng thừa (\r, \n)
  version_str.trim();
  return version_str;
}

/**
 * @brief Tải file App.hex về mảng file_hex khi có bản cập nhật mới
 */
bool fetch_hex_file(void) 
{
  ftp.OpenConnection();
  ftp.ChangeWorkDir(FTP_PATH);

  size_t fileSize = 0;
  String list[128];

  ftp.InitFile("Type A");
  ftp.ContentList("", list);

  String targetLower = String(FILE_HEX_TARGET);
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

  if (fileSize == 0 || fileSize >= MAX_HEX_BUF_SIZE) {
    Serial.println("[FTP] ERROR: File App.hex not found or exceeds MAX_HEX_BUF_SIZE!");
    ftp.CloseConnection();
    return false;
  }

  ftp.InitFile("Type I");
  Serial.println("[FTP] Downloading App.hex to RAM...");
  
  ftp.DownloadFile(FILE_HEX_TARGET, (unsigned char*)file_hex, fileSize, false);
  file_hex[fileSize] = '\0'; // Đánh dấu kết thúc chuỗi

  ftp.CloseConnection();
  Serial.println("[FTP] App.hex Downloaded successfully!");
  return true;
}

/**
 * @brief Kiểm tra version và tải file Hex nếu phát hiện bản mới
 */
bool check_and_download_ota(void) 
{
  if (!wifi_connect()) return false;

  Serial.println("\n[OTA Check] Checking version.txt from Cloud...");
  new_version = fetch_cloud_version();
  
  Serial.printf("[OTA Check] Current Ver: %s | Cloud Ver: %s\n", current_version.c_str(), new_version.c_str());

  // Nếu file rỗng hoặc trùng version hiện tại -> Bỏ qua
  if (new_version.length() == 0 || new_version.equals(current_version)) {
    Serial.println("[OTA Check] Firmware is up to date. Skip downloading App.hex.");
    return false;
  }

  // Nếu có version mới -> Tiến hành kéo App.hex về
  Serial.println("[OTA Check] New Version detected! Starting download...");
  return fetch_hex_file();
}

/**
 * @brief Khởi tạo module Download File
 */
bool dowload_file_init(void) 
{
  memset(file_hex, 0, sizeof(file_hex));
  
  // Đọc Version đã lưu trong Flash NVS của ESP32 ra
  prefs.begin("ota_info", false);
  current_version = prefs.getString("ver", "0.0.0"); // Mặc định 0.0.0 nếu chưa có
  prefs.end();
  
  Serial.printf("[Init] System started with Firmware Version: %s\n", current_version.c_str());

  wifi_connect();
  
  // Lần đầu bật nguồn: Kiểm tra ngay có bản mới không
  return check_and_download_ota();
}

/**
 * @brief Gọi trong loop() để kiểm tra cập nhật định kỳ (Polling)
 */
void dowload_file_handle(void) 
{
  if (millis() - last_check_time >= OTA_CHECK_INTERVAL) {
    last_check_time = millis();
    check_and_download_ota();
  }
}

/**
 * @brief Lưu Version mới vào Flash NVS (Chỉ gọi khi STM32 báo nạp thành công!)
 */
void save_installed_version(void) 
{
  if (new_version.length() > 0) {
    prefs.begin("ota_info", false);
    prefs.putString("ver", new_version);
    prefs.end();
    
    current_version = new_version;
    Serial.printf("[NVS] Saved new Version to Flash: %s\n", current_version.c_str());
  }
}

#endif // DOWNLOAD_FILE_H