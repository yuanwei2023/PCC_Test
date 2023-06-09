#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/mailbox_client.h>

#include <linux/kernel.h>
#include <linux/acpi.h>
#include <linux/completion.h>
#include <linux/idr.h>
#include <linux/io.h>

#include <acpi/pcc.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/mailbox_client.h>
#include <linux/pcc.h>
#include <linux/completion.h>
#include <linux/acpi.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>

struct pcct_type4_data {
	struct pcc_mbox_chan *pcc_chan;
	void __iomem *pcc_comm_addr;
	struct completion done;
	struct mbox_client cl;
	struct acpi_pcc_info ctx;
};

static void pcc_rx_callback(struct mbox_client *cl, void *msg)
{
	// Implement the callback function for handling received messages
}

static int pcct_type4_proc_show(struct seq_file *m, void *v)
{
	struct pcct_type4_data *data = m->private;

	// You can customize this to display the content of the shared memory or any other information
	seq_printf(m, "Shared Memory Base Address: %p\n", (void *)data->pcc_chan->shmem_base_addr);
	seq_printf(m, "Shared Memory Size: %zu\n", data->pcc_chan->shmem_size);

	return 0;
}

static int pcct_type4_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, pcct_type4_proc_show, PDE_DATA(inode));
}

static const struct proc_ops pcct_type4_proc_ops = {
	.proc_open	= pcct_type4_proc_open,
	.proc_read	= seq_read,
	.proc_lseek	= seq_lseek,
	.proc_release	= single_release,
};

static int pcct_type4_probe(struct platform_device *pdev)
{
	struct pcct_type4_data *data;
	struct pcc_mbox_chan *pcc_chan;
	uint8_t read_buffer[256];

	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->cl.dev = &pdev->dev;
	data->cl.rx_callback = pcc_rx_callback;
	data->cl.knows_txdone = true;

	data->pcc_chan = pcc_mbox_request_channel(&data->cl, 0); // Use the appropriate subspace_id
	if (IS_ERR(data->pcc_chan)) {
		dev_err(&pdev->dev, "Failed to find PCC channel for subspace\n");
		return PTR_ERR(data->pcc_chan);
	}

	pcc_chan = data->pcc_chan->con_priv;

	if (!pcc_chan->mchan->mbox->txdone_irq) {
		dev_err(&pdev->dev, "This channel does not support interrupt.\n");
		return -EINVAL;
	}

	data->pcc_comm_addr = devm_ioremap(&pdev->dev, pcc_chan->shmem_base_addr, pcc_chan->shmem_size);
	if (!data->pcc_comm_addr) {
		dev_err(&pdev->dev, "Failed to ioremap PCC comm region mem\n");
		return -ENOMEM;
	}

	init_completion(&data->done);

	// Read data from shared memory
	memcpy_fromio(read_buffer, data->pcc_comm_addr, sizeof(read_buffer));
	dev_info(&pdev->dev, "Data read from shared memory: %*ph\n", (int)sizeof(read_buffer),
	dev_info(&pdev->dev, "Data read from shared memory: %*ph\n", (int)sizeof(read_buffer), read_buffer);

	platform_set_drvdata(pdev, data);
	return 0;
}

static int pcct_type4_remove(struct platform_device *pdev)
{
	struct pcct_type4_data *data = platform_get_drvdata(pdev);

	if (proc_entry)
		proc_remove(proc_entry);
	
    pcc_mbox_free_channel(data->pcc_chan);
	return 0;
}

static struct platform_driver pcct_type4_driver = {
	.probe = pcct_type4_probe,
	.remove = pcct_type4_remove,
	.driver = {
		.name = "pcct_type4",
	},
};

module_platform_driver(pcct_type4_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("PCCT Type 4 Driver");
MODULE_AUTHOR("Perry Yuan");
