#include "qemu/osdep.h"
#include "hw/pci/pci_device.h"
#include "hw/qdev-properties.h"
#include "qemu/module.h"
#include "sysemu/kvm.h"
#include "qom/object.h"
#include "qapi/error.h"

#include <sys/random.h>
#include <sys/sendfile.h>
#include "hw/char/memo_persist.h"

#define PAGE_SIZE 0x1000

struct PCIMemoDevHdr {
	dma_addr_t sdma_addr;
	uint32_t key;
	union {
		uint32_t len;
		uint32_t pgoff;
	};
};

struct PCIMemoDevState {
	PCIDevice parent_obj;

	const bool prefetch_ram;
	const uint32_t limit_pages;

	MemoryRegion portio;
	MemoryRegion mmio;
	MemoryRegion ram;

	struct PCIMemoDevHdr reg_mmio;
	void *addr_ram;
	uint8_t cmd_result;
	uint8_t int_flag;

	int data_fd;
	uint32_t *list_base, *list_cur;
	uint32_t key, count;
};

OBJECT_DECLARE_SIMPLE_TYPE(PCIMemoDevState, PCI_MEMO_DEV)

static uint64_t pci_memodev_pio_read(void *opaque, hwaddr addr, unsigned size);
static void pci_memodev_pio_write(void *opaque, hwaddr addr, uint64_t val, unsigned size);
static uint64_t pci_memodev_mmio_read(void *opaque, hwaddr addr, unsigned size);
static void pci_memodev_mmio_write(void *opaque, hwaddr addr, uint64_t val, unsigned size);
static void pci_memodev_reset(PCIMemoDevState *ms);
static void do_command(PCIMemoDevState *ms, uint8_t cmd);

static const MemoryRegionOps pci_memodev_pio_ops = {
	.read       = pci_memodev_pio_read,
	.write      = pci_memodev_pio_write,
	.endianness = DEVICE_LITTLE_ENDIAN,
	.impl = {
		.min_access_size = 1,
		.max_access_size = 1,
	},
};

static const MemoryRegionOps pci_memodev_mmio_ops = {
	.read       = pci_memodev_mmio_read,
	.write      = pci_memodev_mmio_write,
	.endianness = DEVICE_LITTLE_ENDIAN,
	.impl = {
		.min_access_size = 1,
		.max_access_size = 4,
	},
};

static void pci_memodev_realize(PCIDevice *pci_dev, Error **errp) {
	PCIMemoDevState *ms = PCI_MEMO_DEV(pci_dev);
	uint8_t *pci_conf;

	tprintf("called\n");
	pci_conf = pci_dev->config;
	pci_conf[PCI_INTERRUPT_PIN] = 1;

	memory_region_init_io(&ms->portio, OBJECT(ms), &pci_memodev_pio_ops, ms, TYPE_PCI_MEMO_DEV"-portio", 4);
	pci_register_bar(pci_dev, 0, PCI_BASE_ADDRESS_SPACE_IO, &ms->portio);

	memory_region_init_io(&ms->mmio, OBJECT(ms), &pci_memodev_mmio_ops, ms, TYPE_PCI_MEMO_DEV"-mmio", 0x20);
	pci_register_bar(pci_dev, 1, PCI_BASE_ADDRESS_SPACE_MEMORY, &ms->mmio);

	memory_region_init_ram(&ms->ram, OBJECT(ms), TYPE_PCI_MEMO_DEV"-ram", PAGE_SIZE, &error_fatal);
	if(ms->prefetch_ram)
		pci_register_bar(pci_dev, 2, PCI_BASE_ADDRESS_SPACE_MEMORY | PCI_BASE_ADDRESS_MEM_PREFETCH | PCI_BASE_ADDRESS_MEM_TYPE_64, &ms->ram);
	ms->addr_ram = memory_region_get_ram_ptr(&ms->ram);

	ms->data_fd = -1;
}

static void pci_memodev_uninit(PCIDevice *pci_dev) {
	PCIMemoDevState *ms = PCI_MEMO_DEV(pci_dev);

	pci_memodev_reset(ms);
}

static void qdev_pci_memodev_reset(DeviceState *s) {
	PCIMemoDevState *ms = PCI_MEMO_DEV(s);

	pci_memodev_reset(ms);
}

static Property pci_memodev_properties[] = {
	DEFINE_PROP_BOOL("ram", PCIMemoDevState, prefetch_ram, true),
	DEFINE_PROP_UINT32("plimit", PCIMemoDevState, limit_pages, 0x100),
	DEFINE_PROP_END_OF_LIST(),
};

static void pci_memodev_class_init(ObjectClass *klass, void *data) {
	DeviceClass *dc = DEVICE_CLASS(klass);
	PCIDeviceClass *k = PCI_DEVICE_CLASS(klass);

	k->realize = pci_memodev_realize;
	k->exit = pci_memodev_uninit;
	k->vendor_id = MEMO_PCI_VENDOR_ID;
	k->device_id = MEMO_PCI_DEVICE_ID;
	k->revision = 0x00;
	k->class_id = PCI_CLASS_OTHERS;
	dc->desc = "SECCON CTF 2023 Challenge : Memo Persistent Device";
	set_bit(DEVICE_CATEGORY_MISC, dc->categories);
	dc->reset = qdev_pci_memodev_reset;
	device_class_set_props(dc, pci_memodev_properties);
}

static const TypeInfo pci_memodev_info = {
	.name          = TYPE_PCI_MEMO_DEV,
	.parent        = TYPE_PCI_DEVICE,
	.instance_size = sizeof(PCIMemoDevState),
	.class_init    = pci_memodev_class_init,
	.interfaces = (InterfaceInfo[]) {
		{ INTERFACE_CONVENTIONAL_PCI_DEVICE },
		{ },
	},
};

static void pci_memodev_register_types(void) {
	type_register_static(&pci_memodev_info);
}

type_init(pci_memodev_register_types)

static uint64_t pci_memodev_pio_read(void *opaque, hwaddr addr, unsigned size) {
	PCIMemoDevState *ms = opaque;
	tprintf("addr:%lx, size:%d\n", addr, size);

	switch(addr){
		case PORT_GET_RESULT:
			return ms->cmd_result;
		case PORT_GET_INTFLAG:
			return ms->int_flag;
	}

	return 0;
}

static void pci_memodev_pio_write(void *opaque, hwaddr addr, uint64_t val, unsigned size) {
	PCIMemoDevState *ms = opaque;
	PCIDevice *pci_dev = PCI_DEVICE(ms);
	tprintf("addr:%lx, size:%d, val:%lx\n", addr, size, val);

	switch(addr){
		case PORT_SET_COMMAND:
			ms->cmd_result = RESULT_INPROGRESS;
			do_command(ms, val);
			break;
		case PORT_DROP_IRQ:
			ms->int_flag = val;
			pci_irq_deassert(pci_dev);
			break;
	}
}

static uint64_t pci_memodev_mmio_read(void *opaque, hwaddr addr, unsigned size) {
	PCIMemoDevState *ms = opaque;
	const char *buf = (void*)&ms->reg_mmio;

	if(addr > sizeof(ms->reg_mmio))
		return 0;

	tprintf("addr:%lx, size:%d, %p\n", addr, size, &buf[addr]);

	return *(uint64_t*)&buf[addr];
}

static void pci_memodev_mmio_write(void *opaque, hwaddr addr, uint64_t val, unsigned size) {
	PCIMemoDevState *ms = opaque;
	char *buf = (void*)&ms->reg_mmio;

	if(addr > sizeof(ms->reg_mmio)) return;

	tprintf("addr:%lx, size:%d, val:%lx\n", addr, size, val);

	*(uint64_t*)&buf[addr] = (val & ((1UL << size*8) - 1)) | (*(uint64_t*)&buf[addr] & ~((1UL << size*8) - 1));
}

static void pci_memodev_reset(PCIMemoDevState *ms) {
	tprintf("called\n");

	bzero(&ms->reg_mmio, sizeof(ms->reg_mmio));
	g_free(ms->list_base);
	ms->list_base = ms->list_cur = NULL;
	ms->cmd_result = ms->int_flag = 0;
	if(ms->data_fd >= 0){
		close(ms->data_fd);
		ms->data_fd = -1;
	}
}

static void do_command(PCIMemoDevState *ms, uint8_t cmd){
	PCIDevice *pci_dev = PCI_DEVICE(ms);
	uint8_t result = RESULT_FAILED;
	char fname[24];

	switch(cmd){
		case CMD_STORE_GETKEY:
			tprintf("STORE_GETKEY (len:%x)\n", le32_to_cpu(ms->reg_mmio.len));

			if(ms->list_base || ms->data_fd >= 0)
				break;

			uint32_t count = le32_to_cpu(ms->reg_mmio.len);
			if(count > ms->limit_pages)
				break;

			if(!(ms->list_base = ms->list_cur = g_malloc(count*sizeof(uint32_t))))
				break;

			if(getrandom(&ms->key, sizeof(ms->key), 0) < 0)
				ms->key = random();
			ms->reg_mmio.key = cpu_to_le32(ms->key);

			if(snprintf(fname, sizeof(fname), "/tmp/mp_%08x-data", ms->key) < 0)
				break;
			if((ms->data_fd = open(fname, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR)) < 0)
				break;

			ms->count = count;
			result = RESULT_COMPLETE;
			break;

		case CMD_STORE_PAGE:
			tprintf("STORE_PAGE (pgoff:%d)\n", le32_to_cpu(ms->reg_mmio.pgoff));

			if(!ms->count || !ms->list_cur || ms->data_fd < 0)
				break;

			if(le64_to_cpu(ms->reg_mmio.sdma_addr) != DMA_MAPPING_ERROR){
				tprintf("dma_read (%lx -> %p)\n", le64_to_cpu(ms->reg_mmio.sdma_addr), ms->addr_ram);
				pci_dma_read(pci_dev, le64_to_cpu(ms->reg_mmio.sdma_addr), ms->addr_ram, PAGE_SIZE);
				ms->int_flag |= INT_SDMA;
			}

			if(write(ms->data_fd, ms->addr_ram, PAGE_SIZE) < 0)
				break;
			*ms->list_cur++ = le32_to_cpu(ms->reg_mmio.pgoff);
			ms->count--;
			ms->int_flag |= INT_WRITE_FILE;

			result = RESULT_COMPLETE;
			break;

		case CMD_STORE_FIN:
			tprintf("STORE_FIN\n");

			if(!ms->list_base || ms->data_fd < 0)
				break;

			if(snprintf(fname, sizeof(fname), "/tmp/mp_%08x", ms->key) < 0)
				goto ERR_STORE_FIN1;

			int idx_fd;
			if((idx_fd = open(fname, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR)) < 0)
				goto ERR_STORE_FIN1;
			if(write(idx_fd, ms->list_base, (void*)ms->list_cur - (void*)ms->list_base) < 0)
				goto ERR_STORE_FIN2;

			ms->count = 0;
			ms->int_flag |= INT_WRITE_FILE;
			result = RESULT_COMPLETE;
ERR_STORE_FIN2:
			close(idx_fd);
ERR_STORE_FIN1:
			ms->key = -1;
			close(ms->data_fd);
			ms->data_fd = -1;
			g_free(ms->list_base);
			ms->list_base = ms->list_cur = NULL;
			break;

		case CMD_LOAD_SETKEY:
			tprintf("LOAD_SETKEY\n");

			if(ms->list_base || ms->data_fd >= 0)
				break;

			if(snprintf(fname, sizeof(fname), "/tmp/mp_%08x-data", le32_to_cpu(ms->reg_mmio.key)) < 0)
				break;
			if((ms->data_fd = open(fname, O_RDONLY)) < 0)
				break;

			fname[16] = '\x00';
			if((idx_fd = open(fname, O_RDONLY)) < 0){
				close(ms->data_fd);
				ms->data_fd = -1;
				break;
			}
			size_t len = lseek(idx_fd, 0, SEEK_END);

			ms->reg_mmio.len = cpu_to_le32(len/sizeof(uint32_t));
			ms->list_base = ms->list_cur = g_malloc(len);

			if(pread(idx_fd, ms->list_base, len, 0) < 0){
				close(ms->data_fd);
				ms->data_fd = -1;
				g_free(ms->list_base);
				ms->list_base = NULL;
			}
			else {
				ms->int_flag |= INT_READ_FILE;
				result = RESULT_COMPLETE;
			}
			close(idx_fd);
			break;

		case CMD_LOAD_PAGE:
			tprintf("LOAD_PAGE\n");

			if(!ms->list_cur || ms->data_fd < 0)
				break;

			if(read(ms->data_fd, ms->addr_ram, PAGE_SIZE) < 0)
				break;
			ms->reg_mmio.pgoff = cpu_to_le32(*ms->list_cur++);
			ms->int_flag |= INT_READ_FILE;

			if(le64_to_cpu(ms->reg_mmio.sdma_addr) != DMA_MAPPING_ERROR){
				tprintf("dma_write (%lx <- %p)\n", le64_to_cpu(ms->reg_mmio.sdma_addr), ms->addr_ram);
				pci_dma_write(pci_dev, le64_to_cpu(ms->reg_mmio.sdma_addr), ms->addr_ram, PAGE_SIZE);
				ms->int_flag |= INT_SDMA;
			}

			result = RESULT_COMPLETE;
			break;

		case CMD_LOAD_FIN:
			tprintf("LOAD_FIN\n");

			if(!ms->list_base || ms->data_fd < 0)
				break;

			close(ms->data_fd);
			ms->data_fd = -1;
			g_free(ms->list_base);
			ms->list_base = NULL;

			result = RESULT_COMPLETE;
			break;
	}

	ms->cmd_result = result;
	ms->int_flag |= INT_CMD;
	pci_irq_assert(pci_dev);
}
