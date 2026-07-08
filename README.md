# MSPM0G3507 小车工程说明

这是一个基于 MSPM0G3507、CCS Theia 和 SysConfig 的两驱小车工程，目标板按立创天猛星 MSPM0G3507 使用。当前功能包括 TB6612 驱动 310 电机、编码器测速、龙邱 8 路灰度循迹、I2C OLED 显示和 MS901M 陀螺仪辅助固定角度转向。

## 工程入口

主入口在 `empty.c`。`SYSCFG_DL_init()` 会先初始化 SysConfig 生成的时钟、GPIO、PWM、ADC、UART 等底层配置，然后初始化各软件模块：

```c
Motor_Init();
Encoder_Init();
Tracking_Init();
NAV_Init();
Display_Init();
```

主循环约每 10 ms 运行一次：

```text
导航状态机 -> 延时 10 ms -> 更新编码器 -> 更新 OLED -> 翻转运行灯
```

当前真正控制小车行为的是 `InertialNav.c` 里的 `NAV_ControlStep()`，不是旧的 `Tracking_LineFollowStep()`。

## 目录结构

| 文件或目录 | 作用 |
| --- | --- |
| `empty.c` | 主入口和主循环 |
| `empty.syscfg` | SysConfig 外设和引脚配置，改引脚优先改这里 |
| `Debug/ti_msp_dl_config.c/h` | SysConfig 生成代码，提供所有引脚宏和外设初始化 |
| `Debug/makefile`、`Debug/subdir_vars.mk` | 当前 CCS 生成工程的构建文件 |
| `targetConfigs/` | CCS 下载调试配置 |
| 其余 `.c/.h` | 小车各功能模块 |

## 模块说明

| 文件 | 作用 |
| --- | --- |
| `InertialNav.c/h` | 小车导航状态机，决定直行、巡线或陀螺仪转向 |
| `Tracking.c/h` | 龙邱 8 路灰度采样、归一化、线检测和误差计算 |
| `Motor.c/h` | TB6612 方向控制和左右轮有符号速度接口 |
| `PWM.c/h` | TIMA0 两路 PWM，占空比范围为 `0..10000` |
| `Encoder.c/h` | 左右轮编码器 GPIO 中断计数和速度采样 |
| `MS901M.c/h` | MS901M/WitMotion 风格串口帧解析，提取 yaw 角 |
| `Display.c/h` | OLED 页面刷新，显示轮速、灰度状态、误差和计数 |
| `OLED.c/h` | 软件 I2C SSD1306 OLED 驱动 |
| `OLED_Font.h` | OLED 字模数据 |
| `Delay.c/h` | 阻塞毫秒延时 |

## 当前控制逻辑

状态机在 `InertialNav.c`：

```text
一开始没线：直行，速度 NAV_FORWARD_SPEED
检测到线：进入巡线，用 PD 修正左右轮速度
稳定有线后又稳定丢线：计数 C + 1
C 为奇数：左转 45 度
C 为偶数：右转 45 度
转向完成或超时：回到白地直行
```

为了避免上电误触发，代码要求：

```c
#define NAV_LINE_STABLE_CYCLES  5U
#define NAV_LOST_STABLE_CYCLES  3U
```

也就是连续多次检测到线后才允许“丢线计数”，连续多次没线才真正触发转向。

## 主要接线

TB6612：

| TB6612 | MSPM0G3507 |
| --- | --- |
| PWMA | PA0 / TIMA0_CCP0 |
| PWMB | PA1 / TIMA0_CCP1 |
| AIN1 | PA15 |
| AIN2 | PB20 |
| BIN1 | PA28 |
| BIN2 | PA16 |
| STBY | PB26 |
| VM | 电机电源，例如 12 V |
| VCC | 3.3 V |
| GND | MSP GND 和电机电源 GND 共地 |

8 路灰度模块：

| 灰度模块 | MSPM0G3507 |
| --- | --- |
| AS | PA25 / ADC0_CH2 |
| S0 | PA24 |
| S1 | PA26 |
| S2 | PA27 |
| VCC | 3.3 V |
| GND | MSP GND |

编码器：

| 编码器 | MSPM0G3507 |
| --- | --- |
| 左轮 A | PB6 |
| 左轮 B | PB7 |
| 右轮 A | PB8 |
| 右轮 B | PB9 |

I2C OLED：

| OLED | MSPM0G3507 |
| --- | --- |
| SCL | PB10 |
| SDA | PB11 |
| VCC | 3.3 V |
| GND | MSP GND |

MS901M 陀螺仪：

| MS901M | MSPM0G3507 |
| --- | --- |
| TX | PA11 / UART0_RX |
| RX | PA10 / UART0_TX |
| VCC | 3.3 V，或模块说明允许的电压 |
| GND | MSP GND |

## OLED 显示内容

| 行 | 内容 |
| --- | --- |
| 第 1 行 | `W1` 左轮编码器采样值 |
| 第 2 行 | `W2` 右轮编码器采样值 |
| 第 3 行 | `S:xxxxxxxx` 8 路灰度数字状态 |
| 第 4 行 | `E` 循迹误差，`C` 稳定丢线计数 |

## 常用调参位置

循迹参数在 `Tracking.h`：

```c
#define TRACKING_BASE_SPEED      2500
#define TRACKING_KP_NUM          48
#define TRACKING_KP_DEN          100
#define TRACKING_KD_NUM          2
#define TRACKING_KD_DEN          10
```

白地直行速度和转向参数在 `InertialNav.c`：

```c
#define NAV_FORWARD_SPEED       2500
#define NAV_TURN_45_CDEG        4500
#define NAV_TURN_TIMEOUT_STEPS  200U
```

如果左右轮编码器显示方向反了，改 `Encoder.h`：

```c
#define ENCODER_LEFT_DIR  1
#define ENCODER_RIGHT_DIR 1
```

如果黑线读数比白底低，改 `Tracking.h`：

```c
#define TRACKING_BLACK_IS_HIGH   1
```

## 编译和烧录

在 CCS Theia 中打开本工程，先确认 `empty.syscfg` 没有报错，然后 Build。当前命令行也可在 `Debug` 目录下构建：

```powershell
& 'D:\ti\ccs2100\ccs\utils\bin\gmake.exe' -k all
```

生成的 `.out`、`.map`、`.o` 等文件不进入 Git 仓库。

## 版本控制

远程仓库：

```text
https://github.com/likechen0/dian-sai-2026-mspm0-car
```

当前主要标签：

```text
mspm0-v0.1  初始 MSPM0 工程快照
mspm0-v0.2  当前导航工作区版本
mspm0-v0.3  电机、编码器、PWM 注释版本
mspm0-v0.4  导航和 MS901M 注释版本
```

