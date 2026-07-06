#include <kernel/printf.h>
#include <kernel/mm.h>
#include <arch/timer.h>
#include <kernel/trap.h>
#include <kernel/serial.h>
#include <kernel/string.h>

extern int _hartid[];
void kmain()
{
    printk_set_level(LOG_DEBUG);
    info("entered S-mode\n");
    info("booting on hart %d\n", _hartid[0]);
    info("setting up virtual memory...\n");
    vm_init();

    info("enabling traps...\n");
    trap_setup();
    info("enabling timer...\n");
    timer_irq_enable();
    info("enabling serial...\n");
    serial_init();
    serial_irq_enable();
    hart_irq_enable();

    /* Buffer local do shell */
    char cmd_buf[256];
    size_t cmd_idx = 0;
    char rx_buf[256];

    printk("> ");

    while (1) {
        /* Lê o que a interrupção já salvou no driver */
        size_t n = serial_read(rx_buf);
        
        for (size_t i = 0; i < n; i++) {
            char c = rx_buf[i];
            
            /* Carriage return identifica execução do comando */
            if (c == '\r') {
                printf("\n");
                cmd_buf[cmd_idx] = '\0';
                
                if (cmd_idx > 0) {
                    if (strcmp(cmd_buf, "uptime") == 0) {
                        u64 time = timer_read();
                        u64 uptime_secs = time / TIMER_FREQ;
                        printk("%llus\n", uptime_secs);
                    } 
                    else if (strncmp(cmd_buf, "echo ", 5) == 0) {
                        printk("%s\n", cmd_buf + 5);
                    } 
                    else if (strncmp(cmd_buf, "alarm ", 6) == 0) {
                        int secs = atoi(cmd_buf + 6);
                        timer_set_alarm((u64)secs);
                    } 
                    else {
                        printk("Unknown command: %s\n", cmd_buf);
                    }
                }
                
                cmd_idx = 0;
                printk("> ");
            } 
            else if (c == '\b' || c == 0x7F) {
                if (cmd_idx > 0) {
                    cmd_idx--;
                    serial_puts("\b \b");
                }
            }
            else {
                /* Echo e armazenamento do caractere na linha */
                if (cmd_idx < sizeof(cmd_buf) - 1) {
                    cmd_buf[cmd_idx++] = c;
                    serial_putc(c);
                }
            }
        }
    }
}
