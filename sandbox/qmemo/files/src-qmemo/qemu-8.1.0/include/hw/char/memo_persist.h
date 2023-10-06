#ifndef HW_MEMO_PERSIST_H
#define HW_MEMO_PERSIST_H

#define TYPE_PCI_MEMO_DEV "memo-persist"

#define MEMO_PCI_VENDOR_ID 0x4296
#define MEMO_PCI_DEVICE_ID 0x1337

#define PORT_GET_RESULT    0
#define PORT_GET_INTFLAG   1
#define PORT_SET_COMMAND   0
#define PORT_DROP_IRQ      1

#define CMD_STORE_GETKEY   0x10
#define CMD_STORE_PAGE     0x11
#define CMD_STORE_FIN      0x12
#define CMD_LOAD_SETKEY    0x20
#define CMD_LOAD_PAGE      0x21
#define CMD_LOAD_FIN       0x22

#define RESULT_COMPLETE    0x00
#define RESULT_INPROGRESS  0x01
#define RESULT_FAILED      0xff

#define INT_CMD            (1<<0)
#define INT_READ_FILE      (1<<1)
#define INT_WRITE_FILE     (1<<2)
#define INT_SDMA           (1<<3)

#define DMA_MAPPING_ERROR  (~(dma_addr_t)0)

//#define TEST_PCI_DEVICE_DEBUG

#ifdef  TEST_PCI_DEVICE_DEBUG
#define tprintf(fmt, ...) printf("## (%3d) %-20s: " fmt, __LINE__, __func__, ## __VA_ARGS__)
#else
#define tprintf(fmt, ...)
#endif

#endif
