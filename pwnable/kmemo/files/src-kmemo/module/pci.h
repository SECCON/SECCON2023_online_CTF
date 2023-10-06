#ifndef _PCI_H
#define _PCI_H

#include "kmemo/module.h"

#define PCI_DRIVER_NAME MODULE_NAME

#define MEMO_PCI_VENDOR_ID 0x4296
#define MEMO_PCI_DEVICE_ID 0x1337

#define PORT_GET_RESULT  0
#define PORT_GET_INTFLAG 1

#define PORT_SET_COMMAND 0
#define PORT_DROP_IRQ    1

#define CMD_STORE_GETKEY 0x10
#define CMD_STORE_PAGE   0x11
#define CMD_STORE_FIN    0x12
#define CMD_LOAD_SETKEY  0x20
#define CMD_LOAD_PAGE    0x21
#define CMD_LOAD_FIN     0x22

#define RESULT_COMPLETE   0x00
#define RESULT_INPROGRESS 0x01
#define RESULT_FAILED     0xff

#define INT_CMD         (1<<0)
#define INT_READ_FILE   (1<<1)
#define INT_WRITE_FILE  (1<<2)
#define INT_SDMA        (1<<3)

#define EVENT_COMPLETE  (1<<0)
#define EVENT_FAILED    (1<<1)
#define EVENT_FILE_IO   (1<<2)
#define EVENT_SDMA_DONE (1<<3)
#define EVENT_CMD       (EVENT_COMPLETE|EVENT_FAILED)
#define EVENT_ANY		((1<<4)-1)

struct reg_mmio {
	dma_addr_t sdma_addr;
	uint32_t key;
	union {
		uint32_t len;
		uint32_t pgoff;
	};
};

struct pcidrv_info {
	struct pci_dev *pdev;

	unsigned long pio_base,  pio_flags,  pio_length;
	unsigned long mmio_base, mmio_flags, mmio_length;
	unsigned long ram_base,  ram_flags,  ram_length;

	struct reg_mmio *mmio;
	void *ram;
};

#endif
