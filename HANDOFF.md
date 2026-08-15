# Handoff — ESP32-C3 Super Mini + GC9A01

> 交接文档。下一会话可直接按本文继续，不必重走摸索过程。

## 当前状态（已验证）

| 项 | 状态 |
|----|------|
| 硬件接线 | 已接好并点亮 |
| macOS 识别 | Espressif USB JTAG/serial，无需额外驱动 |
| 工具链 | PlatformIO Core 6.1.19（`~/.platformio/penv/bin`） |
| 固件 | **模块化多风格表盘**；Wi‑Fi NTP（未配 SSID 则软时钟） |
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

- Platform：`espressif32`（当前 7.0.1）
- Board：`esp32-c3-devkitm-1`
- Framework：Arduino
- 库：`moononournation/GFX Library for Arduino@1.4.7`  
  - **不要随意升到 1.5.x**：会依赖 `esp32-hal-periman.h`（Arduino-ESP32 3.x），与当前 PlatformIO 自带的 2.x core 不兼容。
- USB CDC：`ARDUINO_USB_MODE=1` + `ARDUINO_USB_CDC_ON_BOOT=1`

## 当前固件：模块化表盘

### 结构

```
src/main.cpp              # 编排
src/config.h              # 默认；本地覆盖见下
src/config.local.h.example
src/pins.h
src/time/TimeService.*    # NTP / Soft
src/face/IWatchFace.h     # 接口
src/face/Face*.*          # Classic / Lume / Skeleton / Calendar
src/face/FaceRegistry.*
src/face/gfx_util.h
```

### Wi‑Fi NTP

```bash
cp src/config.local.h.example src/config.local.h
# 填写 WIFI_SSID / WIFI_PASS
```

- `config.local.h` 已 gitignore，勿提交密码。
- 时区默认 UTC+8（`NTP_TZ_OFFSET_SEC`）。
- 未配置或联网失败 → 软时钟（`SOFT_START_*`，默认 10:08:00）。

### 切换风格

- 编译期：`#define DEFAULT_FACE FaceId::Lume`（写在 `config.local.h`）
- 运行期串口：`1` Classic · `2` Lume · `3` Skeleton · `4` Calendar · `n` 下一个

串口示例：`Watch OK [NTP] Classic 20:44:01` 或 `[Soft]`。

## 常用命令

```bash
ls /dev/cu.usbmodem*
export PATH="$HOME/.platformio/penv/bin:$PATH"
export http_proxy=http://127.0.0.1:7890 https_proxy=http://127.0.0.1:7890
export ALL_PROXY=socks5://127.0.0.1:7890
cd /Users/lizhenhe/vscode/esp32-GC9A01
pio run -t upload
pio device monitor -b 115200
```

## 后续可做

- 开机轮播多风格 / 按键切面（省串口）
- 外接 RTC，断网保时
- 固定 `upload_port` 自动探测
- 若要用 Arduino_GFX 1.5+，需换支持 Arduino-ESP32 3.x 的 platform（如 pioarduino）

## 踩坑摘要

1. macOS 对 C3 原生 USB **一般不需驱动**；设备名形如 `USB JTAG/serial debug unit`。
2. 下载慢时务必走系统代理 `7890`。
3. GFX `@^1.5.0` 会编译失败 → 钉死 `1.4.7`。
4. 供电只用 **3.3V**，不要接板子 **5V** 脚给屏。
5. Wi‑Fi 密码放 `config.local.h`，不要写进已跟踪的源码。
