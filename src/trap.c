#include <kernel/trap.h>
#include <kernel/panic.h>
#include <kernel/printf.h>
#include <arch/csr.h>
#include <arch/plic.h>
#include <arch/timer.h>
#include <kernel/serial.h>

extern void trap_entry();

void handle_irq()
{
    u64 cause = csr_read(CSR_SCAUSE);

    if (cause == TRAP_TIMER_IRQ) {
        timer_irq();
    } 
    else if (cause == TRAP_EXTERNAL_IRQ) {
        /* Pede ao PLIC o IRQ atual */
        u32 irq = plic_hart_claim_irq();
        
        if (irq == IRQ_SERIAL) {
            serial_irq();
        }
        
        /* Notifica o PLIC que o tratamento terminou */
        if (irq) {
            plic_hart_complete_irq(irq);
        }
    }
}

void handle_exception()
{
    u64 cause = csr_read(CSR_SCAUSE);
    u64 stval = csr_read(CSR_STVAL);
    u64 sepc = csr_read(CSR_SEPC);
    
    error("Exception %llu at %p (stval: %p)\n", cause, (void*)sepc, (void*)stval);
    panic("Unhandled exception");
}

void handle_trap()
{
    u64 cause = csr_read(CSR_SCAUSE);

    /* O bit 63 define se é interrupção (1) ou exceção (0) */
    if (cause & TRAP_IRQ_BIT) {
        handle_irq();
    } else {
        handle_exception();
    }
}

void trap_setup()
{
    /* Escreve o endereço do assembly stub no stvec */
    csr_write(CSR_STVEC, (u64)trap_entry);
}

void hart_irq_enable()
{
    csr_set(CSR_SSTATUS, SSTATUS_SIE);
}

void hart_irq_disable()
{
    csr_clear(CSR_SSTATUS, SSTATUS_SIE);
}

u64 hart_irq_save()
{
    u64 status = csr_read(CSR_SSTATUS);
    hart_irq_disable();
    return status & SSTATUS_SIE;
}

void hart_irq_restore(u64 flags)
{
    if (flags) {
        hart_irq_enable();
    } else {
        hart_irq_disable();
    }
}
