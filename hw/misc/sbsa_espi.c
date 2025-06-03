#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "qapi/error.h"
#include "qemu/log.h"
#include "hw/irq.h"
#include "qemu/module.h"
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

#include "ftd2xx.h"
#include "LibFT4222.h"


#define Addr_Write_Page0 0x50
#define Addr_Write_Page1 0x51
#define Addr_Write_Page2 0x52

// SPI Master can assert SS0O in single mode
// SS0O and SS1O in dual mode, and
// SS0O, SS1O, SS2O and SS3O in quad mode.
#define SLAVE_SELECT(x) (1 << (x))

const int slaveSelectPin = 10;

uint8_t PWM_Gamma64[64] =
{
  0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
  0x08,0x09,0x0b,0x0d,0x0f,0x11,0x13,0x16,
  0x1a,0x1c,0x1d,0x1f,0x22,0x25,0x28,0x2e,
  0x34,0x38,0x3c,0x40,0x44,0x48,0x4b,0x4f,
  0x55,0x5a,0x5f,0x64,0x69,0x6d,0x72,0x77,
  0x7d,0x80,0x88,0x8d,0x94,0x9a,0xa0,0xa7,
  0xac,0xb0,0xb9,0xbf,0xc6,0xcb,0xcf,0xd6,
  0xe1,0xe9,0xed,0xf1,0xf6,0xfa,0xfe,0xff
};

typedef struct SBSAeSPIState {
    SysBusDevice parent_obj;
    MemoryRegion iomem;
    int fd;
    FT_HANDLE ftHandle;
} SBSAeSPIState;

#define TYPE_SBSA_ESPI "sbsa-espi"
OBJECT_DECLARE_SIMPLE_TYPE(SBSAeSPIState, SBSA_ESPI)

static FT_STATUS FindFT4222(DWORD *LocId)
{
    FT_STATUS                 ftStatus = FT_OK;
    FT_DEVICE_LIST_INFO_NODE *devInfo = NULL;
    DWORD                     numDevs = 0;
    DWORD                     i;

    ftStatus = FT_CreateDeviceInfoList(&numDevs);
    if (ftStatus != FT_OK)
    {
        printf("ERROR: FT_CreateDeviceInfoList failed (error code %d)\n", ftStatus);
        goto exit;
    }

    if (numDevs == 0)
    {
        printf("ERROR: No FTDI devices connected\n");
	ftStatus = FT_DEVICE_NOT_FOUND;
        goto exit;
    }

    devInfo = calloc((size_t)numDevs, sizeof(FT_DEVICE_LIST_INFO_NODE));
    if (devInfo == NULL)
    {
        printf("ERROR: FTDI find allocation failure\n");
	ftStatus = FT_DEVICE_NOT_FOUND;
        goto exit;
    }

    ftStatus = FT_GetDeviceInfoList(devInfo, &numDevs);
    if (ftStatus != FT_OK)
    {
        printf("ERROR: FT_GetDeviceInfoList failed (error code %d)\n", ftStatus);
        goto exit;
    }

    for (i = 0; i < numDevs; i++)
    {
        if (devInfo[i].Type == FT_DEVICE_4222H_3)
        {
            printf("Device %d is FT4222H in mode 3 (single Master or Slave):\n", i);
            printf("  0x%08x  %s  %s\n",
                   (unsigned int)devInfo[i].ID,
                   devInfo[i].SerialNumber,
                   devInfo[i].Description);
	    *LocId = devInfo[i].LocId;
            break;
        }
    }

exit:
    free(devInfo);
    return ftStatus;
}

static FT4222_STATUS SPI_WriteByte(FT_HANDLE ftHandle, uint8_t command, uint8_t address, uint8_t data) {
        uint16_t sizeTransferred;
	uint8_t writebuf[] = {command, address, data};

        return FT4222_SPIMaster_SingleWrite(ftHandle, &writebuf[0], (uint16_t)sizeof(writebuf), &sizeTransferred, true);
}

static void Init3743B(FT_HANDLE ftHandle)
{
        uint8_t i;

        for (i = 0; i < 0xC7; i++)
        {
                SPI_WriteByte(ftHandle, Addr_Write_Page0, i, 0);      // PWM
        }

        for (i = 1; i < 0xC7; i++)
        {
                SPI_WriteByte(ftHandle, Addr_Write_Page1, i, 0xff);   // Scaling
        }

        SPI_WriteByte(ftHandle, Addr_Write_Page2, 0x02, 0x70);
        SPI_WriteByte(ftHandle, Addr_Write_Page2, 0x01, 0xFF);    // GCC
        SPI_WriteByte(ftHandle, Addr_Write_Page2, 0x00, 0x09);    //
}

static uint64_t sbsa_espi_read(void *opaque, hwaddr addr, unsigned size)
{
    //SBSAeSPIState *s = opaque;
    //uint8_t buf = 0;

    printf("INFO: sbsa_espi_read (enter).\n");

    //if (read(s->fd, &buf, 1) == 1) {
        //printf("INFO: sbsa_espi_read (exit - read).\n");
        //return buf;
    //}

    printf("INFO: sbsa_espi_read (exit - read fail).\n");
    return 0;
}

static void sbsa_espi_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    SBSAeSPIState *s = opaque;
    uint8_t byte = val & 0xFF;
    uint8_t i;
    int8_t j;


    printf("INFO: sbsa_espi_write (enter), byte=%d.\n", byte);

    switch(byte) {
        case 0:
	{
		for (j = 0; j < 64; j++)        //BLUE
                {
                        for (i = 1; i < 0xC7; i = i + 3)
                        {
                                SPI_WriteByte(s->ftHandle, Addr_Write_Page0, i, PWM_Gamma64[j]);   //PWM
                        }
                }

                for (j = 63; j >= 0; j--)
                {
                        for (i = 1; i < 0xC7; i = i + 3)
                        {
                                SPI_WriteByte(s->ftHandle, Addr_Write_Page0, i, PWM_Gamma64[j]);   //PWM
                        }
                }
	}
        break;
        case 1:
	{
		for (j = 0; j < 64; j++)        //GREEN
                {
                        for (i = 2; i < 0xC7; i = i + 3)
                        {
                                SPI_WriteByte(s->ftHandle, Addr_Write_Page0, i, PWM_Gamma64[j]);   //PWM
                        }
                }

                for (j = 63; j >= 0; j--)
                {
                        for (i = 2; i < 0xC7; i = i + 3)
                        {
                                SPI_WriteByte(s->ftHandle, Addr_Write_Page0, i, PWM_Gamma64[j]);   //PWM
                        }
                }
	}
        break;
        case 2:
	{
		for (j = 0; j < 64; j++)        //RED
                {
                        for (i = 3; i < 0xC7; i = i + 3)
                        {
                                SPI_WriteByte(s->ftHandle, Addr_Write_Page0, i, PWM_Gamma64[j]);   //PWM
                        }
                }

                for (j = 63; j >= 0; j--)
                {
                        for (i = 3; i < 0xC7; i = i + 3)
                        {
                                SPI_WriteByte(s->ftHandle, Addr_Write_Page0, i, PWM_Gamma64[j]);   //PWM
                        }
                }
	}
        break;
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
    uint32_t ftdi_loc_id = 0;
    FT_STATUS            ftStatus;
    FT4222_Version       ft4222Version;

    
    // Initialize FTDI device
    if (FindFT4222(&ftdi_loc_id) == 0) {
        
	printf("INFO: opening eSPI port (location_id=%d)...\n", ftdi_loc_id);

        ftStatus = FT_OpenEx((PVOID)(uintptr_t)ftdi_loc_id,
                             FT_OPEN_BY_LOCATION,
                             &s->ftHandle);
        if (ftStatus != FT_OK)
        {
            printf("FT_OpenEx failed (error %d)\n", (int)ftStatus);
            exit(1);
        }

	ftStatus = FT4222_GetVersion(s->ftHandle, &ft4222Version);
        if (FT4222_OK != ftStatus)
        {
                printf("FT4222_GetVersion failed (error %d)\n", (int)ftStatus);
                exit(1);
        }

        printf("INFO: Chip version: 0x%x, LibFT4222 version: 0x%x\n", (uint32_t)ft4222Version.chipVersion, (uint32_t)ft4222Version.dllVersion);

        ftStatus = FT4222_SPIMaster_Init(s->ftHandle, SPI_IO_SINGLE, CLK_DIV_2, CLK_IDLE_LOW, CLK_LEADING, SLAVE_SELECT(0));
        if (FT_OK != ftStatus)
        {
                printf("Init FT4222 as SPI master device failed!");
                exit(1);
        }

        ftStatus = FT4222_SPI_SetDrivingStrength(s->ftHandle, DS_8MA, DS_8MA, DS_8MA);
        if (FT_OK != ftStatus)
        {
                printf("set spi driving strength failed!");
                exit(1);
        }

        ftStatus = FT4222_SPIMaster_SetLines(s->ftHandle, SPI_IO_SINGLE);
        if (FT_OK != ftStatus)
        {
                printf("set spi single line failed!");
                exit(1);
        }

	Init3743B(s->ftHandle);
    }

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
