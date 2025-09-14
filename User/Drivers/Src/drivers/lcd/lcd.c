#include <bus.h>
#include <device/lcd/lcd.h>

#include <errno.h>
#include <string.h>

static struct list_head device_list = LIST_HEAD_INIT(device_list);

static int lcd_probe(struct device *dev)
{
    return dev->driver->probe(dev);
}

static int lcd_remove(struct device *dev)
{
    return dev->driver->remove(dev);
}

static int lcd_match(struct device *dev, struct device_driver *drv)
{
    const struct device_match_table *ptr;

    for (ptr = drv->match_ptr; ptr && ptr->compatible; ptr++) {
        if (strcmp(ptr->compatible, dev->init_name) == 0) {
            dev->driver = drv;
            return 1;
        }
    }
    return 0;
}

static struct bus_type lcd_bus_type = {
    .name = "lcd",
    .probe = lcd_probe,
    .remove = lcd_remove,
    .match = lcd_match,
};

int lcd_device_register(struct lcd_device *lcd)
{
    int ret;

    if (!lcd || !lcd->ops || !lcd->ops->fill || !lcd->ops->point)
        return -EINVAL;

    lcd->dev.bus = &lcd_bus_type;

    ret = device_register(&lcd->dev);
    if (ret)
        return ret;

    list_add_tail(&lcd->list, &device_list);

    return 0;
}

int lcd_driver_register(struct lcd_driver *drv)
{
    drv->drv.bus = &lcd_bus_type;
    drv->drv.probe = lcd_probe;
    drv->drv.remove = lcd_remove;

    return driver_register(&drv->drv);
}

register_bus_type(lcd_bus_type);