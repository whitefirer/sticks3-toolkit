# StickS3 Toolkit

M5Stack StickS3 硬件诊断与固件烧录工具集。

## 功能

- **I2C 总线扫描**：自动检测板上 I2C 设备（PMIC、BMI270 IMU 等）
- **BMI270 六轴传感器**：实时读取加速度（g）和陀螺仪（deg/s）数据
- **M5PM1 电源管理**：自动初始化和背光控制
- **网页烧录**：浏览器 USB 一键烧录任意 ESP32-S3 bin 文件
- **串口终端**：Web Serial API 串口调试

## 端口

| 服务 | 端口 |
|------|------|
| StickS3 Toolkit Web Flash | 8997 |

## 快速开始

```bash
# 启动 Web Flash 服务
cd web-flash && python3 -m http.server 8997

# 构建固件
cd firmware && bash build.sh
```

## 硬件信息

| 组件 | 接口 | 地址/引脚 |
|------|------|-----------|
| M5PM1 PMIC | I2C | 0x6E |
| BMI270 IMU | I2C | 0x68 |
| ST7789 显示屏 | SPI | MOSI=39, SCK=40, DC=45, CS=41, RST=21, BL=38 |
| 按钮 A | GPIO | 11 |
| 按钮 B | GPIO | 12 |
| I2C 总线 | GPIO | SDA=47, SCL=48 |

## 按键操作

| 按键 | 功能 |
|------|------|
| A（正面） | 重新扫描 I2C 总线 |
| B（侧面） | 进入 BMI270 六轴实时数据显示 |

## 项目结构

```
sticks3-toolkit/
  firmware/          # ESP32-S3 固件 (ESP-IDF 5.5)
    main/
      main.cpp       # 主循环、按键、扫描逻辑
      display.c/h    # ST7789 显示驱动
      pmic.c/h       # M5PM1 电源管理
      bmi270_wrap.c/h # BMI270 驱动封装
      CMakeLists.txt
      sdkconfig.defaults
    build.sh          # 构建 + 合并 + SHA256
  web-flash/
    index.html       # 通用烧录 + 串口终端
```

## 构建要求

- ESP-IDF 5.5
- 芯片：ESP32-S3
- Python 3.9+

## 链接

- GitHub: https://github.com/whitefirer/sticks3-toolkit
- Blog: https://whitefirer.org
