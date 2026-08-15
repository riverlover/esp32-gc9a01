# Handoff — ESP32-C3 Super Mini + GC9A01

> 交接文档。下一会话可直接按本文继续，不必重走摸索过程。

## 当前状态（已验证）

| 项 | 状态 |
|----|------|
| 硬件接线 | 已接好并点亮 |
| macOS 识别 | Espressif USB JTAG/serial，无需额外驱动 |
| 工具链 | PlatformIO Core 6.1.19（`~/.platformio/penv/bin`） |
| 固件 | 已烧录 bring-up 程序；屏显 `GC9A01` / `ESP32-C3`，中心色圆循环变色 |
| 工程路径 | `/Users/lizhenhe/vscode/esp32-GC9A01` |

## 硬件

- **MCU**：ESP32-C3 Super Mini（USB-C，板标 V1601 / Super Mini）
- **屏**：1.28" TFT 240×240，驱动 IC **GC9A01**，7 针排针
- **连接**：杜邦线母对母

### 已采用引脚（与 `src/main.cpp` 一致）

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

## 常用命令

串口端口可能变化，先确认：

```bash
ls /dev/cu.usbmodem*
```

代理（本机 Clash 等，HTTP `127.0.0.1:7890`）：

```bash
export PATH="$HOME/.platformio/penv/bin:$PATH"
export http_proxy=http://127.0.0.1:7890
export https_proxy=http://127.0.0.1:7890
export ALL_PROXY=socks5://127.0.0.1:7890
```

编译 / 烧录 / 监视：

```bash
cd /Users/lizhenhe/vscode/esp32-GC9A01
# 若端口变了，改 platformio.ini 的 upload_port / monitor_port，或：
pio run -t upload --upload-port /dev/cu.usbmodemXXXX
pio device monitor -b 115200
```

## 工程文件

```
esp32-GC9A01/
├── HANDOFF.md          # 本交接文档
├── STEPS.md            # 全程步骤记录
├── platformio.ini
└── src/main.cpp        # 点亮 / 色条 / 中心闪烁圆
```

## 后续可做（未开始）

- 去掉 bring-up，改正式 UI（时钟、传感器、动画等）
- 固定 `upload_port` 为通配或脚本自动探测
- 若要用 Arduino_GFX 1.5+，需换支持 Arduino-ESP32 3.x 的 platform（如 pioarduino）

## 踩坑摘要

1. macOS 对 C3 原生 USB **一般不需驱动**；设备名形如 `USB JTAG/serial debug unit`。
2. 下载慢时务必走系统代理 `7890`。
3. GFX `@^1.5.0` 会编译失败 → 钉死 `1.4.7`。
4. 供电只用 **3.3V**，不要接板子 **5V** 脚给屏。
