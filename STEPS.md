# 全程步骤记录 — ESP32-C3 + GC9A01 点亮

按实际操作顺序整理，便于复现。

---

## 1. 识别硬件

1. MCU：ESP32-C3 Super Mini（USB-C，排针两侧丝印含 5V / G / 3.3 / 0–10 / 20 / 21）。
2. 屏：圆形 1.28" TFT，背面标 **GC9A01**、240×240，7 针：RST / CS / DC / SDA / SCL / GND / VCC。
3. 材料：杜邦线（彩虹排线 + 电源线）。

---

## 2. 确定接线原则

1. **VCC / GND 固定**：VCC → ESP32 **3.3**，GND → **G**。不要接 5V。
2. **SCL / SDA / DC / CS / RST 不固定**：可任意合法 GPIO，但必须与程序一致。
3. 采用首套推荐（省事、避开 BOOT 相关脚）：

| 屏 | ESP32-C3 | 颜色 |
|----|----------|------|
| VCC | 3.3 | 红 |
| GND | G | 黑 |
| SCL | 6 | 橙 |
| SDA | 7 | 黄 |
| DC | 2 | 绿 |
| CS | 10 | 蓝 |
| RST | 3 | 紫 |

4. 上电前检查：无短路、杜邦插紧、仅用 USB-C 给 ESP32 供电（屏从 3.3 取电）。

---

## 3. macOS 确认设备（烧录前）

1. 插入 USB 后执行：

```bash
ls /dev/cu.*
```

2. 成功时应出现类似：`/dev/cu.usbmodem1101`（数字可能不同）。
3. USB 信息应为 **Espressif / USB JTAG/serial debug unit** → **无需另装串口驱动**。
4. 若完全没有 `usbmodem` 设备：换线、换口；仅当确认是 CH340 芯片板时才考虑装 WCH 驱动（本板不是）。

---

## 4. 安装 PlatformIO（本机原先没有）

1. 系统代理已开：HTTP/HTTPS/SOCKS → `127.0.0.1:7890`（Flash/Clash 等）。
2. 带代理安装官方脚本：

```bash
export http_proxy=http://127.0.0.1:7890
export https_proxy=http://127.0.0.1:7890
export ALL_PROXY=socks5://127.0.0.1:7890

curl -fsSL -o /tmp/get-platformio.py \
  https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py
python3 /tmp/get-platformio.py

export PATH="$HOME/.platformio/penv/bin:$PATH"
pio --version   # 验证，例如 6.1.19
```

3. 建议把 `~/.platformio/penv/bin` 永久加入 shell PATH。

---

## 5. 创建工程

目录：`/Users/lizhenhe/vscode/esp32-GC9A01`

### `platformio.ini` 要点

- `board = esp32-c3-devkitm-1`
- `upload_port` / `monitor_port` 指向当前 `cu.usbmodem*`
- `build_flags`：`ARDUINO_USB_MODE=1`、`ARDUINO_USB_CDC_ON_BOOT=1`
- 库钉死：`GFX Library for Arduino@1.4.7`

### `src/main.cpp` 行为

1. 初始化 GC9A01（引脚见上表）。
2. 全屏红 → 绿 → 蓝 → 黑。
3. 显示文字 `GC9A01` / `ESP32-C3`。
4. `loop` 中心半径 30 的圆按七色循环填充。

---

## 6. 编译与烧录

```bash
export PATH="$HOME/.platformio/penv/bin:$PATH"
export http_proxy=http://127.0.0.1:7890
export https_proxy=http://127.0.0.1:7890
export ALL_PROXY=socks5://127.0.0.1:7890

cd /Users/lizhenhe/vscode/esp32-GC9A01
pio run -t upload
```

首次会下载 platform / toolchain / framework，代理可明显加速。

成功标志：日志出现 `Hash of data verified` 与 `[SUCCESS]`，板子复位。

---

## 7. 验收

| 现象 | 含义 |
|------|------|
| 红绿蓝依次刷屏 | SPI + 驱动初始化成功 |
| 显示 GC9A01 / ESP32-C3 | 文字绘制正常 |
| 中心圆彩色闪烁 | `loop` 在跑，整体链路 OK |

可选串口监视：`pio device monitor -b 115200`，应看到 `Display OK`。

---

## 8. 曾失败再修复的一步

第一次使用 `GFX Library for Arduino@^1.5.0` 时报错：

```text
fatal error: esp32-hal-periman.h: No such file or directory
```

原因：1.5.x 需要 Arduino-ESP32 3.x，当前 PlatformIO `espressif32` 自带 2.x。

处理：改为 `@1.4.7`，清 `libdeps` 后重新 `pio run -t upload` → 成功。

---

## 时间线摘要

1. 硬件选型确认 → 2. 接线方案与颜色约定 → 3. 上电安全检查 →  
4. macOS 识别 USB（免驱）→ 5. 代理安装 PlatformIO →  
6. 写 bring-up 工程 → 7. 遇 GFX 版本冲突并钉版本 → 8. 烧录成功并点亮。

更精简的交接信息见 [HANDOFF.md](./HANDOFF.md)。
