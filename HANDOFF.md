# Handoff — ESP32-C3 Super Mini + GC9A01

> 交接文档。下一会话可直接按本文继续，不必重走摸索过程。

## 当前状态（已验证）

| 项 | 状态 |
|----|------|
| 硬件接线 | 已接好并点亮 |
| macOS 识别 | Espressif USB JTAG/serial，无需额外驱动 |
| 工具链 | PlatformIO Core 6.1.19（`~/.platformio/penv/bin`） |
| 固件 | 模块化表盘；**默认 Photo(5)**；STA + NTP 已通；**EC11 切面已接** |
| Wi‑Fi | 热点 `WatchESP` 已通；家宽 `TP-LINK_D6B1` **空载（拔屏）已通** |
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

### EC11（与 `src/pins.h` 一致）

| EC11 | ESP32-C3 |
|------|----------|
| CLK | **0** |
| DT | **1** |
| SW | **5** |
| + | **3.3** |
| GND | **G** |

交互：旋转预览切面 → 短按确认（停转约 4s 自动确认）→ 长按预留设置。驱动：`src/input/Ec11.*`。

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
src/main.cpp                 # 编排 / 串口 / EC11
src/config.h                 # 默认；DEFAULT_FACE=Photo
src/config.local.h           # gitignore：Wi‑Fi 与可选覆盖
src/config.local.h.example
src/pins.h                   # 屏 + EC11
src/input/Ec11.*             # 旋钮正交解码 + 按键消抖
src/time/TimeService.*       # NTP / Soft / Manual
src/wifi/WifiProvision.*     # STA 连接（8.5dBm、关省电、多 mode 重试）
src/ui/ProvQr.*              # 配网提示 / 二维码（历史 SoftAP 用）
src/face/                    # Classic / Lume / Skeleton / Calendar / Photo
```

## 表盘

| 键 | 风格 | 说明 |
|----|------|------|
| 1 | Classic | 经典指针 |
| 2 | Lume | 夜光 |
| 3 | Skeleton | 镂空 |
| 4 | Calendar | 日历窗 |
| **5** | **Photo** | **照片背景（当前默认）** |

- 编译期：`#define DEFAULT_FACE FaceId::Photo`
- 运行期：串口 `1`–`5` / `n`，或 EC11 旋转/短按

## Wi‑Fi / NTP（已验证路径）

### A. 手机热点（接屏也稳）

**`WatchESP` / `12345678`（WPA2）** → ESP STA → NTP。

### B. 家宽空载（2026-08-15 验证）

拔掉 GC9A01（仅 USB 供电、无外接屏）后连 **`TP-LINK_D6B1`**：

| 项 | 结果 |
|----|------|
| 条件 | 空载、无杜邦线外设 |
| mode | **0（Plain）一次成功** |
| IP | `192.168.0.109` |
| RSSI | **-43 dBm** |
| NTP | OK → `Watch OK [NTP] Calendar` |
| 耗时 | 约数秒 |

密码等密钥只写在 `config.local.h`（gitignore），勿提交。

```text
Wi-Fi OK → NTP OK → Watch OK [NTP] Calendar …
```

**推论**：接屏时对该 SSID 的 `AUTH_EXPIRE` / `4WAY_HANDSHAKE_TIMEOUT` 更像是 **屏/杜邦线供电或 GPIO 干扰**，而非密码错或家宽完全不可用。有载对比尚未复测。

RF 调参（`WifiProvision.cpp`）：`WiFi.setTxPower(WIFI_POWER_8_5dBm)` + `WiFi.setSleep(WIFI_PS_NONE)`。

本地配置示例（勿提交）：

```bash
# src/config.local.h
#define WIFI_SSID "TP-LINK_D6B1"   # 或 WatchESP
#define WIFI_PASS "********"
#define DEFAULT_FACE FaceId::Calendar
```

串口辅助：`w SSID PASS` · `s` 跳过 Wi‑Fi · `t YYYY-MM-DD HH:MM:SS` 手动校时 · `p` 重新进热点等待。

### 踩坑（Wi‑Fi）

1. `AUTH_EXPIRE(2)` / `4WAY_HANDSHAKE_TIMEOUT(15)` **≠ 一定密码错**；C3 Super Mini 上极常见（射频/功率/天线/双频路由/**外设干扰**）。
2. 家宽 `TP-LINK_D6B1`：**空载可连**；接屏时曾多次握手失败。小米 IoT 能连不代表 ESP 接屏也能连。
3. ESP SoftAP / BLE 配网在本板不可靠（SoftAP 难被手机发现；BLE WiFiProv 曾 `abort`）。
4. 华为连 ESP 热点常失败；反向「手机开热点、ESP 去连」更稳。
5. 排障顺序：降功率 8.5dBm、关 sleep、专用 2.4G WPA2、**拔屏空载对比**（已证实有效）。

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

- EC11：预览缩小悬浮视觉、短按/长按屏上反馈、长按设置菜单
- Calendar / Photo 表盘专用旋钮交互；断网保时（RTC）
- **接回 GC9A01 有载复测** `TP-LINK_D6B1`（对照空载成功）
- 有载仍失败时：缩短杜邦线、加近端去耦、或家宽单独 2.4G IoT SSID
- Arduino_GFX 1.5+ 需换 pioarduino（ESP32 core 3.x）

## 其他踩坑

1. macOS 对 C3 原生 USB 一般免驱。  
2. 下载走代理 `7890`。  
3. 屏供电只用 **3.3V**。  
4. Wi‑Fi 密码只放 `config.local.h`。
