#include <linux/pci.h>
#include <asm/dma.h>
#include "kmemo/memo.h"
#include "pci.h"

#define PORT_OUT(addr, val) (outb((val), pinfo.pio_base + (addr)))
#define PORT_IN(addr)       (inb(pinfo.pio_base + (addr)))

#define PORT_OUT_WAITCOND(addr, val, q, f, cond) { (f) = 0; PORT_OUT((addr), (val)); wait_event_interruptible((q), ((f) & (cond))); } while(0)

static struct pcidrv_info pinfo = {};
static uint8_t event_flag;
static wait_queue_head_t event_queue;

static bool use_dma = true;
module_param(use_dma, bool, S_IRUGO);

static struct pci_driver memo_pci_driver;

int reg_pcidrv(void){
	return pci_register_driver(&memo_pci_driver);
}

void unreg_pcidrv(void){
	pci_unregister_driver(&memo_pci_driver);
}

static irqreturn_t pcidrv_handler(int irq, void *dev_id) {
	uint8_t int_flag = PORT_IN(PORT_GET_INTFLAG);
	uint8_t result   = PORT_IN(PORT_GET_RESULT);

	if(!int_flag || result == RESULT_INPROGRESS)
		return IRQ_NONE;

	if(int_flag & INT_CMD){
		switch(result){
			case RESULT_COMPLETE:
				event_flag |= EVENT_COMPLETE;
				break;
			case RESULT_FAILED:
				event_flag |= EVENT_FAILED;
				break;
		}
	}
	if(int_flag & INT_READ_FILE)
		event_flag |= EVENT_FILE_IO;
	if(int_flag & INT_WRITE_FILE)
		event_flag |= EVENT_FILE_IO;
	if(int_flag & INT_SDMA)
		event_flag |= EVENT_SDMA_DONE;

	PORT_OUT(PORT_DROP_IRQ, 0);

	return IRQ_HANDLED;
}

static int pcidrv_probe(struct pci_dev *pdev, const struct pci_device_id *id) {
	if(pinfo.pdev)
		return -1;

	if(pci_enable_device(pdev))
		return -1;

	if(request_irq(pdev->irq, pcidrv_handler, IRQF_SHARED, PCI_DRIVER_NAME, pdev))
		goto ERR1;

	pinfo.pio_base    = pci_resource_start(pdev, 0);
	pinfo.pio_length  = pci_resource_len(pdev, 0);
	pinfo.pio_flags   = pci_resource_flags(pdev, 0);
	printk(KERN_INFO "pio_base: %lx, pio_length: %lx, pio_flags: %lx\n", pinfo.pio_base, pinfo.pio_length, pinfo.pio_flags);
	if(!(pinfo.pio_flags & IORESOURCE_IO))
		goto ERR1;
	if(pci_request_region(pdev, 0, PCI_DRIVER_NAME))
		goto ERR1;

	pinfo.mmio_base   = pci_resource_start(pdev, 1);
	pinfo.mmio_length = pci_resource_len(pdev, 1);
	pinfo.mmio_flags  = pci_resource_flags(pdev, 1);
	printk(KERN_INFO "mmio_base: %lx, mmio_length: %lx, mmio_flags: %lx\n", pinfo.mmio_base, pinfo.mmio_length, pinfo.mmio_flags);
	if(!(pinfo.mmio_flags & IORESOURCE_MEM))
		goto ERR2;
	if(pci_request_region(pdev, 1, PCI_DRIVER_NAME))
		goto ERR2;
	pinfo.mmio = ioremap(pinfo.mmio_base, pinfo.mmio_length);

	pinfo.ram_base    = pci_resource_start(pdev, 2);
	pinfo.ram_length  = pci_resource_len(pdev, 2);
	pinfo.ram_flags   = pci_resource_flags(pdev, 2);
	if(pinfo.ram_base){
		printk(KERN_INFO "ram_base: %lx, ram_length: %lx, ram_flags: %lx\n", pinfo.ram_base, pinfo.ram_length, pinfo.ram_flags);
		if(!(pinfo.mmio_flags & IORESOURCE_MEM) || !(pinfo.ram_flags & IORESOURCE_PREFETCH) || pinfo.ram_length < PAGE_SIZE)
			goto ERR3;
		if(pci_request_region(pdev, 2, PCI_DRIVER_NAME))
			goto ERR3;
		pinfo.ram = ioremap(pinfo.ram_base, pinfo.ram_length);
	}

	if(!pinfo.ram){
		printk(KERN_INFO "BAR2 RAM not found\n");
		use_dma = true;
	}
	if(use_dma){
		printk(KERN_INFO "DMA Enabled\n");
		dma_set_mask(&pdev->dev, DMA_BIT_MASK(32));
		pci_set_master(pdev);
	}

	init_waitqueue_head(&event_queue);

	pinfo.pdev = pdev;

	return 0;

ERR3:
	iounmap(pinfo.mmio);
	pci_release_region(pdev, 1);
ERR2:
	pci_release_region(pdev, 0);
ERR1:
	pci_disable_device(pdev);
	return -1;
}

static void pcidrv_remove(struct pci_dev *pdev) {
	if(pdev != pinfo.pdev)
		return;

	pci_clear_master(pdev);

	free_irq(pdev->irq, pdev);

	if(pinfo.ram){
		iounmap(pinfo.ram);
		pci_release_region(pdev, 2);
	}

	iounmap(pinfo.mmio);
	pci_release_region(pdev, 1);
	pci_release_region(pdev, 0);
	pci_disable_device(pdev);

	pinfo.pdev = NULL;
}

static struct pci_device_id pcidrv_ids[] = {
	{ PCI_DEVICE(MEMO_PCI_VENDOR_ID, MEMO_PCI_DEVICE_ID) },
	{},
};

static struct pci_driver memo_pci_driver = {
	.name     = PCI_DRIVER_NAME,
	.id_table = pcidrv_ids,
	.probe    = pcidrv_probe,
	.remove   = pcidrv_remove,
};

int pcidrv_prepare_store(const uint32_t count, uint32_t *p_key){
	if(!pinfo.pdev)
		return -ENODEV;

	pinfo.mmio->len = count;
	PORT_OUT_WAITCOND(PORT_SET_COMMAND, CMD_STORE_GETKEY, event_queue, event_flag, EVENT_ANY);

	if(event_flag & EVENT_FAILED)
		return -EIO;

	*p_key = pinfo.mmio->key;

	return 0;
}

int pcidrv_store_page(pgoff_t pgoff, void *page){
	if(!pinfo.pdev)
		return -ENODEV;

	dma_addr_t sdma_addr = use_dma ? dma_map_single(&pinfo.pdev->dev, page, PAGE_SIZE, DMA_TO_DEVICE) : DMA_MAPPING_ERROR;
	if(sdma_addr == DMA_MAPPING_ERROR){
		if(!pinfo.ram)
			return -EIO;
		memcpy(pinfo.ram, page, pinfo.ram_length);
	}

	pinfo.mmio->sdma_addr = sdma_addr;
	pinfo.mmio->pgoff = pgoff;
	PORT_OUT_WAITCOND(PORT_SET_COMMAND, CMD_STORE_PAGE, event_queue, event_flag, EVENT_FILE_IO|EVENT_CMD);

	if(event_flag & EVENT_SDMA_DONE)
		dma_unmap_single(&pinfo.pdev->dev, sdma_addr, PAGE_SIZE, DMA_TO_DEVICE);

	if(event_flag & EVENT_FAILED)
		return -EIO;

	return 0;
}

void pcidrv_cleanup_store(void){
	if(!pinfo.pdev)
		return;

	PORT_OUT_WAITCOND(PORT_SET_COMMAND, CMD_STORE_FIN, event_queue, event_flag, EVENT_FILE_IO);
}

int pcidrv_prepare_load(const uint32_t key, int *p_count){
	if(!pinfo.pdev)
		return -ENODEV;

	pinfo.mmio->key = key;
	PORT_OUT_WAITCOND(PORT_SET_COMMAND, CMD_LOAD_SETKEY, event_queue, event_flag, EVENT_FILE_IO|EVENT_CMD);

	if(event_flag & EVENT_FAILED)
		return -EIO;

	*p_count = pinfo.mmio->len;

	return 0;
}

int pcidrv_load_page(int index, pgoff_t *p_pgoff, void *page){
	if(!pinfo.pdev)
		return -ENODEV;

	dma_addr_t sdma_addr = use_dma ? dma_map_single(&pinfo.pdev->dev, page, PAGE_SIZE, DMA_FROM_DEVICE) : DMA_MAPPING_ERROR;
	if(sdma_addr == DMA_MAPPING_ERROR && !pinfo.ram)
		return -EIO;

	pinfo.mmio->sdma_addr = sdma_addr;
	PORT_OUT_WAITCOND(PORT_SET_COMMAND, CMD_LOAD_PAGE, event_queue, event_flag, EVENT_FILE_IO|EVENT_CMD);

	if(event_flag & EVENT_SDMA_DONE)
		dma_unmap_single(&pinfo.pdev->dev, sdma_addr, PAGE_SIZE, DMA_FROM_DEVICE);
	else
		memcpy(page, pinfo.ram, pinfo.ram_length);

	if(event_flag & EVENT_FAILED)
		return -EIO;

	*p_pgoff = pinfo.mmio->pgoff;

	return 0;
}

void pcidrv_cleanup_load(void){
	if(!pinfo.pdev)
		return;

	PORT_OUT_WAITCOND(PORT_SET_COMMAND, CMD_LOAD_FIN, event_queue, event_flag, EVENT_ANY);
}
