#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/mailbox_client.h>
#include <acpi/pcc.h>
#include <acpi/cppc_acpi.h>
#define pr_fmt(fmt)	"ACPI PCCT: " fmt

#define PCC_DRIVER              "pcct_driver"

struct pcct_type4_data {
	struct pcc_mbox_chan *pcc_chan;
	void __iomem *pcc_comm_addr;
	struct completion done;
	struct mbox_client cl;
	//struct acpi_pcc_info ctx;
};

static void pcc_rx_callback(struct mbox_client *cl, void *msg)
{
	// Implement the callback function for handling received messages
}

struct cppc_workaround_oem_info {
	char oem_id[ACPI_OEM_ID_SIZE + 1];
	char oem_table_id[ACPI_OEM_TABLE_ID_SIZE + 1];
	u32 oem_revision;
};

static struct cppc_workaround_oem_info wa_info[] = {
	{
		.oem_id		= "HISI  ",
		.oem_table_id	= "HIP07   ",
		.oem_revision	= 0,
	}, {
		.oem_id		= "HISI  ",
		.oem_table_id	= "HIP08   ",
		.oem_revision	= 0,
	}
};


static void cppc_check_hisi_workaround(void)
{
	struct acpi_table_header *tbl;
	acpi_status status = AE_OK;
	int i;

	status = acpi_get_table(ACPI_SIG_PCCT, 0, &tbl);
	if (ACPI_FAILURE(status) || !tbl)
		return;

	for (i = 0; i < ARRAY_SIZE(wa_info); i++) {
		if (!memcmp(wa_info[i].oem_id, tbl->oem_id, ACPI_OEM_ID_SIZE) &&
		    !memcmp(wa_info[i].oem_table_id, tbl->oem_table_id, ACPI_OEM_TABLE_ID_SIZE) &&
		    wa_info[i].oem_revision == tbl->oem_revision) {
			break;
		}
	}

	acpi_put_table(tbl);
}


static int pcct_type4_probe(struct platform_device *pdev)
{
	struct pcct_type4_data *data;
	struct mbox_client mbox_cl;
	struct pcc_mbox_chan *pcc_chan;
	uint8_t read_buffer[256];

    cppc_check_hisi_workaround();

	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	memset(&mbox_cl, 0, sizeof(mbox_cl));
	mbox_cl.dev = &pdev->dev;
	mbox_cl.rx_callback = pcc_rx_callback;
	mbox_cl.knows_txdone = true;

    pr_err("mbox_cl.knows_txdone = %d\n" , mbox_cl.knows_txdone);

	pcc_chan = pcc_mbox_request_channel(&mbox_cl, 4); // Use the appropriate subspace_id
	if (IS_ERR(data->pcc_chan)) {
		dev_err(&pdev->dev, "Failed to find PCC channel for subspace\n");
		return PTR_ERR(data->pcc_chan);
	}

	data->pcc_chan = pcc_chan;

	if (!pcc_chan->mchan->mbox->txdone_irq) {
		dev_err(&pdev->dev, "This channel does not support interrupt.\n");
		return -EINVAL;
	}

	data->pcc_comm_addr = acpi_os_ioremap(pcc_chan->shmem_base_addr, pcc_chan->shmem_size);
	if (!data->pcc_comm_addr) {
		dev_err(&pdev->dev, "Failed to ioremap PCC comm region mem\n");
		return -ENOMEM;
	}
    pr_err("pcc_chan name = %x\n" , data->pcc_comm_addr);
	// Read data from shared memory
	memcpy_fromio(read_buffer, data->pcc_comm_addr, sizeof(read_buffer));
	dev_info(&pdev->dev, "Data read from shared memory: %*ph\n", (int)sizeof(read_buffer), read_buffer);

	platform_set_drvdata(pdev, data);
	return 0;
}

static int pcct_type4_remove(struct platform_device *pdev)
{
	struct pcct_type4_data *data = platform_get_drvdata(pdev);

	pcc_mbox_free_channel(data->pcc_chan);
	return 0;
}

static struct platform_driver pcct_type4_driver = {
    .probe = pcct_type4_probe,
	.remove = pcct_type4_remove,
	.driver = {
		.name = PCC_DRIVER,
        .owner = THIS_MODULE,
	},
};

static int __init pcct_init(void)
{
    int ret = 0;


    printk(KERN_ALERT "Demo driver init function called\n");
    ret = platform_driver_register(&pcct_type4_driver);
    if (ret) {
        printk(KERN_ALERT "Failed to register demo driver\n");
    }

   // ret = platform_device_register(&demo_device);
    device = platform_device_register_simple(PCC_DRIVER, -1, NULL, 0);
    printk(KERN_ALERT "Demo driver loaded successfully\n");
    return ret;
}


static void __exit pcct_exit(void)
{
    printk(KERN_ALERT "Demo driver exit function called\n");
    platform_driver_unregister(&demo_driver);
}

module_init(pcct_init);
module_exit(pcct_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("PCCT Type 4 Driver");
MODULE_AUTHOR("Perry Yuan");
