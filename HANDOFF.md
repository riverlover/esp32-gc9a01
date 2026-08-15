# Handoff — ESP32-C3 Super Mini + GC9A01

> 交接文档。下一会话可直接按本文继续，不必重走摸索过程。

## 当前状态（已验证）

| 项 | 状态 |
|----|------|
| 硬件接线 | 已接好并点亮 |
| macOS 识别 | Espressif USB JTAG/serial，无需额外驱动 |
| 工具链 | PlatformIO Core 6.1.19（`~/.platformio/penv/bin`） |
| 固件 | 模块化表盘；**默认 Calendar(4)**；手机热点 `WatchESP` + NTP 已通 |
| 工程路径 | `/Users/lizhenhe/vscode/esp32-GC9A01` |

## 硬件

- **MCU**：ESP32-C3 Super Mini（USB-C，板标 V1601 / Super Mini）
- **屏**：1.28" TFT 240×240，驱动 IC **GC9A01**，7 针排针
- **连接**：杜邦线母对母

### 已采用引脚（与 `src/pins.h` 一致）

| 屏引脚 | ESP32-C3 | 杜邦线颜色 |
|--------|----------|------------|
| VCC | **3.3**（勿接 5V） | 红 |
| GND | **G** | 黑 |
| SCL | **6** | 橙 |
| SDA | **7** | 黄 |
| DC | **2** | 绿 |
| CS | **10** | 蓝 |
| RST | **3** | 紫 |

说明：信号脚可改，但必须与代码宏一致。尽量避开 GPIO 8/9（启动相关）。

### 屏 PCB 备注

板上有「此模块可不接 CS、RST」字样；本工程仍接全，驱动更省事。

## 软件栈

- Platform：`espressif32`（当前 7.0.1）→ Arduino-ESP32 **2.0.17**
- Board：`esp32-c3-devkitm-1`，`board_build.partitions = huge_app.csv`
- Framework：Arduino
- 库：`GFX Library for Arduino@1.4.7`、`QRCode@0.0.1`  
  - **不要升 GFX 到 1.5.x**（需 Arduino-ESP32 3.x）
- USB CDC：`ARDUINO_USB_MODE=1` + `ARDUINO_USB_CDC_ON_BOOT=1`

## 工程结构

```
src/main.cpp                 # 编排 / 串口命令
src/config.h                 # 默认；DEFAULT_FACE=Calendar
src/config.local.h           # gitignore：Wi‑Fi 与可选覆盖
src/config.local.h.example
src/pins.h
src/time/TimeService.*       # NTP / Soft / Manual
src/wifi/WifiProvision.*     # STA 连接（8.5dBm、关省电、多 mode 重试）
src/ui/ProvQr.*              # 配网提示 / 二维码（历史 SoftAP 用）
src/face/                    # Classic / Lume / Skeleton / Calendar
```

## 表盘

| 键 | 风格 | 说明 |
|----|------|------|
| 1 | Classic | 经典指针 |
| 2 | Lume | 夜光 |
| 3 | Skeleton | 镂空 |
| **4** | **Calendar** | **日历窗（当前默认）** |

- 编译期：`#define DEFAULT_FACE FaceId::Calendar`
- 运行期：串口 `1`–`4` / `n`

## Wi‑Fi / NTP（已验证路径）

**成功组合**：手机个人热点 **`WatchESP` / `12345678`（WPA2）** → ESP STA → NTP。

```text
Wi-Fi OK → NTP OK → Watch OK [NTP] Calendar …
```

RF 调参（`WifiProvision.cpp`）：`WiFi.setTxPower(WIFI_POWER_8_5dBm)` + `WiFi.setSleep(WIFI_PS_NONE)`。

本地配置（勿提交）：

```bash
# src/config.local.h
#define WIFI_SSID "WatchESP"
#define WIFI_PASS "12345678"
#define DEFAULT_FACE FaceId::Calendar
```

串口辅助：`w SSID PASS` · `s` 跳过 Wi‑Fi · `t YYYY-MM-DD HH:MM:SS` 手动校时 · `p` 重新进热点等待。

### 踩坑（Wi‑Fi）

1. `AUTH_EXPIRE(2)` / `4WAY_HANDSHAKE_TIMEOUT(15)` **≠ 一定密码错**；C3 Super Mini 上极常见（射频/功率/天线/双频路由）。
2. 家宽 `TP-LINK_D6B1` 双频合一多次握手失败；小米 IoT 能连不代表 ESP 能连。
3. ESP SoftAP / BLE 配网在本板不可靠（SoftAP 难被手机发现；BLE WiFiProv 曾 `abort`）。
4. 华为连 ESP 热点常失败；反向「手机开热点、ESP 去连」更稳。
5. 社区常见修复：降功率 8.5dBm、关 sleep、专用 2.4G WPA2 SSID、拔外设空载对比供电。

## 常用命令

```bash
ls /dev/cu.usbmodem*
export PATH="$HOME/.platformio/penv/bin:$PATH"
export http_proxy=http://127.0.0.1:7890 https_proxy=http://127.0.0.1:7890
cd /Users/lizhenhe/vscode/esp32-GC9A01
pio run -t upload
# 串口：python 读 /dev/cu.usbmodem* 115200，或 pio device monitor
```

## 后续可做

- 按键切面 / 断网保时（RTC）
- 家宽单独 2.4G IoT SSID 再试 STA
- 拔屏空载对比供电对握手的影响
- Arduino_GFX 1.5+ 需换 pioarduino（ESP32 core 3.x）

## 其他踩坑

1. macOS 对 C3 原生 USB 一般免驱。  
2. 下载走代理 `7890`。  
3. 屏供电只用 **3.3V**。  
4. Wi‑Fi 密码只放 `config.local.h`。
