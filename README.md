# ESP32-C3 + GC9A01 圆形表盘

ESP32-C3 Super Mini 驱动 1.28" GC9A01（240×240）圆形 TFT，实现模块化模拟表盘，并通过 Wi‑Fi + NTP 校时。

## 功能

- 五种表盘：Classic / Lume / Skeleton / Calendar / **Photo**（默认）
- Wi‑Fi STA 连接 + NTP 校时（默认东八区）
- 离屏 Canvas 双缓冲，减轻秒针刷新闪烁
- **EC11 旋钮**：黑底缩小悬浮预览、短按确认、长按预留设置
- 串口切换表盘、手动校时、跳过/重试配网

## 硬件

| 部件 | 说明 |
|------|------|
| MCU | ESP32-C3 Super Mini（USB-C） |
| 屏 | 1.28" TFT 240×240，驱动 IC **GC9A01** |
| 旋钮 | EC11 旋转编码器（CLK / DT / SW） |
| 连接 | 杜邦线母对母 |

### 接线（与 `src/pins.h` 一致）

| 屏引脚 | ESP32-C3 | 建议线色 |
|--------|----------|----------|
| VCC | **3.3**（勿接 5V） | 红 |
| GND | **G** | 黑 |
| SCL | **6** | 橙 |
| SDA | **7** | 黄 |
| DC | **2** | 绿 |
| CS | **10** | 蓝 |
| RST | **3** | 紫 |

| EC11 | ESP32-C3 |
|------|----------|
| CLK | **0** |
| DT | **1** |
| SW | **5** |
| + | **3.3** |
| GND | **G** |

信号脚可改，但必须与代码宏一致。尽量避开 GPIO 8/9（启动相关）。

## 软件栈

- [PlatformIO](https://platformio.org/) + `espressif32` / Arduino-ESP32 2.0.x
- 板型：`esp32-c3-devkitm-1`，分区 `huge_app.csv`
- 库：`GFX Library for Arduino@1.4.7`、`QRCode@0.0.1`  
  **不要升 GFX 到 1.5.x**（需要 Arduino-ESP32 3.x）

## 快速开始

### 1. 安装 PlatformIO

```bash
curl -fsSL -o /tmp/get-platformio.py \
  https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py
python3 /tmp/get-platformio.py
export PATH="$HOME/.platformio/penv/bin:$PATH"
```

### 2. 配置 Wi‑Fi（本地，不提交）

```bash
cp src/config.local.h.example src/config.local.h
# 编辑 WIFI_SSID / WIFI_PASS
```

`src/config.local.h` 已在 `.gitignore` 中，请勿提交密码。

推荐优先用手机个人热点（2.4 GHz / WPA2），例如默认提示热点名 `WatchESP`。家宽路由器在部分环境下接屏时可能握手失败，可先空载对比。

### 3. 确认串口并烧录

```bash
ls /dev/cu.usbmodem*          # macOS；数字可能不同
# 按需修改 platformio.ini 中的 upload_port / monitor_port

pio run -t upload
pio device monitor            # 115200
```

烧录成功后串口大致流程：

```text
Wi-Fi OK → NTP OK → Watch OK [NTP] Calendar …
```

## 表盘

| 键 | 风格 | 说明 |
|----|------|------|
| `1` | Classic | 经典指针 |
| `2` | Lume | 夜光风格 |
| `3` | Skeleton | 镂空 |
| `4` | Calendar | 日历窗 |
| `5` | Photo | 照片背景（默认） |

编译期默认：`DEFAULT_FACE`（见 `src/config.h` / `config.local.h`）。运行期串口发 `1`–`5` 或 `n` 切换；也可用 EC11。

## EC11 操作

| 操作 | 作用 |
|------|------|
| 旋转 | **缩小悬浮预览**（约 82%、黑底、静态冻结）上一张 / 下一张 |
| 短按 | 确认当前预览 → 全屏；屏上 Toast |
| 停转约 4s | 自动确认 |
| 长按 | Toast「Settings...」（菜单预留） |

预览期间不按秒刷新，避免缩放重绘抖动。串口 `e` 可查看 SW 电平 / 极性。

## 串口命令

| 命令 | 作用 |
|------|------|
| `1`–`5` / `n` | 选表盘 / 下一个 |
| `e` | EC11 诊断（SW raw / activeLow / 当前面） |
| `w SSID PASS` | 设置 Wi‑Fi 并连接 |
| `s` | 跳过 Wi‑Fi（软时钟） |
| `t YYYY-MM-DD HH:MM:SS` | 手动校时 |
| `p` | 重新进入热点等待 |
| `h` / `?` | 帮助 |

## 工程结构

```text
src/main.cpp           # 编排与串口 / EC11
src/config.h           # 默认配置
src/config.local.h     # 本地密钥（gitignore）
src/pins.h             # GC9A01 + EC11 引脚
src/input/             # EC11 正交解码与按键
src/time/              # NTP / Soft / Manual
src/wifi/              # STA 连接与 RF 调参
src/ui/                # 配网提示 / 二维码
src/face/              # Classic / Lume / Skeleton / Calendar / Photo
```

## 文档

- [HANDOFF.md](HANDOFF.md) — 当前状态、踩坑与后续计划
- [STEPS.md](STEPS.md) — 从接线到点亮的完整操作记录

## 许可

个人/学习用途；第三方库遵循各自许可证。
