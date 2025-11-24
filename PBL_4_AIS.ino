// ====================================================================================
// --- KONFIGURASI BLYNK DAN WIFI ---
// ====================================================================================

// ID unik untuk template Blynk yang Anda buat. Ganti dengan ID template Anda.
#define BLYNK_TEMPLATE_ID "TMPL634idmXKG"
// Nama template (judul proyek) di Blynk. Ganti dengan nama template Anda.
#define BLYNK_TEMPLATE_NAME "Smart Farming"
// Token autentikasi yang diberikan oleh Blynk untuk menghubungkan perangkat ini. Ganti dengan token Anda.
#define BLYNK_AUTH_TOKEN "-NmSuP-cikIhTHBaFylhw3KisFvUPCV7"

// Memuat library penting untuk koneksi WiFi dan Blynk pada ESP32
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// Memuat library untuk komunikasi I2C (diperlukan untuk LCD)
#include <Wire.h> 
// Memuat library untuk LCD I2C 16x2
#include <LiquidCrystal_I2C.h> 
// Memuat library untuk sensor Suhu dan Kelembaban (DHT11)
#include <DHT.h> 

// ====================================================================================
// --- DEFINISI PIN HARDWARE DAN VARIABEL GLOBAL ---
// ====================================================================================

#define DHTPIN 4        // Pin GPIO 4 -> Sensor DHT11 (Suhu & Kelembaban)
#define DHTTYPE DHT11   // Jenis sensor: DHT11
#define SOIL_PIN 34     // Pin GPIO 34 (Analog) -> Sensor Kelembaban Tanah
#define RELAY_PIN 18    // Pin GPIO 18 -> Modul Relay

// Inisialisasi Perangkat Keras
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);
BlynkTimer timer; // Membuat objek timer dari BlynkTimer

// Kredensial WiFi (Ganti dengan data Anda!)
char ssid[] = "POCO F3"; 
char pass[] = "poco1234"; 

// --- DEFINISI VIRTUAL PIN BLYNK ---
#define VP_RELAY V0       // V0: Kontrol Pompa (Tombol/LED)
#define VP_TEMP V1        // V1: Suhu
#define VP_HUMI V2        // V2: Kelembaban Udara
#define VP_SOIL V3        // V3: Kelembaban Tanah
#define VP_SOIL_MIN V4    // V4: Batas Bawah Kelembaban Tanah
#define VP_SOIL_MAX V5    // V5: Batas Atas Kelembaban Tanah
#define VP_MODE V6        // V6: Mode Kontrol (0=Otomatis, 1=Manual)

// Variabel Status
int modeControl = 0; 
int relayState = 0; // 0 = OFF, 1 = ON
int soilMin= 30; 
int soilMax = 70;

// ====================================================================================
// --- FUNGSI UTAMA BLYNK: SINKRONISASI DAN KONTROL ---
// ====================================================================================

// FUNGSI PENTING: Dipanggil HANYA SEKALI setelah koneksi ke Blynk sukses.
// Bertugas mengambil nilai terakhir dari Virtual Pin di server Blynk.
BLYNK_CONNECTED() {
  // Meminta Blynk untuk mengirimkan nilai terbaru dari pin V4, V5, V6
  Blynk.syncVirtual(VP_SOIL_MIN, VP_SOIL_MAX, VP_MODE);
  // Catatan: V0 (Relay) tidak perlu di-sync karena akan diperbarui oleh logika di bawah.

  // Tampilkan status koneksi di LCD
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Blynk CONNECTED");
  lcd.setCursor(0,1);
  lcd.print("Mode: Otomatis"); // Asumsi default, akan diupdate oleh sync V6
  delay(2000); // Tampilkan sebentar
}

// ------------------ KONTROL MODE OTOMATIS/MANUAL (VP_MODE) ----------------------
BLYNK_WRITE(VP_MODE) {
  modeControl = param.asInt();
  if (modeControl == 0) {
    relayState = 0;
    digitalWrite(RELAY_PIN, HIGH); // Matikan pompa saat masuk mode Otomatis
    Blynk.virtualWrite(VP_RELAY, relayState); 
  }
}

// ------------------ KONTROL MANUAL POMPA (VP_RELAY) ----------------------
BLYNK_WRITE(VP_RELAY) {
  // Kontrol manual hanya diizinkan jika modeControl adalah 1
  if (modeControl == 1) {
    relayState = param.asInt();
    // RELAY ACTIVE-LOW: 1 (ON) -> LOW, 0 (OFF) -> HIGH
    digitalWrite(RELAY_PIN, relayState == 1 ? LOW : HIGH);
    } else {
      // Jika mode Otomatis, kembalikan tombol di Blynk ke status saat ini (misalnya OFF)
      Blynk.virtualWrite(VP_RELAY, relayState);
    }
}

// -------------------- PENYIMPANAN BATAS SOIL ------------------------
BLYNK_WRITE(VP_SOIL_MIN) {
  soilMin = param.asInt();
}

BLYNK_WRITE(VP_SOIL_MAX) {
  soilMax = param.asInt();
}

// ====================================================================================
// --- FUNGSI TIMER: AMBIL DATA SENSOR DAN UPDATE BLYNK/LCD ---
// ====================================================================================
void sendSensorDataAndControl() {
    // -------------------- BACA SENSOR KELEMBABAN TANAH -------------------
    int soilRaw = analogRead(SOIL_PIN); 
    int soilPercent = map(soilRaw, 4095, 0, 0, 100); // 4095=Kering, 0=Basah

    // -------------------- BACA SENSOR SUHU DAN KELEMBABAN UDARA (DHT11) ------------------
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    // Cek error DHT
    if (isnan(h) || isnan(t)) {
      // Kirim notifikasi error ke Blynk
      Blynk.logEvent("dht_error", "Sensor DHT11 GAGAL BACA! Cek koneksi.");
      // Tampilkan error di LCD
      lcd.setCursor(0,0);
      lcd.print("Sensor DHT Error");
      lcd.setCursor(0,1);
      lcd.print("Skipping updates");
      return; // Hentikan fungsi jika error
      }
    
    // -------------------- LOGIKA MODE OTOMATIS -------------------
    if (modeControl == 0 ) {
      // Jika mode Otomatis, terapkan logika batas
      if (soilPercent <= soilMin) {
        relayState = 1; // Pompa ON
      } else if (soilPercent >= soilMax) {
        relayState = 0; // Pompa OFF
      }
      // Update status relay secara fisik dan di Blynk
      digitalWrite(RELAY_PIN, relayState == 1 ? LOW : HIGH); 
      Blynk.virtualWrite(VP_RELAY, relayState); 
    }

    // -------------------- KIRIM DATA KE BLYNK -----------------
    Blynk.virtualWrite(VP_SOIL, soilPercent);
    Blynk.virtualWrite(VP_TEMP, t);
    Blynk.virtualWrite(VP_HUMI, h);

    // -------------------- TAMPILAN LCD DISPLAY ---------------------
    lcd.clear();
    // Baris 1: Suhu dan Kelembaban Udara
    lcd.setCursor(0,0);
    lcd.print("T:");
    lcd.print(t, 0);
    lcd.print("C H:");
    lcd.print(h, 0);
    lcd.print("% ");

    // Baris 2: Kelembaban Tanah
    lcd.setCursor(0,1);
    lcd.print("Soil:");
    lcd.print(soilPercent);
    lcd.print("% ");
}


// ====================================================================================
// --- FUNGSI SETUP (JALAN HANYA SEKALI SAAT ESP32 DIHIDUPKAN) ---
// ====================================================================================
void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Pompa OFF saat startup (Active-LOW)

  lcd.init();
  lcd.backlight();
  dht.begin();

  // Tampilkan proses koneksi di LCD
  lcd.setCursor(0,0);
  lcd.print("Connecting WiFi");
  lcd.setCursor(0,1);
  lcd.print(ssid);

  // Mulai koneksi ke Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Inisialisasi Timer: Panggil fungsi 'sendSensorDataAndControl' setiap 2000 ms (2 detik)
  // Pembacaan sensor DHT akan stabil pada interval ini
  timer.setInterval(2000L, sendSensorDataAndControl);
}

// ====================================================================================
// --- FUNGSI LOOP (JALAN TERUS MENERUS) ---
// ====================================================================================
void loop() {
  // Wajib: Menjalankan komunikasi Blynk
  Blynk.run(); 
  // Wajib: Menjalankan timer yang mengatur jadwal pembacaan sensor
  timer.run(); 
  // Loop sekarang sangat ringan dan efisien, tanpa delay()
}