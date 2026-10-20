#ifndef __LCD_FONT_H
#define __LCD_FONT_H

/* ASCII 点阵数组：定义在 lcd_font.c 中，此处仅作声明 */
extern const unsigned char ascii_1206[95][12];
extern const unsigned char ascii_1608[95][16];
extern const unsigned char ascii_2412[95][48];
extern const unsigned char ascii_3216[95][64];

typedef struct
{
	unsigned char Index[3];
	unsigned char Msk[24];
} typFNT_GB12;

typedef struct
{
	unsigned char Index[3];
	unsigned char Msk[32];
} typFNT_GB16;

typedef struct
{
	unsigned char Index[3];
	unsigned char Msk[72];
} typFNT_GB24;

typedef struct
{
	unsigned char Index[3];
	unsigned char Msk[128];
} typFNT_GB32;

/* 汉字点阵数组：定义在 lcd_font.c 中，此处仅作声明。
   维数必须写全，否则 lcd.c 里的 sizeof(tfont12) 会报 "incomplete type"。
   不加 const：与 lcd.c 中 LCD_ShowChinese 的调用方式保持一致。 */
extern typFNT_GB12 tfont12[5];
extern typFNT_GB16 tfont16[5];
extern typFNT_GB24 tfont24[5];
extern typFNT_GB32 tfont32[5];

#endif
