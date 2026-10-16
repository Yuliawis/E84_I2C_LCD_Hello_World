/*******************************************************************************
* File Name        : main.c
*
* Description      : This source file contains the main routine for non-secure
*                    application running on CM33 CPU.
*
* Related Document : See README.md
*
********************************************************************************
* (c) 2023-2026, Infineon Technologies AG, or an affiliate of Infineon
* Technologies AG. All rights reserved.
* This software, associated documentation and materials ("Software") is
* owned by Infineon Technologies AG or one of its affiliates ("Infineon")
* and is protected by and subject to worldwide patent protection, worldwide
* copyright laws, and international treaty provisions. Therefore, you may use
* this Software only as provided in the license agreement accompanying the
* software package from which you obtained this Software. If no license
* agreement applies, then any use, reproduction, modification, translation, or
* compilation of this Software is prohibited without the express written
* permission of Infineon.
*
* Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
* IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
* THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
* SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
* Infineon reserves the right to make changes to the Software without notice.
* You are responsible for properly designing, programming, and testing the
* functionality and safety of your intended application of the Software, as
* well as complying with any legal requirements related to its use. Infineon
* does not guarantee that the Software will be free from intrusion, data theft
* or loss, or other breaches ("Security Breaches"), and Infineon shall have
* no liability arising out of any Security Breaches. Unless otherwise
* explicitly approved by Infineon, the Software may not be used in any
* application where a failure of the Product or any consequences of the use
* thereof can reasonably be expected to result in personal injury.
*******************************************************************************/

/* Header Files */
#include "cybsp.h"
#include "cy_time.h"
#include "lcd1602.h"
#include "lcd_config.h"

volatile lcd_result_t lcd_status = LCD_ERROR_CONFIG;

#include "FreeRTOS.h"
#include "task.h"
#include "cyabs_rtos.h"
#include "cyabs_rtos_impl.h"

/* Macros */

/* The timeout value in microsecond used to wait for the CM55 core to be booted. Use value 0U for infinite wait till the core is booted successfully. */
#define CM55_BOOT_WAIT_TIME_USEC            (10U)

/* App boot address for CM55 project */
#define CM55_APP_BOOT_ADDR                  (CYMEM_CM33_0_m55_nvm_START + \
                                                CYBSP_MCUBOOT_HEADER_SIZE)

/* Enabling or disabling a MCWDT requires a wait time of upto 2 CLK_LF cycles to come into effect. This wait time value will depend on the actual CLK_LF frequency set by the BSP. */
#define LPTIMER_0_WAIT_TIME_USEC            (62U)

/* Define the LPTimer interrupt priority number. '1' implies highest priority. */
#define APP_LPTIMER_INTERRUPT_PRIORITY      (1U)

/* Global Variables */

#if defined(CYBSP_CM33_LPTIMER_0)
/* LPTimer HAL object */
static mtb_hal_lptimer_t lptimer_obj;
#endif /* defined(CYBSP_CM33_LPTIMER_0) */

#if defined(CYBSP_RTC)
/* RTC HAL object */
static mtb_hal_rtc_t rtc_obj;
#endif /* defined(CYBSP_RTC) */

/* Stop execution after a startup or LCD error. */
static void handle_app_error(void)
{
    /* Disable all interrupts. */
    __disable_irq();

    CY_ASSERT(0);

    /* Infinite loop */
    while(true);
}

/* Initialize the RTC and connect it to the C library support layer. */
#if defined(CYBSP_RTC)
static void setup_clib_support(void)
{
    /* RTC Initialization */
    Cy_RTC_Init(&CYBSP_RTC_config);
    Cy_RTC_SetDateAndTime(&CYBSP_RTC_config);

    /* Initialize the ModusToolbox CLIB support library */
    mtb_clib_support_init(&rtc_obj);
}
#endif /* defined(CYBSP_RTC) */

/* Pass timer interrupts to the HAL. */
#if defined(CYBSP_CM33_LPTIMER_0)
static void lptimer_interrupt_handler(void)
{
    mtb_hal_lptimer_process_interrupt(&lptimer_obj);
}

/* Configure the low-power timer used by FreeRTOS tickless idle. */
static void setup_tickless_idle_timer(void)
{
    /* Interrupt configuration structure for LPTimer */
    cy_stc_sysint_t lptimer_intr_cfg =
    {
        .intrSrc = CYBSP_CM33_LPTIMER_0_IRQ,
        .intrPriority = APP_LPTIMER_INTERRUPT_PRIORITY
    };

    /* Initialize the LPTimer interrupt and specify the interrupt handler. */
    cy_en_sysint_status_t interrupt_init_status = 
                                    Cy_SysInt_Init(&lptimer_intr_cfg, 
                                                    lptimer_interrupt_handler);
    
    /* LPTimer interrupt initialization failed. Stop program execution. */
    if(CY_SYSINT_SUCCESS != interrupt_init_status)
    {
        handle_app_error();
    }

    /* Enable NVIC interrupt. */
    NVIC_EnableIRQ(lptimer_intr_cfg.intrSrc);

    /* Initialize the MCWDT block */
    cy_en_mcwdt_status_t mcwdt_init_status = 
                                    Cy_MCWDT_Init(CYBSP_CM33_LPTIMER_0_HW, 
                                                &CYBSP_CM33_LPTIMER_0_config);

    /* MCWDT initialization failed. Stop program execution. */
    if(CY_MCWDT_SUCCESS != mcwdt_init_status)
    {
        handle_app_error();
    }
  
    /* Enable MCWDT instance */
    Cy_MCWDT_Enable(CYBSP_CM33_LPTIMER_0_HW,
                    CY_MCWDT_CTR_Msk, 
                    LPTIMER_0_WAIT_TIME_USEC);

    /* Setup LPTimer using the HAL object and desired configuration as defined in the device configurator. */
    cy_rslt_t result = mtb_hal_lptimer_setup(&lptimer_obj, 
                                            &CYBSP_CM33_LPTIMER_0_hal_config);
    
    /* LPTimer setup failed. Stop program execution. */
    if(CY_RSLT_SUCCESS != result)
    {
        handle_app_error();
    }

    /* Pass the LPTimer object to abstraction RTOS library that implements tickless idle mode */
    cyabs_rtos_set_lptimer(&lptimer_obj);
}
#endif /* defined(CYBSP_CM33_LPTIMER_0) */

/* Approximate timing only: start the music when the first lyric appears.
 * Adjust each page's hold_ms to match your recording. */
#define LCD_HELLO_HOLD_MS (2000U)

typedef struct
{
    char row0[17];
    char row1[17];
    uint32_t hold_ms;
} lyric_page_t;

static const lyric_page_t lyric_pages[] =
{
    {"I need you right", "now, once I     ", 2200U},
    {"leave you I'm   ", "strung out      ", 2200U},
    {"If I get you,   ", "I'm slowly      ", 2200U},
    {"breaking down   ", "                ", 2200U},
    {"And, oh, it's   ", "hard to see you,", 2200U},
    {"but I wish you  ", "were right here ", 2200U},
    {"Oh, it's hard to", "leave you when I", 2200U},
    {"get you         ", "everywhere      ", 2200U},
    {"All this time   ", "I'm thinking we ", 2200U},
    {"could never be a", "pair            ", 2200U},
    {"Oh, no, I don't ", "need you, but I ", 2200U},
    {"miss you, come  ", "here            ", 2200U},
};

/* Update both full rows so shorter phrases do not leave old characters behind. */
static void lyrics_task(void *argument)
{
    (void)argument;
    TickType_t next_page = xTaskGetTickCount();
    vTaskDelayUntil(&next_page, pdMS_TO_TICKS(LCD_HELLO_HOLD_MS));

    for (uint32_t i = 0U; i < sizeof(lyric_pages) / sizeof(lyric_pages[0]); ++i)
    {
        lcd_status = lcd1602_set_cursor(0U, 0U);
        if (lcd_status == LCD_OK)
        {
            lcd_status = lcd1602_write(lyric_pages[i].row0);
        }
        if (lcd_status == LCD_OK)
        {
            lcd_status = lcd1602_set_cursor(1U, 0U);
        }
        if (lcd_status == LCD_OK)
        {
            lcd_status = lcd1602_write(lyric_pages[i].row1);
        }
        if (lcd_status != LCD_OK)
        {
            /* Keep the error visible in the debugger and stop further writes. */
            break;
        }
        /* Absolute tick scheduling includes the time taken by I2C writes. */
        vTaskDelayUntil(&next_page, pdMS_TO_TICKS(lyric_pages[i].hold_ms));
    }
    /* Play once, leaving the final page visible until reset. */
    vTaskDelete(NULL);
}

/* Initialize the LCD, start CM55, and schedule the lyric sequence. */
int main(void)
{
    cy_rslt_t result;

    /* Initialize the device and board peripherals */
    result = cybsp_init();

    /* Board initialization failed. Stop program execution */
    if (CY_RSLT_SUCCESS != result)
    {
        handle_app_error();
    }

#if defined(CYBSP_RTC)
    /* Setup CLIB support library. */
    setup_clib_support();
#endif /* defined(CYBSP_RTC) */

#if defined(CYBSP_CM33_LPTIMER_0)
    /* Setup the LPTimer instance for CM33 CPU. */
    setup_tickless_idle_timer();
#endif /* defined(CYBSP_CM33_LPTIMER_0) */

    /* Polling LCD startup runs before the RTOS or CM55 can enter idle. */
    uint32_t lcd_clock_hz = Cy_SysClk_PeriPclkGetFrequency(
        (en_clk_dst_t)LCD_CLOCK_GROUP, LCD_CLOCK_TYPE, LCD_CLOCK_NUMBER);
    lcd_status = lcd1602_init(LCD_I2C_HW, &LCD_I2C_CONFIG,
                              lcd_clock_hz, LCD_I2C_ADDRESS);
    if (lcd_status == LCD_OK)
    {
        lcd_status = lcd1602_set_cursor(0U, 0U);
    }
    if (lcd_status == LCD_OK)
    {
        lcd_status = lcd1602_write("Hello World");
    }
    if (lcd_status != LCD_OK)
    {
        handle_app_error();
    }

    /* Enable CM55. */
    /* CM55_APP_BOOT_ADDR must be updated if CM55 memory layout is changed. */
    Cy_SysEnableCM55(MXCM55, CM55_APP_BOOT_ADDR, CM55_BOOT_WAIT_TIME_USEC);

    /* Enable global interrupts */
    __enable_irq();

    if (xTaskCreate(lyrics_task, "LCD lyrics", 512U, NULL,
                    tskIDLE_PRIORITY + 1U, NULL) != pdPASS)
    {
        handle_app_error();
    }

    /* Start the RTOS Scheduler */
    vTaskStartScheduler();
}

/* [] END OF FILE */
