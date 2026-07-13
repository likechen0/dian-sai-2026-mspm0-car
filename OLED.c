#include "OLED.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-braces"
#include "OLED_Font.h"
#pragma clang diagnostic pop
#include "ti_msp_dl_config.h"

/*
 * 简单的软件 I2C SSD1306 OLED 驱动。
 * 本工程用 PA1/PA0 模拟开漏：高电平靠释放引脚，低电平主动拉低。
 */
#define OLED_ADDRESS 0x78U

static void OLED_I2C_Delay(void)
{
    /* 软件 I2C 翻转边沿之间的短延时。 */
    delay_cycles(CPUCLK_FREQ / 1000000U);
}

static void OLED_WritePin(uint32_t pin, uint8_t level)
{
    if (level != 0U) {
        /* 释放引脚，让外部/内部上拉把线拉高。 */
        DL_GPIO_disableOutput(OLED_I2C_PORT, pin);
    } else {
        /* 主动输出低电平。 */
        DL_GPIO_clearPins(OLED_I2C_PORT, pin);
        DL_GPIO_enableOutput(OLED_I2C_PORT, pin);
    }
    OLED_I2C_Delay();
}

#define OLED_W_SCL(x) OLED_WritePin(OLED_I2C_SCL_PIN, (x))
#define OLED_W_SDA(x) OLED_WritePin(OLED_I2C_SDA_PIN, (x))

static void OLED_I2C_Init(void)
{
    DL_GPIO_clearPins(OLED_I2C_PORT, OLED_I2C_SCL_PIN | OLED_I2C_SDA_PIN);
    OLED_W_SCL(1);
    OLED_W_SDA(1);
}

static void OLED_I2C_Start(void)
{
    /* I2C 起始信号：SCL 为高时 SDA 下降。 */
    OLED_W_SDA(1);
    OLED_W_SCL(1);
    OLED_W_SDA(0);
    OLED_W_SCL(0);
}

static void OLED_I2C_Stop(void)
{
    /* I2C 停止信号：SCL 为高时 SDA 上升。 */
    OLED_W_SDA(0);
    OLED_W_SCL(1);
    OLED_W_SDA(1);
}

static void OLED_I2C_SendByte(uint8_t Byte)
{
    uint8_t i;

    /* 先发高位。为了驱动简单，这里忽略 ACK。 */
    for (i = 0; i < 8U; i++) {
        OLED_W_SDA((Byte & (0x80U >> i)) != 0U);
        OLED_W_SCL(1);
        OLED_W_SCL(0);
    }

    OLED_W_SDA(1);
    OLED_W_SCL(1);
    OLED_W_SCL(0);
}

static void OLED_WriteCommand(uint8_t Command)
{
    /* 控制字节 0x00 表示后面发送命令。 */
    OLED_I2C_Start();
    OLED_I2C_SendByte(OLED_ADDRESS);
    OLED_I2C_SendByte(0x00U);
    OLED_I2C_SendByte(Command);
    OLED_I2C_Stop();
}

static void OLED_WriteData(uint8_t Data)
{
    /* 控制字节 0x40 表示后面发送显示数据。 */
    OLED_I2C_Start();
    OLED_I2C_SendByte(OLED_ADDRESS);
    OLED_I2C_SendByte(0x40U);
    OLED_I2C_SendByte(Data);
    OLED_I2C_Stop();
}

static void OLED_SetCursor(uint8_t Y, uint8_t X)
{
    /* SSD1306 页寻址：Y 是页，X 是列。 */
    OLED_WriteCommand(0xB0U | Y);
    OLED_WriteCommand(0x10U | ((X & 0xF0U) >> 4U));
    OLED_WriteCommand(0x00U | (X & 0x0FU));
}

void OLED_Clear(void)
{
    uint8_t i;
    uint8_t j;

    /* 清空 8 页 x 128 列的所有像素。 */
    for (j = 0; j < 8U; j++) {
        OLED_SetCursor(j, 0);
        for (i = 0; i < 128U; i++) {
            OLED_WriteData(0x00U);
        }
    }
}

void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{
    uint8_t i;
    uint8_t index;

    /* 字模是 8x16，所以屏幕按 4 行 x 16 列使用。 */
    if ((Line < 1U) || (Line > 4U) || (Column < 1U) || (Column > 16U)) {
        return;
    }
    if ((Char < ' ') || (Char > '~')) {
        Char = ' ';
    }

    index = (uint8_t)(Char - ' ');
    OLED_SetCursor((Line - 1U) * 2U, (Column - 1U) * 8U);
    for (i = 0; i < 8U; i++) {
        OLED_WriteData(OLED_F8x16[index][i]);
    }

    OLED_SetCursor((Line - 1U) * 2U + 1U, (Column - 1U) * 8U);
    for (i = 0; i < 8U; i++) {
        OLED_WriteData(OLED_F8x16[index][i + 8U]);
    }
}

void OLED_ShowString(uint8_t Line, uint8_t Column, const char *String)
{
    uint8_t i;

    /* 到达右边界就停止显示，不自动换到下一行。 */
    for (i = 0; String[i] != '\0'; i++) {
        if ((Column + i) > 16U) {
            break;
        }
        OLED_ShowChar(Line, Column + i, String[i]);
    }
}

static uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;

    while (Y > 0U) {
        Result *= X;
        Y--;
    }
    return Result;
}

void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i;

    for (i = 0; i < Length; i++) {
        OLED_ShowChar(Line, Column + i,
            (char)(Number / OLED_Pow(10U, Length - i - 1U) % 10U + '0'));
    }
}

void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
    uint8_t i;
    uint32_t absNumber;

    if (Number >= 0) {
        OLED_ShowChar(Line, Column, '+');
        absNumber = (uint32_t)Number;
    } else {
        OLED_ShowChar(Line, Column, '-');
        absNumber = (uint32_t)(-Number);
    }

    for (i = 0; i < Length; i++) {
        OLED_ShowChar(Line, Column + i + 1U,
            (char)(absNumber / OLED_Pow(10U, Length - i - 1U) % 10U + '0'));
    }
}

void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i;
    uint8_t singleNumber;

    for (i = 0; i < Length; i++) {
        singleNumber = (uint8_t)(Number / OLED_Pow(16U, Length - i - 1U) % 16U);
        if (singleNumber < 10U) {
            OLED_ShowChar(Line, Column + i, (char)(singleNumber + '0'));
        } else {
            OLED_ShowChar(Line, Column + i, (char)(singleNumber - 10U + 'A'));
        }
    }
}

void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i;

    for (i = 0; i < Length; i++) {
        OLED_ShowChar(Line, Column + i,
            (char)(Number / OLED_Pow(2U, Length - i - 1U) % 2U + '0'));
    }
}

void OLED_Init(void)
{
    /* OLED 上电后先等一会儿，再发送初始化命令。 */
    delay_cycles(CPUCLK_FREQ / 10U);

    OLED_I2C_Init();

    /* 128x64 SSD1306 的常规初始化序列。 */
    OLED_WriteCommand(0xAEU);
    OLED_WriteCommand(0xD5U);
    OLED_WriteCommand(0x80U);
    OLED_WriteCommand(0xA8U);
    OLED_WriteCommand(0x3FU);
    OLED_WriteCommand(0xD3U);
    OLED_WriteCommand(0x00U);
    OLED_WriteCommand(0x40U);
    OLED_WriteCommand(0xA1U);
    OLED_WriteCommand(0xC8U);
    OLED_WriteCommand(0xDAU);
    OLED_WriteCommand(0x12U);
    OLED_WriteCommand(0x81U);
    OLED_WriteCommand(0xCFU);
    OLED_WriteCommand(0xD9U);
    OLED_WriteCommand(0xF1U);
    OLED_WriteCommand(0xDBU);
    OLED_WriteCommand(0x30U);
    OLED_WriteCommand(0xA4U);
    OLED_WriteCommand(0xA6U);
    OLED_WriteCommand(0x8DU);
    OLED_WriteCommand(0x14U);
    OLED_WriteCommand(0xAFU);

    OLED_Clear();
}
