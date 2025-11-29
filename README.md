# 💧 Sistem Irigasi Cerdas Otomatis (ESP32 & Blynk)

Proyek ini mengimplementasikan sistem irigasi otomatis yang menggunakan **ESP32** untuk memantau kelembaban tanah, suhu, dan kelembaban udara secara *real-time*. Sistem ini dikendalikan dari jarak jauh melalui platform **IoT Blynk**, dan dilengkapi dengan layar **LCD I2C** untuk monitoring lokal.

Program ini dirancang untuk menggunakan **timer asinkron (BlynkTimer)** agar koneksi ke Blynk tetap stabil dan responsif, tanpa terhambat oleh proses pembacaan sensor.

---

## 🛠️ Komponen Hardware yang Dibutuhkan

Berikut adalah daftar komponen hardware utama yang digunakan dalam proyek ini:

| Komponen | Deskripsi | Pin ESP32 (Dalam Program) |
| :--- | :--- | :--- |
| **Microcontroller** | ESP32 DEVKIT V1 (atau sejenisnya) | - |
| **Sensor Kelembaban Tanah** | Sensor Kelembaban Tanah Analog | `GPIO 34` (`SOIL_PIN`) |
| **Sensor Suhu & Kelembaban** | DHT11 (atau DHT22) | `GPIO 4` (`DHT_PIN`) |
| **Modul Relay 1 Channel** | Untuk mengontrol Pompa Air / Solenoid Valve | `GPIO 18` (`RELAY_PIN`) |
| **Layar Tampilan** | LCD I2C 16x2 | `SDA (GPIO 21)`, `SCL (GPIO 22)` (I2C) |
| **Catu Daya** | Power Supply 5V/7V - 12V | - |



---

## ⚙️ Panduan Pengaturan (Software)

### 1. Prasyarat

* Pastikan Anda telah menginstal **IDE Arduino** dan **Board ESP32**.

### 2. Instalasi Library

Instal library berikut melalui **Arduino Library Manager** (`Sketch > Include Library > Manage Libraries...`):

* **Blynk** (Versi terbaru)
* **DHT sensor library** (oleh Adafruit)
* **Adafruit Unified Sensor** (Prasyarat untuk DHT)
* **LiquidCrystal I2C**

### 3. Pengaturan Blynk

Lakukan langkah-langkah berikut di platform **Blynk** (menggunakan Blynk IoT atau Legacy):

1.  Buat akun di platform Blynk.
2.  Buat **Template Proyek** baru dan catat `BLYNK_TEMPLATE_ID` dan `BLYNK_TEMPLATE_NAME`.
3.  Buat perangkat baru dan catat `BLYNK_AUTH_TOKEN`.
4.  Konfigurasikan **Virtual Pin** di Aplikasi Blynk:

| Pin Virtual | Tipe Widget | Deskripsi |
| :--- | :--- | :--- |
| **V0** (`VP_RELAY`) | Tombol (Switch) & LED (Status Pompa) | Status Pompa & Kontrol Manual |
| **V1** (`VP_TEMP`) | Gauge / Value Display | Menampilkan Suhu (°C) |
| **V2** (`VP_HUMI`) | Gauge / Value Display | Menampilkan Kelembaban Udara (%) |
| **V3** (`VP_SOIL`) | Gauge / Value Display | Menampilkan Kelembaban Tanah (%) |
| **V4** (`VP_SOIL_MIN`) | Slider | Batas Bawah Kelembaban (Misalnya: 20-40) |
| **V5** (`VP_SOIL_MAX`) | Slider | Batas Atas Kelembaban (Misalnya: 60-80) |
| **V6** (`VP_MODE`) | Tombol (Switch) | Pilih Mode: 0=Otomatis, 1=Manual |

### 4. Konfigurasi Kode

Ubah nilai-nilai berikut di bagian awal file `PBL_4_AIS.ino`:

1.  **Kredensial Blynk**:
    ```cpp
    #define BLYNK_TEMPLATE_ID "GANTI_DENGAN_ID_ANDA"
    #define BLYNK_TEMPLATE_NAME "GANTI_DENGAN_NAMA_ANDA"
    #define BLYNK_AUTH_TOKEN "GANTI_DENGAN_TOKEN_ANDA"
    ```
2.  **Kredensial WiFi**:
    ```cpp
    char ssid[] = "Nama_WiFi_Anda";
    char pass[] = "Password_WiFi_Anda";
    ```
3.  **(Opsional) Alamat I2C LCD**: Jika LCD Anda tidak menggunakan alamat `0x27`, ubah baris ini:
    ```cpp
    LiquidCrystal_I2C lcd(0x27, 16, 2);
    ```

---

## 🧠 Cara Kerja Program (Logika Kontrol)

Program ini menggunakan **Blynk Timer** untuk menjalankan tugas secara periodik tanpa memblokir koneksi utama.

### 1. Mode Otomatis (`modeControl == 0`)

* **Pompa MENYALA**: Jika **Kelembaban Tanah** (`VP_SOIL`) **≤ Batas Minimum** (`soilMin` di V4), `relayState` diatur ke **ON (1)**. Pompa diatur ke **LOW** (karena Active-LOW).
* **Pompa MATI**: Jika **Kelembaban Tanah** (`VP_SOIL`) **≥ Batas Maksimum** (`soilMax` di V5), `relayState` diatur ke **OFF (0)**. Pompa diatur ke **HIGH**.
* Jika kelembaban berada di antara batas minimum dan maksimum, status pompa saat ini dipertahankan.

### 2. Mode Manual (`modeControl == 1`)

* Perintah dari tombol **V0** (`VP_RELAY`) di Aplikasi Blynk langsung mengontrol Relay.
* Logika otomatis diabaikan.

### 3. Stabilitas & Sinkronisasi

* **BlynkTimer**: Memastikan pembacaan sensor (setiap 5 detik) dan komunikasi Blynk tidak saling mengganggu.
* **BLYNK_CONNECTED()**: Saat ESP32 terhubung, ia secara otomatis mengambil nilai terakhir dari Batas Kelembaban (V4, V5) dan Mode Kontrol (V6) dari server Blynk, memastikan pengaturan pengguna tidak hilang saat perangkat restart.
* **Penanganan Error DHT**: Jika sensor DHT gagal membaca, sistem akan mengirimkan `Blynk.logEvent("dht_error", ...)` untuk memberi tahu pengguna di aplikasi, sambil tetap menjalankan fungsi utama lainnya.
