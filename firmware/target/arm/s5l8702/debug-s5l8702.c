/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: debug-s5l8700.c 28719 2010-12-01 18:35:01Z Buschel $
 *
 * Copyright © 2008 Rafaël Carré
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdbool.h>
#include "system.h"
#include "config.h"
#include "kernel.h"
#include "button.h"
#include "lcd.h"
#include "font.h"
#include "storage.h"
#include "power.h"
#include "pmu-target.h"
#include "pcm-target.h"
#ifdef HAVE_SERIAL
#include "uc8702.h"
#endif

#define DEBUG_CANCEL BUTTON_MENU

/*  Skeleton for adding target specific debug info to the debug menu
 */

#define _DEBUG_PRINTF(a, varargs...) lcd_putsf(0, line++, (a), ##varargs);

extern int lcd_type;
extern int rec_hw_ver;

bool dbg_hw_info(void)
{
    int line;
    int i;
    unsigned int state = 0;
#ifdef UC8702_DEBUG
    const unsigned int max_states=3;
#else
    const unsigned int max_states=2;
#endif

    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);

    state=0;
    while(1)
    {
        lcd_clear_display();
        line = 0;

        if(state == 0)
        {
            _DEBUG_PRINTF("CPU:");
            _DEBUG_PRINTF("speed: %d MHz", ((CLKCON0 & 1) ?
                                CPUFREQ_NORMAL : CPUFREQ_MAX) / 1000000);
            _DEBUG_PRINTF("current_tick: %d", (unsigned int)current_tick);
            uint32_t __res;
            asm volatile ("mrc p15, 0, %0, c0, c0, 0" : "=r"(__res));
            _DEBUG_PRINTF("ID code: %08x", __res);
            asm volatile ("mrc p15, 0, %0, c0, c0, 1" : "=r"(__res));
            _DEBUG_PRINTF("cache type: %08x", __res);
            line++;

            _DEBUG_PRINTF("LCD type: %d", lcd_type);
            line++;

            _DEBUG_PRINTF("capture HW: %d", rec_hw_ver);
            line++;
        }
        else if(state==1)
        {
            _DEBUG_PRINTF("PMU:");
            for(i=0;i<7;i++)
            {
                char *device[] = {"(unknown)", 
                                  "(unknown)", 
                                  "(unknown)", 
                                  "(unknown)", 
                                  "(unknown)", 
                                  "(unknown)",
                                  "(unknown)"};
                _DEBUG_PRINTF("ldo%d %s: %dmV %s",i,
                    pmu_read(0x2e + (i << 1))?" on":"off",
                    900 + pmu_read(0x2d + (i << 1))*100,
                    device[i]);
            }
            _DEBUG_PRINTF("cpu voltage: %dmV",625 + pmu_read(0x1e)*25);
            _DEBUG_PRINTF("memory voltage: %dmV",625 + pmu_read(0x22)*25);
            line++;
            _DEBUG_PRINTF("charging: %s", charging_state() ? "true" : "false");
            _DEBUG_PRINTF("backlight: %s", pmu_read(0x29) ? "on" : "off");
            _DEBUG_PRINTF("brightness value: %d", pmu_read(0x28));
        }
#ifdef UC8702_DEBUG
        else if(state==2)
        {
            extern struct uartc_port ser_port;
            int tx_stat, rx_stat, tx_speed, rx_speed;
            char line_cfg[4];
            int abr_stat;
            unsigned int abr_cnt;

            char *abrstatus[] = {"Idle", "Launched", "Counting", "Abnormal"};

            uartc_port_get_line_info(&ser_port, &tx_stat, &rx_stat,
                                        &tx_speed, &rx_speed, line_cfg);

            abr_stat = uartc_port_get_abr_info(&ser_port, &abr_cnt);

            _DEBUG_PRINTF("UART %d:", ser_port.id);
            line++;
            _DEBUG_PRINTF("line: %s", line_cfg);
            _DEBUG_PRINTF("Tx: %s, speed: %d", tx_stat ? "On":"Off", tx_speed);
            _DEBUG_PRINTF("Rx: %s, speed: %d", rx_stat ? "On":"Off", rx_speed);
            _DEBUG_PRINTF("ABR: %s, cnt: %d", abrstatus[abr_stat], abr_cnt);
            line++;
            _DEBUG_PRINTF("n_tx_bytes: %u", ser_port.n_tx_bytes);
            _DEBUG_PRINTF("n_rx_bytes: %u", ser_port.n_rx_bytes);
            _DEBUG_PRINTF("n_ovr_err: %u", ser_port.n_ovr_err);
            _DEBUG_PRINTF("n_parity_err: %u", ser_port.n_parity_err);
            _DEBUG_PRINTF("n_frame_err: %u", ser_port.n_frame_err);
            _DEBUG_PRINTF("n_break_detect: %u", ser_port.n_break_detect);
            _DEBUG_PRINTF("n_abnormal0: %u", ser_port.n_abnormal0);
            _DEBUG_PRINTF("n_abnormal1: %u", ser_port.n_abnormal1);

            sleep(HZ/20);
        }
#endif
        else
        {
            state=0;
        }


        lcd_update(); 
        switch(button_get_w_tmo(HZ/20))
        {
            case BUTTON_SCROLL_BACK:
                if(state!=0) state--;
                break;

            case BUTTON_SCROLL_FWD:
                if(state!=max_states-1)
                {
                    state++;
                }
                break;

            case DEBUG_CANCEL:
            case BUTTON_REL:
                lcd_setfont(FONT_UI);
                return false;
        }
    }

    lcd_setfont(FONT_UI);
    return false;
}

bool dbg_ports(void)
{
    int line;

    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        lcd_clear_display();
        line = 0;
        
        _DEBUG_PRINTF("GPIO  0: %08x",(unsigned int)PDAT(0));
        _DEBUG_PRINTF("GPIO  1: %08x",(unsigned int)PDAT(1));
        _DEBUG_PRINTF("GPIO  2: %08x",(unsigned int)PDAT(2));
        _DEBUG_PRINTF("GPIO  3: %08x",(unsigned int)PDAT(3));
        _DEBUG_PRINTF("GPIO  4: %08x",(unsigned int)PDAT(4));
        _DEBUG_PRINTF("GPIO  5: %08x",(unsigned int)PDAT(5));
        _DEBUG_PRINTF("GPIO  6: %08x",(unsigned int)PDAT(6));
        _DEBUG_PRINTF("GPIO  7: %08x",(unsigned int)PDAT(7));
        _DEBUG_PRINTF("GPIO  8: %08x",(unsigned int)PDAT(8));
        _DEBUG_PRINTF("GPIO  9: %08x",(unsigned int)PDAT(9));
        _DEBUG_PRINTF("GPIO 10: %08x",(unsigned int)PDAT(10));
        _DEBUG_PRINTF("GPIO 11: %08x",(unsigned int)PDAT(11));
        _DEBUG_PRINTF("GPIO 12: %08x",(unsigned int)PDAT(12));
        _DEBUG_PRINTF("GPIO 13: %08x",(unsigned int)PDAT(13));
        _DEBUG_PRINTF("GPIO 14: %08x",(unsigned int)PDAT(14));
        _DEBUG_PRINTF("GPIO 15: %08x",(unsigned int)PDAT(15));
        _DEBUG_PRINTF("USEC   : %08x",(unsigned int)USEC_TIMER);

        lcd_update();
        if (button_get_w_tmo(HZ/10) == (DEBUG_CANCEL|BUTTON_REL))
            break;
    }
    lcd_setfont(FONT_UI);
    return false;
}

