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


struct pcct_type4_data {
	struct pcc_mbox_chan *pcc_chan;
	void __iomem *pcc_comm_addr;
	struct completion done;
	struct mbox_client cl;
	struct acpi_pcc_info ctx;
};


static int pcct_type4_proc_show(struct seq_file *m, void *v)
{
	struct pcct_type4_data *data = (struct pcct_type4_data *)m->private;

	seq_printf(m, "PCC Communication Region Base Address: 0x%llx\n", (unsigned long long)data->pcc_comm_addr);
	seq_printf(m, "PCC Communication Region Size: %zu\n", data->pcc_chan->shmem_size);

	return 0;
}

static int pcct_type4_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, pcct_type4_proc_show, inode->i_private);
}


static const struct file_operations pcct_type4_proc_fops = {
	.open		= pcct_type4_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static struct proc_dir_entry *proc_entry;

static void pcc_rx_callback(struct mbox_client *cl, void *msg)
{
	// Implement the callback function for handling received messages
}

static int pcct_type4_probe(struct platform_device *pdev)
{
	struct pcct_type4_data *data;
	struct pcc_mbox_chan *pcc_chan;
	uint8_t read_buffer[256];

	proc_entry = proc_create_data("pcct_type4", 0, NULL, &pcct_type4_proc_fops, data);
	if (!proc_entry) {
		dev_err(&pdev->dev, "Failed to create proc file\n");
		return -ENOMEM;
	}


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

	pcc_chan = data->pcc_chan;

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
