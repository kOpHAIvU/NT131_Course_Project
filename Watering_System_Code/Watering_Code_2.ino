#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid = "KoPhaiVu";
const char* password = "123456789";

// Khởi tạo server HTTP
ESP8266WebServer server(80); // Port 80

// Pin cảm biến và relay
int soilMoisturePin = A0;
int relayPin = D1;
bool pumpState = false; // Trạng thái máy bơm

// Ngưỡng độ ẩm
int moistureLowerThreshold = 15;
int moistureUpperThreshold = 50;

// Kết nối Wi-Fi
void setup() {
  Serial.begin(115200);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW); // Tắt máy bơm ban đầu

  // Kết nối Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("ESP8266 IP Address: ");
  Serial.println(WiFi.localIP()); // In ra địa chỉ IP của ESP8266

  // Đặt các route HTTP
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/control", HTTP_POST, handleControl);
  server.on("/settings", HTTP_POST, handleSettings);

  // Bắt đầu server
  server.begin();
  Serial.println("Server started");
}

void loop() {
  // Xử lý các yêu cầu HTTP
  server.handleClient();

  // Đọc giá trị độ ẩm và điều khiển máy bơm
  int moistureValue = analogRead(soilMoisturePin);
  float moisturePercentage = mapToPercentage(moistureValue);

  if (moisturePercentage < moistureLowerThreshold && !pumpState) {
    digitalWrite(relayPin, HIGH); // Bật máy bơm
    pumpState = true;
  } else if (moisturePercentage >= moistureUpperThreshold && pumpState) {
    digitalWrite(relayPin, LOW); // Tắt máy bơm
    pumpState = false;
  }

  // In trạng thái độ ẩm và máy bơm ra Serial Monitor
  Serial.print("Moisture: ");
  Serial.print(moisturePercentage);
  Serial.print("%, Pump: ");
  Serial.println(pumpState ? "ON" : "OFF");

  delay(2000); // Kiểm tra và in trạng thái mỗi 2 giây
}

// Hàm trả về trạng thái máy bơm và độ ẩm
void handleStatus() {
  // Set CORS headers
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  
  int moistureValue = analogRead(soilMoisturePin);
  float moisturePercentage = mapToPercentage(moistureValue);

  String jsonResponse = "{\"moisture\": " + String(moisturePercentage) +
                        ", \"pump\": " + (pumpState ? "true" : "false") + "}";

  server.send(200, "application/json", jsonResponse);
}

// Hàm điều khiển máy bơm (bật/tắt)
void handleControl() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");

  String action = server.arg("action"); // action sẽ là "on" hoặc "off"
  
  if (action == "on" && !pumpState) {
    digitalWrite(relayPin, HIGH); // Bật máy bơm
    pumpState = true;
    Serial.println("Pump turned ON");  // In ra thông báo khi máy bơm được bật
  } else if (action == "off" && pumpState) {
    digitalWrite(relayPin, LOW); // Tắt máy bơm
    pumpState = false;
    Serial.println("Pump turned OFF");  // In ra thông báo khi máy bơm được tắt
  }
  
  server.send(200, "text/plain", "Action performed: " + action);
}

// Hàm cập nhật ngưỡng độ ẩm
void handleSettings() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");

  if (server.hasArg("lower") && server.hasArg("upper")) {
    moistureLowerThreshold = server.arg("lower").toInt();
    moistureUpperThreshold = server.arg("upper").toInt();
    
    String response = "Updated thresholds: Lower = " + String(moistureLowerThreshold) + 
                      ", Upper = " + String(moistureUpperThreshold);

    // Log ra khi cập nhật ngưỡng độ ẩm
    Serial.print("Moisture Thresholds updated: ");
    Serial.print("Lower = ");
    Serial.print(moistureLowerThreshold);
    Serial.print(", Upper = ");
    Serial.println(moistureUpperThreshold);

    server.send(200, "text/plain", response);
  } else {
    server.send(400, "text/plain", "Missing parameters");
  }
}

// Hàm chuyển đổi giá trị độ ẩm thành phần trăm
float mapToPercentage(int moistureValue) {
  float percentage = (moistureValue / 1023.0) * 100.0;
  
  // Giới hạn phần trăm trong ngưỡng
  if (percentage < moistureLowerThreshold) {
    percentage = moistureLowerThreshold;
  }
  if (percentage > moistureUpperThreshold) {
    percentage = moistureUpperThreshold;
  }

  return percentage;
}
