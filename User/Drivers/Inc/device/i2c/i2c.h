#pragma once

#include <list.h>
#include <device/device.h>
#include <device/driver.h>

#include <stdint.h>

#define I2C_M_RD        0x0001
#define I2C_M_TEN       0x0010
#define I2C_M_RECV_LEN  0x0100
#define I2C_M_PROBE     0x1000

#define I2C_FUNC_I2C                    0x00000001
#define I2C_FUNC_10BIT_ADDR             0x00000002
#define I2C_FUNC_PROTOCOL_MANGLING      0x00000004

struct i2c_adapter;
struct i2c_driver;

struct i2c_client {
    struct device dev;
    struct i2c_driver *drv;
    struct i2c_adapter *adap;
    struct list_head list;
    char name[32];
    uint16_t addr;
    int irq;
};

struct i2c_message {
    uint16_t addr;
    uint8_t *buf;
    uint8_t len;
    uint8_t flags;
};

struct i2c_algorithm {
    int (*master_xfer)(struct i2c_adapter *adap, struct i2c_message *msgs, int num);
    int (*smbus_xfer)(struct i2c_adapter *adap, unsigned short addr,
                     unsigned short flags, char read_write,
                     unsigned char command, int size, void *data);
    unsigned int (*functionality)(struct i2c_adapter *adap);
};

struct i2c_adapter {
    struct device dev;
    struct list_head list;
    struct list_head client_list;
    const struct i2c_algorithm *algo;
    void *algo_data;
    int timeout;
    int retries;
    int nr;  
};

struct i2c_device_id {
    char name[32];
    uint32_t data;
};

struct i2c_driver {
    struct device_driver drv;
    struct list_head list;
    struct list_head device_list;
    int (*probe)(struct i2c_client *client, const struct i2c_device_id *id);
    int (*remove)(struct i2c_client *client);
    int (*probe_new)(struct i2c_client *client);
};

int i2c_add_addapter(struct i2c_adapter *adap);
int i2c_register_adapter(struct i2c_adapter *adap);

#define to_i2c_adapter(d)   container_of(d, struct i2c_adapter, dev)
#define to_i2c_driver(d)    container_of(d, struct i2c_driver, drv)

