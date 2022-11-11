#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "Capture.pio.h"

#define captureSize 512
void dmaHandler();
static inline void setupDma(PIO, uint);

uint32_t buffer[captureSize];
uint dmaChan;

int main()
{
    stdio_init_all();
    dmaChan = dma_claim_unused_channel(true);
    float clkdiv = 12500;

    irq_set_exclusive_handler(DMA_IRQ_0, dmaHandler);
    irq_set_enabled(DMA_IRQ_0, true);
    printf("%d\n", irq_is_enabled(DMA_IRQ_0));

    PIO pio = pio0;
    uint capturePin = 15;
    uint offset = pio_add_program(pio, &capture_program);
    uint sm = pio_claim_unused_sm(pio, true);
    // pio_sm_config c = pio_get_default_sm_config();
    pio_sm_config c = capture_program_get_default_config(offset);

    // pio_gpio_init(pio, capturePin);
    sm_config_set_in_pins(&c, capturePin);
    sm_config_set_in_shift(&c, true, true, 32);  //set autopush to true
    sm_config_set_clkdiv(&c, clkdiv);
    pio_sm_set_consecutive_pindirs(pio, sm, capturePin, 1, false);

    setupDma(pio, sm);
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);


    while (true)
    {
        // printf("%u\n", pio_sm_get_blocking(pio, sm));
        tight_loop_contents();
        // printf("%d\n", dma_channel_is_busy(dmaChan));
    }
    return 0;
}

void dmaHandler()
{
    printf("Received, proccessing ready\n");
    for (int i = 0; i < captureSize; i++)
    {
        printf("%u\n", buffer[i]);
    }
    dma_channel_acknowledge_irq0(dmaChan);
    return;
}

static inline void setupDma(PIO pio, uint sm)
{
    dma_channel_config dc = dma_channel_get_default_config(dmaChan);
    channel_config_set_transfer_data_size(&dc, DMA_SIZE_32);
    channel_config_set_dreq(&dc, pio_get_dreq(pio, sm, false));
    channel_config_set_read_increment(&dc, false);
    channel_config_set_write_increment(&dc, true);

    dma_channel_set_irq0_enabled(dmaChan, true);
    dma_channel_configure(dmaChan,
        &dc,
        &buffer,
        &pio->rxf[sm],
        captureSize,
        true
    );

}

// % c-sdk {
//     static inline void initCapture(Pio pio, uint sm, uint offset, uint pin, float div)
//     {
        
//     }

// %}