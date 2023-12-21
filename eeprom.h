// guus 2023. i noticed that the flash functions from the ch32 lib don't alway seem to work unfortunatly...
// addresss start at 0, not memory address.
// note the use of FLASH->CTLR =  something instead of |=. the mcu seems to lock up in some cases if |= is used, havent investigated.
// uses the last page of flash, so 256b for x035,v103,v103, 64b for v003.

#if defined(CH32X035)
#include <ch32x035.h>
#include <ch32x035_flash.h>
#define P_SIZE 256
#define EEPROM_OFFSET 0x0800F700
#endif
#if defined(CH32V10X)
#include <ch32v10x.h>
#include <ch32v10x_flash.h>
#define P_SIZE 256
#define EEPROM_OFFSET 0x0800F700
#endif
#if defined(CH32V20X)
#include <ch32v20x.h>
#include <ch32v20x_flash.h>
#define P_SIZE 256
#define EEPROM_OFFSET 0x0800F700
#endif

#if defined(CH32V00X)
#include <ch32v00x.h>
#include <ch32v00x_flash.h>
#define P_SIZE 64
#define EEPROM_OFFSET 0x08003FC0
#endif

/* Flash Control Register bits */
#define CR_PG_Set ((uint32_t)0x00000001)
#define CR_PG_Reset ((uint32_t)0xFFFFFFFE)
#define CR_PER_Set ((uint32_t)0x00000002)
#define CR_PER_Reset ((uint32_t)0xFFFFFFFD)
#define CR_MER_Set ((uint32_t)0x00000004)
#define CR_MER_Reset ((uint32_t)0xFFFFFFFB)
#define CR_OPTPG_Set ((uint32_t)0x00000010)
#define CR_OPTPG_Reset ((uint32_t)0xFFFFFFEF)
#define CR_OPTER_Set ((uint32_t)0x00000020)
#define CR_OPTER_Reset ((uint32_t)0xFFFFFFDF)
#define CR_STRT_Set ((uint32_t)0x00000040)
#define CR_LOCK_Set ((uint32_t)0x00000080)
#define CR_FAST_LOCK_Set ((uint32_t)0x00008000)
#define CR_PAGE_PG ((uint32_t)0x00010000)
#define CR_PAGE_ER ((uint32_t)0x00020000)
#define CR_BER32 ((uint32_t)0x00040000)
#define CR_BUF_LOAD ((uint32_t)0x00040000)
#define CR_BER64 ((uint32_t)0x00080000)
#define CR_BUF_RESET ((uint32_t)0x00080000)
#define CR_PG_STRT ((uint32_t)0x00200000)

#define SR_BSY ((uint32_t)0x00000001)

/* FLASH_Access_CLK */
#define FLASH_Access_SYSTEM_HALF ((uint32_t)0x00000000) /* FLASH Enhance Clock = SYSTEM */
#define FLASH_Access_SYSTEM ((uint32_t)0x02000000)      /* Enhance_CLK = SYSTEM/2 */

void _eeprom_ul()
{
    // Unkock flash - be aware you need extra stuff for the bootloader.
    FLASH->KEYR = 0x45670123;
    FLASH->KEYR = 0xCDEF89AB;

    // For option bytes.
    //	FLASH->OBKEYR = 0x45670123;
    //	FLASH->OBKEYR = 0xCDEF89AB;

    // For unlocking programming, in general.
    FLASH->MODEKEYR = 0x45670123;
    FLASH->MODEKEYR = 0xCDEF89AB;
}

void _eeprom_l()
{
    FLASH->CTLR |= CR_FAST_LOCK_Set;
    FLASH->CTLR |= CR_LOCK_Set;
}

void _eeprom_pageE(unsigned long addr)
{
    unsigned long a = EEPROM_OFFSET + addr;

    FLASH->CTLR |= CR_PAGE_ER;
    FLASH->ADDR = a;
    FLASH->CTLR |= CR_STRT_Set;
    while (FLASH->STATR & SR_BSY)
        ;
    FLASH->CTLR &= ~CR_PAGE_ER;
}

void _eeprom_wordWr(unsigned char addr, unsigned long word)
{
    // assume always just erased
    if (word == 0xffffffff)
        return;

    unsigned long a = EEPROM_OFFSET + addr;
    // Unkock flash - be aware you need extra stuff for the bootloader.
    // FLASH->KEYR = 0x45670123;
    // FLASH->KEYR = 0xCDEF89AB;

    // // // For option bytes.
    // // FLASH->OBKEYR = 0x45670123;
    // // FLASH->OBKEYR = 0xCDEF89AB;

    // // For unlocking programming, in general.
    // FLASH->MODEKEYR = 0x45670123;
    // FLASH->MODEKEYR = 0xCDEF89AB;

    // // a &= 0xFFFFFF00;

    FLASH->CTLR = CR_BUF_RESET;
    while (FLASH->STATR & SR_BSY)
        ;
    while (FLASH->STATR & (SR_BSY << 1))
        ;
    FLASH->CTLR = CR_PAGE_PG;
    FLASH->ADDR = a;
    while (FLASH->STATR & SR_BSY)
        ;
    while (FLASH->STATR & (SR_BSY << 1))
        ;
    // for (unsigned short i = 0; i < 2; i++)
    // // for (unsigned short i = 0; i < (P_SIZE / 4); i++)
    // {
    // *(unsigned long *)FLASH->ADDR = 0xf0f0f0;
    *(unsigned long *)FLASH->ADDR = word;
    // *(unsigned long *)FLASH->ADDR = p[i];
    // *(unsigned long *)FLASH->ADDR = p[i];
    // *(unsigned long *)(a + (i * 4)) = i;
    while (FLASH->STATR & SR_BSY)
        ;
    while (FLASH->STATR & (SR_BSY << 1))
        ;

    FLASH->CTLR = CR_BUF_LOAD;
    while (FLASH->STATR & SR_BSY)
        ;
    while (FLASH->STATR & (SR_BSY << 1))
        ;

    FLASH->CTLR = CR_PG_STRT | CR_PAGE_PG;
    while (FLASH->STATR & SR_BSY)
        ;

    FLASH->CTLR &= ~CR_PAGE_PG;
    while (FLASH->STATR & SR_BSY)
        ;
    while (FLASH->STATR & (SR_BSY << 1))
        ;
}

void eeprom_read_block(unsigned char addr, unsigned char *p, unsigned short len)
{
    unsigned char *data = (unsigned char *)(EEPROM_OFFSET + addr);
    for (unsigned short i = 0; i < len; i++)
    {
        p[i] = data[i];
    }
}

void eeprom_write_block(unsigned char addr, unsigned char *p, unsigned short len)
{
    if (addr + len > P_SIZE)
        return;
    unsigned long *l[P_SIZE / 4];             // arr
    unsigned char *ch = (unsigned char *)(l); // char p to arr + address
    eeprom_read_block(0, (unsigned char *)l, P_SIZE);
    // memcpy(ch + addr, p, len); //?? is memcpy wise here?
    for (unsigned char i = 0; i < len; i++)
    {
        // ch[i] = i;
        ch[i + addr] = p[i];
    }
    _eeprom_ul();
    _eeprom_pageE(0);
    for (unsigned char i = 0; i < 64; i++)
    {
        _eeprom_wordWr(i * 4, l[i]);
        // delay(10);
        // _eeprom_wordWr(i, l[i]);
    }
    _eeprom_l();
}

unsigned char eeprom_read(unsigned char addr)
{
    unsigned char p[1];
    eeprom_read_block(addr, p, 1);
    return p[0];
}

void eeprom_write(unsigned char addr, unsigned char val)
{
    unsigned char p[1];
    p[0] = val;
    eeprom_write_block(addr, p, 1);
}
