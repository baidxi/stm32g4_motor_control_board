#include <device/tty/tty.h>
#include <device/tty/stm32_uart.h>

#include <init.h>
#include <bus.h>
#include <ring.h>



#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#include <cmsis_os2.h>

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

struct stm32_uart {
    struct tty_device tty;
    uint8_t buf[128];
    size_t buf_len;
    bool is_open;
    struct ring ringbuf;
    xSemaphoreHandle lock;
    osThreadId_t tid;
    bool tx_cplt;
    uint8_t mode;
    uint8_t chart;
};

#define to_stm32_uart(d)  container_of(d, struct stm32_uart, tty)

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    struct stm32_uart *tty = (struct stm32_uart *)tty_device_lookup_by_handle(huart);
    tty->tx_cplt = true;
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(huart);
  UNUSED(Size);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_UARTEx_RxEventCallback can be implemented in the user file.
   */
}

void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
    struct stm32_uart *uart = tty_device_lookup_by_handle(huart);
    struct ring *r = &uart->ringbuf;

    if (uart->mode == TTY_MODE_CONSOLE) {
        uart->buf[r->head & r->mask] = uart->chart;
        ring_enqueue(r, 1);
        if (!ring_is_full(r))
            HAL_UART_Receive_DMA(huart, &uart->chart, 1);
    } else {
        ring_enqueue(r, uart->buf_len -1);
    }
}

static int stm32_uart_open(struct device *dev)
{
    struct stm32_uart *uart = to_stm32_uart(dev);
    UART_HandleTypeDef *handle = uart->tty.dev.private_data;
    struct ring *r = &uart->ringbuf;
    int ret;

    xSemaphoreTake(uart->lock, portMAX_DELAY);

    if (uart->is_open) {
        xSemaphoreGive(uart->lock);
        return -EBUSY;
    }

    uart->is_open = true;
    r->head = r->tail = 0;
    r->mask = uart->buf_len -1;

    if (uart->mode == TTY_MODE_CONSOLE)
        ret = HAL_UART_Receive_DMA(handle, &uart->chart, 1);
    else
        ret = HAL_UART_Receive_DMA(handle, uart->buf, uart->buf_len - 1);

    xSemaphoreGive(uart->lock);

    return ret;
}

static int stm32_uart_close(struct device *dev)
{
    return 0;
}

static int stm32_uart_ioctl(struct device *dev, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static size_t stm32_uart_read(struct device *dev, void *buf, size_t count)
{
    struct stm32_uart *uart =(struct stm32_uart *) to_tty_device(dev);
    UART_HandleTypeDef *handle = uart->tty.dev.private_data;
    uint8_t *out = buf;
    struct ring *r = &uart->ringbuf;
    int i = 0;
    bool dma_start = false;

    if (ring_is_empty(r))
        return 0;

    if (uart->mode == TTY_MODE_CONSOLE)
        dma_start = ring_is_full(r);

    for (i = 0; i < count; i++) {
        out[i] = uart->buf[(r->tail & r->mask)];
        ring_dequeue(r, 1);
        if (ring_is_empty(r))
            break;
    }

    if (uart->mode == TTY_MODE_CONSOLE && dma_start) {
        HAL_UART_Receive_DMA(handle, &uart->chart, 1);
    }
    return i;
}

static size_t stm32_uart_write(struct device *dev, const void *buf, size_t len)
{
    struct tty_device *tty = to_tty_device(dev);
    struct stm32_uart *uart = (struct stm32_uart *)tty;
    UART_HandleTypeDef *handle = tty->dev.private_data;
    int ret;

    xSemaphoreTake(uart->lock, portMAX_DELAY);

    uart->tx_cplt = false;

    ret = HAL_UART_Transmit_DMA(handle, buf, len);

    if (ret == HAL_OK) {
        while(!uart->tx_cplt) {
            taskYIELD();
            ;
        }
    }

    xSemaphoreGive(uart->lock);

    return 0;
}

const struct tty_operations stm32_uart_ops = {
    .open = stm32_uart_open,
    .close = stm32_uart_close,
    .ioctl = stm32_uart_ioctl,
    .read = stm32_uart_read,
    .write = stm32_uart_write,
};

static int stm32_uart_probe(struct tty_device *tty)
{
    struct stm32_uart *uart = (struct stm32_uart *)tty;

    tty->ops = &stm32_uart_ops;

    uart->lock = xSemaphoreCreateMutex();

    if (!uart->lock)
        return -ENOMEM;

    uart->buf_len = sizeof(uart->buf);

    return 0;
}

static void stm32_uart_remove(struct tty_device *tty)
{
    struct stm32_uart *uart = (struct stm32_uart *)tty;

    vSemaphoreDelete(uart->lock);
    tty->ops = NULL;
}

static const struct driver_match_table stm32_uart_ids[] = {
    {
        .compatible = "stm32-uart"
    },
    {

    }
};

int stm32_uart_device_register(struct tty_device *tty)
{
    struct stm32_uart *uart = to_stm32_uart(tty);

    if (!uart)
        return -ENOMEM;

    return tty_device_register(&uart->tty);
}

static void tty_driver_init(struct driver *drv)
{
    tty_driver_register(to_tty_driver(drv));
}

void __weak stm32_uart_init(struct device *dev)
{

}

struct stm32_uart stm32_uart1 = {
    .tty = {
        .dev = {
            .init_name = "stm32-uart",
            .name = "ttyS1",
            .init = stm32_uart_init,
        },
        .port_num = 1,
    },
    .mode = TTY_MODE_CONSOLE,
};

static struct tty_driver stm32_uart_drv = {
    .drv = {
        .match_ptr = stm32_uart_ids,
        .name = "stm32-uart-drv",
        .init = tty_driver_init,
    },
    .probe = stm32_uart_probe,
    .remove = stm32_uart_remove,
};

register_device(stm32_uart1, stm32_uart1.tty.dev);
register_driver(stm32_uart, stm32_uart_drv.drv);