# lkc 扩展板引脚核对表

本文档是 `lkc` 工程的硬件连接基准。后续修改 SysConfig、排查电机或更换模块时，
优先以本文档和实板连通测量为准，不再使用旧的自动提取引脚表。

## 核对依据

- 原理图：`原理图.pdf`，日期 2025-07-17，标题为“2025电赛拓展版之天猛星（MSPM0G3507）拓展版”。
- PCB 图：同一资料目录中的 `PCB图.png` 和 `3D图 .png`。
- 实板测量：2026-07-13 已确认 `PB14 -> PWMA`、`PA7 -> PWMB`。
- SysConfig：`empty.syscfg`，生成后再次确认 PA7 和 PB14 的定时器复用合法。

原始资料目录：

```text
D:\BaiduNetdiskDownload\07遇到困难睡大觉zxz25电赛系列版--天猛星（M0）基础款\（接着看这里）文件
```

## 关键纠正

| 信号 | 旧错误记录 | 原理图与实板确认值 | SysConfig 复用 |
| --- | --- | --- | --- |
| PWMA | PA18 | PB14 | TIMG12_CCP1 |
| PWMB | PB14 | PA7 | TIMG7_CCP1 |

`PA18` 在这块扩展板上只是普通引出脚，没有连接 TB6612 的 PWM 输入。
此前 U16 不转的直接原因是程序把 B 桥 PWM 输出到了 PB14，而 PB14 实际进入 A 桥的
PWMA；真正的 PWMB/PA7 没有波形，因此 BO1/BO2 始终为 0。

## 逻辑左右轮

左右以小车实际安装位置为准，不按原理图中的 L1/R1 名称直接推断：

| 软件逻辑 | PCB 接口 | TB6612 通道 | PWM | 方向 | 编码器 |
| --- | --- | --- | --- | --- | --- |
| 左轮 | U16 / R1 | B 桥 BO1/BO2 | PWMB / PA7 | BIN1 PB9、BIN2 PB6 | A2 PB4、B2 PB5 |
| 右轮 | U6 / L1 | A 桥 AO1/AO2 | PWMA / PB14 | AIN1 PA13、AIN2 PB10 | A1 PB11、B1 PB12 |

所以软件调用关系是：

```text
Motor_SetSpeed_L() -> U16 -> B桥 -> PWMB/BIN1/BIN2
Motor_SetSpeed_R() -> U6  -> A桥 -> PWMA/AIN1/AIN2
```

## TB6612 控制信号

| TB6612 信号 | MSPM0G3507 | 当前用途 |
| --- | --- | --- |
| PWMA | PB14 / TIMG12_CCP1 | U6 逻辑右轮 PWM |
| AIN1 | PA13 | U6 逻辑右轮方向 |
| AIN2 | PB10 | U6 逻辑右轮方向 |
| PWMB | PA7 / TIMG7_CCP1 | U16 逻辑左轮 PWM |
| BIN1 | PB9 | U16 逻辑左轮方向 |
| BIN2 | PB6 | U16 逻辑左轮方向 |
| STBY | 硬接 5V | TB6612 常使能 |
| VCC | 5V | TB6612 逻辑电源 |
| VM | VBAT_IN | 电机电源 |
| GND | GND | 与主控、电池共地 |

TB6612 芯片 U2 关键管脚：

| U2 管脚 | 信号 | U2 管脚 | 信号 |
| ---: | --- | ---: | --- |
| 1 | VM / VBAT_IN | 16 | PWMA / PB14 |
| 2 | VCC / 5V | 15 | AIN2 / PB10 |
| 3 | GND | 14 | AIN1 / PA13 |
| 4 | AO2 | 13 | STBY / 5V |
| 5 | AO1 | 12 | BIN1 / PB9 |
| 6 | BO2 | 11 | BIN2 / PB6 |
| 7 | BO1 | 10 | PWMB / PA7 |
| 8 | GND | 9 | GND |

当前正速度命令的预期电平：

| 逻辑轮 | PWM | IN1 | IN2 |
| --- | --- | --- | --- |
| 左轮 U16 | PA7 输出 PWM | PB9 低 | PB6 高 |
| 右轮 U6 | PB14 输出 PWM | PA13 低 | PB10 高 |

## 电机接口

| PCB 接口 | 原理图名 | Pin6 | Pin5 | Pin4 | Pin3 | Pin2 | Pin1 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| U6 | L1 | AO1 | 3.3V | A1 | B1 | GND | AO2 |
| U7 | L2 | AO1 | NC | NC | NC | NC | AO2 |
| U16 | R1 | BO1 | 3.3V | A2 | B2 | GND | BO2 |
| U9 | R2 | BO1 | NC | NC | NC | NC | BO2 |

- U6 与 U7 共用 A 桥，不能独立调速。
- U16 与 U9 共用 B 桥，不能独立调速。
- 两驱编码器小车应使用 U6 和 U16；U7、U9 没有编码器电源和反馈线。

## 编码器

| 软件信号 | 电机接口 | MSPM0G3507 | 中断用途 |
| --- | --- | --- | --- |
| LEFT_A | U16 A2 | PB4 | 双边沿计数 |
| LEFT_B | U16 B2 | PB5 | 方向判断 |
| RIGHT_A | U6 A1 | PB11 | 双边沿计数 |
| RIGHT_B | U6 B1 | PB12 | 方向判断 |

由于左右电机镜像安装，实测向前时两侧原始编码器符号相反是正常现象。加入轮速 PI
或使用里程计之前，必须在 `Encoder.h` 中把其中一侧的 `ENCODER_*_DIR` 改为 `-1`，
使“整车向前”时左右速度都为正数。不能直接对编码器值取绝对值，否则倒车方向会丢失。

## 感为八路灰度

H10 是八路独立模拟输出，不使用旧版的 OUT+AD0/AD1/AD2 轮询方式：

| H10 | 信号 | MSPM0G3507 / ADC |
| ---: | --- | --- |
| 1 | OUT_1 / S1 | PB19 / ADC1_CH6 |
| 2 | OUT_2 / S2 | PB17 / ADC1_CH4 |
| 3 | OUT_3 / S3 | PA16 / ADC1_CH1 |
| 4 | OUT_4 / S4 | PA14 / ADC0_CH12 |
| 5 | OUT_5 / S5 | PB20 / ADC0_CH6 |
| 6 | OUT_6 / S6 | PB25 / ADC0_CH4 |
| 7 | OUT_7 / S7 | PA25 / ADC0_CH2 |
| 8 | OUT_8 / S8 | PA27 / ADC0_CH0 |
| 9 | VCC | 5V |
| 10 | GND | GND |

MSPM0 ADC 输入不得超过 3.3V。首次更换传感器型号时，必须先用万用表或示波器确认
每一路 OUT 的最高电压。

## OLED、UART 与指示灯

| 功能 | MSPM0G3507 | 说明 |
| --- | --- | --- |
| OLED SDA | PA0 | 软件 I2C |
| OLED SCL | PA1 | 软件 I2C |
| OLED VCC | 5V | 扩展板 U8 插座 |
| MS901M UART0 TX | PA10 | 接模块 RX |
| MS901M UART0 RX | PA11 | 接模块 TX |
| RUN_LED / LED1 | PA15 | 低电平点亮 |

## 便于测量的排针位置

| 信号 | 排针位置 |
| --- | --- |
| PWMA / PB14 | H1 Pin4 |
| PWMB / PA7 | H1 Pin5 |
| AIN1 / PA13 | H4 Pin15 |
| AIN2 / PB10 | H2 Pin5 |
| BIN1 / PB9 | H4 Pin14 |
| BIN2 / PB6 | H3 Pin13 |
| A1 / PB11 | H2 Pin4 |
| B1 / PB12 | H4 Pin8 |
| A2 / PB4 | H4 Pin4 |
| B2 / PB5 | H4 Pin5 |

排查电机时应沿以下顺序测量：

```text
MSP PWM引脚 -> U2 PWM输入 -> U2方向输入 -> U2 AO/BO输出 -> U6/U16接口
```

不要用编码器 A/B 高电平来判断 TB6612 是否使能；编码器反馈与电机桥驱动是两条独立
电路。

