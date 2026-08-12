#ifndef DOWNLOAD_FILE_H
#define DOWNLOAD_FILE_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <ESP32_FTPClient.h>
#include <Preferences.h>

// ================= MACRO CẤU HÌNH HỆ THỐNG =================
#define WIFI_SSID           "202 H30"
#define WIFI_PASS           "68686868"

#define FTP_SERVER          "ftpupload.net"
#define FTP_USER            "if0_42484011"
#define FTP_PASS            "Sang03022004"
#define FTP_PATH            "/htdocs/Firmware"

#define FILE_VERSION_TARGET "Version.txt"
#define FILE_HEX_TARGET     "App.hex"

// Giảm bộ đệm RAM xuống 30KB (vừa đủ dung lượng file App.hex)
#define MAX_HEX_BUF_SIZE    (30 * 1024) 

// ================= BIẾN TOÀN CỤC =================
char file_hex[MAX_HEX_BUF_SIZE];
String current_version = "0.0.0"; 
String new_version = "";          

ESP32_FTPClient ftp(FTP_SERVER, FTP_USER, FTP_PASS);
Preferences prefs;

// ================= HÀM XỬ LÝ (FUNCTIONS) =====================

bool wifi_connect(void) 
{
  if (WiFi.status() == WL_CONNECTED) return true;

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("[FTP] Connecting Wifi...");
  uint8_t timeout = 0;
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (++timeout > 30) {
      Serial.println("\n[FTP] WiFi Connection Failed!");
      return false;
    }
  }
  
  Serial.println("\n[FTP] WiFi Connected!");
  Serial.print("[FTP] IP address: ");
  Serial.println(WiFi.localIP());
  return true;
}

String fetch_cloud_version(void) 
{
  ftp.OpenConnection();
  ftp.ChangeWorkDir(FTP_PATH);

  String version_str = "";
  ftp.InitFile("Type A");
  
  ftp.DownloadString(FILE_VERSION_TARGET, version_str);
  ftp.CloseConnection();

  version_str.trim();
  return version_str;
}

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
  file_hex[fileSize] = '\0'; 

  ftp.CloseConnection();
  Serial.println("[FTP] App.hex Downloaded successfully!");
  return true;
}

bool check_and_download_ota(void) 
{
  memset(file_hex, 0, sizeof(file_hex)); // Clear RAM buffer cũ trước khi check
  
  if (!wifi_connect()) return false;

  Serial.println("\n[OTA Check] Checking version.txt from Cloud...");
  new_version = fetch_cloud_version();
  
  Serial.printf("[OTA Check] Current Ver: %s | Cloud Ver: %s\n", current_version.c_str(), new_version.c_str());

  if (new_version.length() == 0 || new_version.equals(current_version)) {
    Serial.println("[OTA Check] Firmware is up to date. Skip downloading App.hex.");
    return false;
  }

  Serial.println("[OTA Check] New Version detected! Starting download...");
  return fetch_hex_file();
}

bool dowload_file_init(void) 
{
  memset(file_hex, 0, sizeof(file_hex));
  
  prefs.begin("ota_info", false);
  current_version = prefs.getString("ver", "0.0.0"); 
  prefs.end();
  
  Serial.printf("[Init] System started with Firmware Version: %s\n", current_version.c_str());

  return wifi_connect();
}

void dowload_file_handle(void) 
{
  // Không dùng timer 5 phút nữa
}

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