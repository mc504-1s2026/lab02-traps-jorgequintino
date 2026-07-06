#include <arch/timer.h>
#include <kernel/panic.h>
#include <arch/csr.h>
#include <kernel/printf.h>

/* Supervisor Timer Interrupt Enable (Bit 5) */
#define SIE_STIE (1UL << 5)

u64 timer_read()
{
    return csr_read(CSR_TIME);
}

void timer_irq_enable()
{
    csr_set(CSR_SIE, SIE_STIE);
}

void timer_irq_disable()
{
    csr_clear(CSR_SIE, SIE_STIE);
}

void timer_set_alarm(u64 secs)
{
    u64 now = timer_read();
    u64 tick_in = now + (secs * TIMER_FREQ);
    csr_write(CSR_STIMECMP, tick_in);
}

void timer_irq()
{
    /* Desarma o timer setando para o máximo valor possível */
    csr_write(CSR_STIMECMP, -1ULL);
    printk("alarm\n> ");
}
