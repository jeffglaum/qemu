#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "qapi/error.h"
#include "qemu/log.h"
#include "hw/irq.h"
#include "qemu/module.h"
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

typedef struct SBSAeSPIState {
    SysBusDevice parent_obj;
    MemoryRegion iomem;
    int fd;
} SBSAeSPIState;

#define TYPE_SBSA_ESPI "sbsa-espi"
OBJECT_DECLARE_SIMPLE_TYPE(SBSAeSPIState, SBSA_ESPI)

static uint64_t sbsa_espi_read(void *opaque, hwaddr addr, unsigned size)
{
    SBSAeSPIState *s = opaque;
    uint8_t buf = 0;

    printf("INFO: sbsa_espi_read (enter).\n");

    if (read(s->fd, &buf, 1) == 1) {
        printf("INFO: sbsa_espi_read (exit - read).\n");
        return buf;
    }

    printf("INFO: sbsa_espi_read (exit - read fail).\n");
    return 0;
}

static void sbsa_espi_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    SBSAeSPIState *s = opaque;
    uint8_t byte = val & 0xFF;

    printf("INFO: sbsa_espi_write (enter).\n");

    if (write(s->fd, &byte, 1) == 1 ) {
        // Error condition
    }
    printf("INFO: sbsa_espi_write (exit).\n");
}

static const MemoryRegionOps sbsa_espi_ops = {
    .read = sbsa_espi_read,
    .write = sbsa_espi_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 1,
        .max_access_size = 1,
    },
};

static void sbsa_espi_init(Object *obj)
{
    SBSAeSPIState *s = SBSA_ESPI(obj);
    SysBusDevice *dev = SYS_BUS_DEVICE(obj);

    s->fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY);
    if (s->fd < 0) {
        perror("Failed to open eSPI (USB dongle) port");
        exit(1);
    }

    printf("INFO: opened eSPI port\n");

    memory_region_init_io(&s->iomem, obj, &sbsa_espi_ops, s, "sbsa-espi", 0x1000);
    sysbus_init_mmio(dev, &s->iomem);
}

static void sbsa_espi_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->desc = "SBSA eSPI Device";
}

static const TypeInfo sbsa_espi_info = {
    .name = TYPE_SBSA_ESPI,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(SBSAeSPIState),
    .instance_init = sbsa_espi_init,
    .class_init = sbsa_espi_class_init,
    .class_data = NULL,
};

static void sbsa_espi_register_types(void)
{
    type_register_static(&sbsa_espi_info);
}

type_init(sbsa_espi_register_types)
