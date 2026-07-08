#include "OLED.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-braces"
#include "OLED_Font.h"
#pragma clang diagnostic pop
#include "ti_msp_dl_config.h"

/*
 * Simple software-I2C SSD1306 OLED driver.
 * This project uses PB10/PB11 as open-drain-like GPIOs. A logic high is made
 * by releasing the pin, and a low is made by driving it low.
 */
#define OLED_ADDRESS 0x78U

static void OLED_I2C_Delay(void)
{
    /* Short delay between software-I2C edges. */
    delay_cycles(CPUCLK_FREQ / 1000000U);
}

static void OLED_WritePin(uint32_t pin, uint8_t level)
{
    if (level != 0U) {
        /* Release line high; external/internal pull-up raises it. */
        DL_GPIO_disableOutput(OLED_I2C_PORT, pin);
    } else {
        /* Actively drive line low. */
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
    /* I2C start: SDA falls while SCL is high. */
    OLED_W_SDA(1);
    OLED_W_SCL(1);
    OLED_W_SDA(0);
    OLED_W_SCL(0);
}

static void OLED_I2C_Stop(void)
{
    /* I2C stop: SDA rises while SCL is high. */
    OLED_W_SDA(0);
    OLED_W_SCL(1);
    OLED_W_SDA(1);
}

static void OLED_I2C_SendByte(uint8_t Byte)
{
    uint8_t i;

    /* Send MSB first. ACK is ignored to keep the driver simple. */
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
    /* Control byte 0x00 selects command mode. */
    OLED_I2C_Start();
    OLED_I2C_SendByte(OLED_ADDRESS);
    OLED_I2C_SendByte(0x00U);
    OLED_I2C_SendByte(Command);
    OLED_I2C_Stop();
}

static void OLED_WriteData(uint8_t Data)
{
    /* Control byte 0x40 selects display data mode. */
    OLED_I2C_Start();
    OLED_I2C_SendByte(OLED_ADDRESS);
    OLED_I2C_SendByte(0x40U);
    OLED_I2C_SendByte(Data);
    OLED_I2C_Stop();
}

static void OLED_SetCursor(uint8_t Y, uint8_t X)
{
    /* SSD1306 page addressing: Y is page, X is column. */
    OLED_WriteCommand(0xB0U | Y);
    OLED_WriteCommand(0x10U | ((X & 0xF0U) >> 4U));
    OLED_WriteCommand(0x00U | (X & 0x0FU));
}

void OLED_Clear(void)
{
    uint8_t i;
    uint8_t j;

    /* Clear all 8 pages x 128 columns. */
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

    /* This font is 8x16, so the display is treated as 4 lines x 16 columns. */
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

    /* Stop at the right edge instead of wrapping into the next line. */
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
    /* Give the OLED panel time to power up before sending commands. */
    delay_cycles(CPUCLK_FREQ / 10U);

    OLED_I2C_Init();

    /* Standard SSD1306 init sequence for 128x64 display. */
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
