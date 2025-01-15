#include <WiFi.h>
#include <ESPAsyncWebServer.h>  // Library untuk web server
#include <ArduinoJson.h>        // Library untuk JSON (jika diperlukan untuk komunikasi)
#include <Hash.h>               // Library hashing, ganti sesuai algoritma mining

// Informasi Wi-Fi
const char* defaultSSID = "12Miner";
const char* defaultPassword = "120131";
const char* onlineSSID = "ROBITHOH"; // Jaringan online untuk mining pool
const char* onlinePassword = "Salamulloh";

// Informasi Pool Mining
#define POOL_HOST "pool.minexmr.com"
#define POOL_PORT 4444
#define WALLET_ADDRESS "43dwwbGimKVZMxx65GNtZwaHMyUEtNDbWV3menbYhG82M3CDnHiNZbbPojRoDggEijTJruACrkzMUBDkTFQQjxT5LKjMEnU"

// Variabel untuk status mining
unsigned long hashCount = 0;
unsigned long successfulSubmissions = 0;

// Setup server web
AsyncWebServer server(80);

// Wi-Fi client untuk jaringan online
WiFiClient client;

// Fungsi untuk menghubungkan ke pool mining
void connectToPool() {
  if (client.connect(POOL_HOST, POOL_PORT)) {
    Serial.println("Connected to pool!");
    String loginData = "{\"jsonrpc\":\"2.0\",\"method\":\"login\",\"params\":{\"login\":\"";
    loginData += WALLET_ADDRESS;
    loginData += "\"},\"id\":1}\n";
    client.print(loginData);
  } else {
    Serial.println("Connection failed");
  }
}

// Fungsi untuk melakukan mining (sederhana contoh hashing)
void mine() {
  // Simulasikan perhitungan hash
  for (unsigned long i = 0; i < 1000; i++) {
    hashCount++;
    // Proses hashing atau algoritma mining Anda di sini (misalnya, SHA256)
    // hashResult = SHA256(i);
    // Kirim hasilnya ke pool mining jika diperlukan
  }
}

// Fungsi untuk menangani server web
void setupWebServer() {
  // Halaman utama untuk monitoring
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = "<html><body>";
    html += "<h1>ESP32 Mining Monitor</h1>";
    html += "<p>Status: Connected to Pool</p>";
    html += "<p>Hash Rate: " + String(hashCount) + " hashes</p>";
    html += "<p>Successful Submissions: " + String(successfulSubmissions) + "</p>";
    html += "<h3>Mining Pool Information</h3>";
    html += "<p>Pool Host: " + String(POOL_HOST) + "</p>";
    html += "<p>Pool Port: " + String(POOL_PORT) + "</p>";
    html += "<p>Wallet Address: " + String(WALLET_ADDRESS) + "</p>";
    
    // Form untuk mengubah Wi-Fi SSID dan Password
    html += "<h3>Wi-Fi Settings</h3>";
    html += "<form action='/set_wifi' method='POST'>";
    html += "<label for='ssid'>SSID:</label><input type='text' id='ssid' name='ssid' value='" + String(defaultSSID) + "'><br>";
    html += "<label for='password'>Password:</label><input type='text' id='password' name='password' value='" + String(defaultPassword) + "'><br>";
    html += "<input type='submit' value='Save Wi-Fi Settings'>";
    html += "</form>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  // Endpoint untuk mengubah Wi-Fi SSID dan Password
  server.on("/set_wifi", HTTP_POST, [](AsyncWebServerRequest *request){
    String newSSID;
    String newPassword;

    if (request->hasParam("ssid", true)) {
      newSSID = request->getParam("ssid", true)->value();
    }
    if (request->hasParam("password", true)) {
      newPassword = request->getParam("password", true)->value();
    }

    // Simpan SSID dan password baru, lalu restart ESP32 untuk menghubungkan ulang
    defaultSSID = newSSID.c_str();
    defaultPassword = newPassword.c_str();
    
    WiFi.begin(defaultSSID, defaultPassword);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    Serial.println("Wi-Fi reconnected");
    request->send(200, "text/html", "<h2>Wi-Fi Settings Updated! Restarting...</h2>");
    delay(3000);
    ESP.restart();
  });

  // Mulai server
  server.begin();
}

void setup() {
  // Inisialisasi Serial Monitor
  Serial.begin(115200);
  delay(10);

  // Hubungkan ke Wi-Fi (Access Point untuk landing page)
  WiFi.softAP(defaultSSID, defaultPassword);
  Serial.print("Access Point Started. IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Hubungkan ke Wi-Fi jaringan online untuk mining
  WiFi.begin(onlineSSID, onlinePassword);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to Wi-Fi Network");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Setup web server
  setupWebServer();

  // Connect ke pool mining
  connectToPool();
}

void loop() {
  // Lakukan proses mining
  mine();
  
  // Jika Anda ingin mengirimkan hasil hashing atau pekerjaan ke pool, lakukan di sini
  // Kirim hasil pekerjaan mining ke pool setiap kali hash selesai
  // Misalnya: client.print(hashResult);

  delay(1000);  // Sesuaikan delay sesuai dengan kebutuhan performa
}
