#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "platform.h"
#include "ti/csl/csl_chip.h"
#include "ti/csl/csl_cacheAux.h"

#define DEVICE_REG32_W(x,y)   *(volatile uint32_t *)(x)=(y)
#define DEVICE_REG32_R(x)    (*(volatile uint32_t *)(x))

#define CHIP_LEVEL_REG  0x02620000
#define KICK0           (CHIP_LEVEL_REG + 0x0038)
#define KICK1           (CHIP_LEVEL_REG + 0x003C)
/* Magic address RBL is polling */
#define MAGIC_ADDR          0x8ffffc
#define BOOT_MAGIC_ADDR(x)  (MAGIC_ADDR + (1<<28) + (x<<24))
#define IPCGR(x)            (0x02620240 + x*4)
#define NUMBER_OF_CORES     2
#define BOOT_MAGIC_NUMBER   0xBABEFACE
#define BOOT_NUMBER0   0xAAAA5555
#define BOOT_NUMBER1   0x11111111
#define BOOT_NUMBER2   0x22222222
#define BOOT_NUMBER3   0x33333333
#define DDR_ADDR0       0x81000000
#define DDR_ADDR1       0x82000000
#define DDR_ADDR2       0x83000000
#define DDR_ADDR3       0x84000000
#define BOOT_UART_BAUDRATE         115200
/* boot_helloworld version */
char version[] = "01.00.00.01";
/* OSAL functions for Platform Library */
uint8_t *Osal_platformMalloc (uint32_t num_bytes, uint32_t alignment)
{
    return malloc(num_bytes);
}

void Osal_platformFree (uint8_t *dataPtr, uint32_t num_bytes)
{
    /* Free up the memory */
    if (dataPtr)
    {
        free(dataPtr);
    }
}

void Osal_platformSpiCsEnter(void)
{
    return;
}

void Osal_platformSpiCsExit (void)
{
    return;
}
void write_uart(char* msg)
{
    uint32_t i;
    uint32_t msg_len = strlen(msg);

    /* Write the message to the UART */
    for (i = 0; i < msg_len; i++)
    {
        platform_uart_write(msg[i]);
    }
}

void
write_boot_magic_number(void)
{
    uint32_t                coreNum;
    coreNum = platform_get_coreid();
    DEVICE_REG32_W(MAGIC_ADDR, BOOT_MAGIC_NUMBER);
    //while(1);
}

// Core1 boot address register (see sprugw0c section 2.4.1)
#define BOOT_ADDR_CORE1     (*(volatile uint32_t *)0x018000E4U)
#define PSC_BASE            0x02350000
#define PSC_PTCMD           (*(volatile uint32_t *)(PSC_BASE + 0x120))
#define PSC_PTSTAT          (*(volatile uint32_t *)(PSC_BASE + 0x128))
#define PSC_MDCTL_CORE1     (*(volatile uint32_t *)(PSC_BASE + 0xA04))
#define PSC_MDSTAT_CORE1    (*(volatile uint32_t *)(PSC_BASE + 0x804))
#define PSC_PDCTL_CORE      (*(volatile uint32_t *)(PSC_BASE + 0x300))
#define PSC_PDSTAT_CORE     (*(volatile uint32_t *)(PSC_BASE + 0x200))

#define SHARED_MEM_BASE     0x0C000000
#define SHARED_MEM_OFFSET   2700
#define SHARED_MEM_ADDR     (SHARED_MEM_BASE + SHARED_MEM_OFFSET)
#define BUFFER_SIZE         4096

// IPC registers
#define IPCGR1    (*(volatile uint32_t *)0x02620244U)
#define IPCAR1    (*(volatile uint32_t *)0x02620284U)

typedef struct {
    volatile uint32_t flag;
    volatile uint32_t size;
    volatile char buffer[BUFFER_SIZE];
} SharedPacket;

volatile SharedPacket* shared = (SharedPacket*)SHARED_MEM_ADDR;

void short_delay() {
    volatile int i;
    for (i = 0; i < 100000; i++) {}
}

// --- FLASH BOOT: Core1 Release Function (run on Core0) ---
void release_core1(uint32_t core1_start_addr)
{
    BOOT_ADDR_CORE1 = core1_start_addr;
    if (!(PSC_PDSTAT_CORE & 0x1)) {
        PSC_PDCTL_CORE |= 0x1;   // Enable Core power domain
        PSC_PTCMD |= 0x1;        // Start power transition
        while (!(PSC_PTSTAT & 0x1));
    }
    PSC_MDCTL_CORE1 |= 0x3; // Module is enabled (see docs)
    PSC_PTCMD |= (1 << 1);  // Trigger power state transition for Core1 (module 1)
    while ((PSC_PTSTAT & (1 << 1)) != 0); // Wait for transition
}

void sendFromCore0(uint32_t id) {
    shared->size = snprintf((char*)shared->buffer, BUFFER_SIZE, "Hello from CORE0: msg %u", id);
    shared->flag = 0xA5A50000 | id;
    CACHE_wbL1d((void*)shared, sizeof(SharedPacket), CACHE_WAIT);
    IPCGR1 = 1;  // Trigger Core1
    printf("[C66xx_0] Sent msg %u: \"%s\"\n", id, shared->buffer);
}

void waitReplyCore0() {
    while (1) {
        CACHE_invL1d((void*)shared, sizeof(SharedPacket), CACHE_WAIT);
        if (shared->flag == 0) break;
        short_delay();
    }
    printf("[C66xx_0] Got reply: \"%s\"\n", shared->buffer);
}

void handleCore1() {
    CACHE_invL1d((void*)shared, sizeof(SharedPacket), CACHE_WAIT);
    if (shared->flag != 0) {
        uint32_t msgId = shared->flag & 0xFFFF;
        printf("[C66xx_1] Got msg %u: \"%s\"\n", msgId, shared->buffer);
        snprintf((char*)shared->buffer, BUFFER_SIZE, "CORE1 ACK: msg %u", msgId);
        shared->size = strlen((char*)shared->buffer);
        shared->flag = 0;
        CACHE_wbL1d((void*)shared, sizeof(SharedPacket), CACHE_WAIT);
    }
}
uint32_t count = 0;
uint32_t err;//PLATFORM_MAC_TYPE_EEPROM
uint32_t                coreNum, core;
/******************************************************************************
 * Function:    main
 ******************************************************************************/
void main ()
{
    char                    version_msg[] = "\r\n\r\nBoot Hello World Example Version ";
    char                    boot_msg[80];
    platform_info           pform_info;
    err = platform_get_emac_start_evt_id;
    /* Initialize UART */
    coreNum = platform_get_coreid();
    if (coreNum == 0)
    {
        platform_uart_init();
        platform_uart_set_baudrate(BOOT_UART_BAUDRATE);

        printf("%s%s\n\n", version_msg, version);
/*
        int blnkcnt;
        for(blnkcnt=0;blnkcnt<100;blnkcnt++){
            platform_led(1,1,PLATFORM_SYSTEM_LED_CLASS);
            platform_delay(1000);
            platform_led(1,0,PLATFORM_SYSTEM_LED_CLASS);
            platform_delay(1000);
        }
*/

        /* Unlock the chip registers */
        DEVICE_REG32_W(KICK0, 0x83e70b13);
        DEVICE_REG32_W(KICK1, 0x95a4f1e0);

        /* Writing the entry address to other cores */
        for (core = 1; core < NUMBER_OF_CORES; core++)
        {
            sprintf(boot_msg, "\r\n\r\nBooting Hello World image on Core %d from Core 0 ...", core);
            printf("%s\n",boot_msg);
            DEVICE_REG32_W(BOOT_MAGIC_ADDR(core), (uint32_t)write_boot_magic_number);
            /* Delay 1 sec */
            platform_delay(1);
        }
        for (core = 1; core < NUMBER_OF_CORES; core++)
        {
            /* IPC interrupt other cores */
            DEVICE_REG32_W(IPCGR(core), 1);
            platform_delay(1000);
        }
        printf("[C66xx_0] CORE0 starting...\n");
        release_core1(0x00880000);
        int i;
        for(i=0;i<4096;i++)shared->buffer[i] = i;
        while (1) {
            sendFromCore0(count);
            waitReplyCore0();
            count++;
        }

    }
    else
    {
        //TSCL=0;
        write_boot_magic_number();
        printf("[C66xx_1] CORE1 ready...\n");
        while (1) {
            handleCore1();
        }
    }
    while(1);

}
