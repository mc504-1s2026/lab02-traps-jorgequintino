#include <kernel/serial.h>
#include <kernel/panic.h>
#include <arch/spinlock.h>
#include <arch/plic.h>
#include <kernel/string.h>
#include <arch/csr.h>

/* Supervisor External Interrupt Enable (Bit 9) */
#define SIE_SEIE (1UL << 9)

/* Macros utilitárias para acessar os registradores como memória */
#define REG8(offset) (*(volatile u8 *)((u8*)SERIAL_BASE + (offset)))

#define SERIAL_BUF_SIZE 256

struct serial_dev {
    char buf[SERIAL_BUF_SIZE];
    size_t len;
    struct spinlock lock;
};

static struct serial_dev sdev = {0};

void serial_init()
{

    /* Desabilita todas as interrupções antes de configurar */
    REG8(SERIAL_IER) = 0x00;

    /* Configura os registradores DLAB para baud rate (se necessário no emulador, 
       mas geralmente o QEMU ignora baud rate virtual) */
    REG8(SERIAL_LCR) = 0x80; 
    REG8(SERIAL_RBR) = 0x01; /* LSB */
    REG8(SERIAL_IER) = 0x00; /* MSB */
    REG8(SERIAL_LCR) = 0x03; /* 8 bits, sem paridade, 1 stop bit */

    /* Habilita a FIFO e limpa os buffers de RX/TX */
    REG8(SERIAL_FCR) = SERIAL_FCR_FIFO_ENABLE | SERIAL_FCR_RX_FIFO_CLEAR | SERIAL_FCR_TX_FIFO_CLEAR;
}

void serial_irq_enable()
{
    /* Habilita interrupção de recebimento (Received Data Available) no próprio device */
    REG8(SERIAL_IER) |= SERIAL_IER_ERBFI;

    /* Configura o PLIC para a porta serial */
    plic_irq_set_priority(IRQ_SERIAL, 1);
    plic_hart_set_threshold(0, 0);
    plic_hart_enable_irq(0, IRQ_SERIAL);

    /* Habilita interrupções externas no hart */
    csr_set(CSR_SIE, SIE_SEIE);
}

void serial_irq_disable()
{
    REG8(SERIAL_IER) &= ~SERIAL_IER_ERBFI;
    plic_irq_set_priority(IRQ_SERIAL, 0);
    csr_clear(CSR_SIE, SIE_SEIE);
}

void serial_irq()
{
    /* Lemos até que a FIFO esteja vazia */
    while (REG8(SERIAL_LSR) & SERIAL_LSR_DTR) {
        char c = REG8(SERIAL_RBR);
        
        spin_lock(&sdev.lock);
        if (sdev.len < SERIAL_BUF_SIZE) {
            sdev.buf[sdev.len++] = c;
        }
        spin_unlock(&sdev.lock);
    }
}

size_t serial_read(char *buf)
{
    size_t bytes_read = 0;
    
    spin_lock(&sdev.lock);
    bytes_read = sdev.len;
    for (size_t i = 0; i < bytes_read; i++) {
        buf[i] = sdev.buf[i];
    }
    sdev.len = 0;
    spin_unlock(&sdev.lock);
    
    return bytes_read;
}

void serial_putc(char c)
{
    /* Aguarda até o Transmitter Holding Register estar vazio */
    while ((REG8(SERIAL_LSR) & SERIAL_LSR_THRE) == 0) {
        /* Busy wait */
    }
    REG8(SERIAL_THR) = c;
}

void serial_puts(char *str)
{
    while (*str) {
        serial_putc(*str++);
    }
}
