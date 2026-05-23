# ESP32-RC-Joystick Controller

Firmware cho ESP32 để kết nối tay cầm PlayStation với RC receiver (CRSF/SBUS/PPM).

## Tính năng

### Kết nối tay cầm
- Hỗ trợ PS2, PS3, PS4, PS5 Controller qua Bluetooth
- Tự động tìm và kết nối
- Tự động reconnect khi mất kết nối
- Hiển thị trạng thái pin
- Hiển thị cường độ tín hiệu RSSI

### Giao thức RC Output
- **CRSF (Crossfire)**: Tốc độ cao, hỗ trợ lên đến 24 kênh
- **SBUS**: Tương thích Futaba
- **PPM**: Pulse Position Modulation
- Có thể bật đồng thời nhiều giao thức

### Channel Mapping
- Hệ thống mapping linh hoạt cho 24 kênh
- Đầu vào: Analog sticks, buttons, triggers, D-pad
- Có thể cấu hình:
  - Nguồn input
  - Kênh output
  - Reverse
  - Deadzone (0-100%)
  - Expo (0-100%)
  - Smoothing (0-100%)
  - Scale (0-200%)
  - Min/Max/Center values

### Web UI
- Dashboard với thông tin trạng thái
- Channel Monitor realtime
- Mapping Editor với drag-drop
- Protocol Configuration
- Calibration Tool
- System Settings
- OTA Update
- Backup/Restore config

## Yêu cầu phần cứng

### ESP32 Board
- ESP32-S3 (khuyến nghị)
- ESP32-C3
- ESP32 thường

### Kết nối
```
ESP32          | Chức năng
---------------|--------------
GPIO 43        | CRSF TX (mặc định)
GPIO 44        | CRSF RX (mặc định)
GPIO 39        | SBUS TX (mặc định)
GPIO 38        | SBUS RX (mặc định)
GPIO 10        | PPM Out (mặc định)
GPIO 2         | Status LED
GPIO 3         | Connected LED
GPIO 0         | Config Button
```

## Cài đặt

### 1. Cài PlatformIO

```bash
# Cài qua pip
pip install platformio

# Hoặc cài VSCode Extension
# PlatformIO IDE for VSCode
```

### 2. Clone/Download project

```bash
cd "d:\app\joystick ESP32"
```

### 3. Cài thư viện

PlatformIO sẽ tự động cài các thư viện từ `platformio.ini`:

- Bluepad32 (PS Controller)
- ESPAsyncWebServer
- AsyncTCP
- ArduinoJson

### 4. Build firmware

```bash
# Build cho ESP32-S3 (mặc định)
pio run

# Build cho ESP32-C3
pio run -e esp32-c3-devkitc-1

# Build cho ESP32 thường
pio run -e esp32dev
```

### 5. Upload firmware

```bash
# Upload qua USB
pio run --target upload

# Upload với monitor
pio run --target upload --monitor
```

## Cấu hình WiFi

### AP Mode (Mặc định)
Khi không có WiFi được cấu hình, ESP32 sẽ tạo Access Point:
- SSID: `ESP32-RC-Joystick-XXXXXX`
- Password: (không có)
- IP: `192.168.4.1`

### Station Mode
Kết nối ESP32 với WiFi có sẵn:
1. Kết nối ESP32 qua AP
2. Mở Web UI: `http://192.168.4.1`
3. Vào tab System > WiFi Configuration
4. Nhập SSID và Password
5. Click Connect

## Sử dụng Web UI

### Dashboard
Xem trạng thái tổng quan:
- Trạng thái kết nối controller
- Mức pin
- RSSI
- System resources
- Protocol status

### Channel Monitor
Theo dõi giá trị kênh realtime:
- 8/12/16/24 kênh tùy chọn
- Thanh trượt animation
- Virtual joystick display

### Mapping
Cấu hình channel mapping:
1. Chọn channel để chỉnh
2. Chọn input source
3. Điều chỉnh các thông số
4. Click Save hoặc đợi auto-save

### Output
Cấu hình protocol:
1. Chọn protocol (CRSF/SBUS/PPM)
2. Điều chỉnh baudrate, frame rate
3. Chọn pins
4. Enable/Disable

### Calibration
Calibrate joystick:
1. Click "Calibrate Center" với sticks ở vị trí trung tâm
2. Click "Detect Min/Max" và di chuyển sticks
3. Test calibration
4. Save

### System
- Xem thông tin hệ thống
- WiFi configuration
- Backup/Restore config
- Factory Reset
- OTA Update

## Kết nối RC Receiver

### CRSF (Crossfire)
```
ESP32 GPIO 43 (TX) ----> CRSF RX
ESP32 GPIO 44 (RX) ----> CRSF TX (optional)
GND -------------------> GND
```

### SBUS
```
ESP32 GPIO 39 (TX) ----> SBUS RX
ESP32 GPIO 38 (RX) ----> SBUS TX (optional)
GND -------------------> GND
```

### PPM
```
ESP32 GPIO 10 -----> PPM Input trên Receiver
GND --------------> GND
```

## Tối ưu RAM và CPU

### Tips
1. Sử dụng `StaticJsonDocument` thay vì `DynamicJsonDocument`
2. Tắt debug logs khi production
3. Sử dụng `PROGMEM` cho strings lớn
4. Giới hạn WebSocket buffer size
5. Sử dụng `EVERY_N_MS` macros thay vì `delay()`

### Memory Management
- Default channel buffer: 24 channels
- JSON config buffer: 4KB
- WebSocket buffer: 1KB per client
- Task stack sizes được tối ưu cho mỗi task

## Troubleshooting

### Controller không kết nối
1. Kiểm tra Bluepad32 đã init chưa
2. Reset ESP32 và thử lại
3. Kiểm tra logs qua Serial

### Không thấy Web UI
1. Kiểm tra ESP32 đã connect WiFi chưa
2. Tìm IP của ESP32 qua Serial logs
3. Thử kết nối qua AP mode

### RC Output không hoạt động
1. Kiểm tra đúng pin đã cấu hình
2. Kiểm tra protocol đã enable chưa
3. Verify baudrate đúng với receiver

### Watchdog reset
1. Kiểm tra task stack overflow
2. Tối ưu code trong loop()
3. Kiểm tra Serial buffer overflow

## License

MIT License

## Credits

- Bluepad32 by Ricardo Quesada
- ESPAsyncWebServer by me-no-dev
