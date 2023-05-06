#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mailbox_controller.h>

#define DRIVER_NAME "pcc_test"

static int pcc_test_probe(struct platform_device *pdev)
{
    int ret = 0;
    struct mbox_client cl;
    struct pcc_mbox_chan *chan;
    int subspace_id = 4; // PCCT subspace type 4

    // Initialize mailbox client
    memset(&cl, 0, sizeof(cl));
    cl.dev = &pdev->dev;
    cl.chan_shutdown = false;
    cl.rx_callback = pcc_test_rx_callback;
    cl.knows_txdone = true;

    // Request mailbox channel for PCCT subspace type 4
    chan = pcc_mbox_request_channel(&cl, subspace_id);
    if (IS_ERR(chan)) {
        dev_err(&pdev->dev, "Failed to request mailbox channel\n");
        ret = PTR_ERR(chan);
        goto error;
    }

    // Register IRQ handler for the platform interrupt
    ret = devm_request_irq(&pdev->dev, chan->irq, pcc_test_irq,
                           0, DRIVER_NAME, chan);
    if (ret < 0) {
        dev_err(&pdev->dev, "Failed to register IRQ handler\n");
        goto error;
    }

    return 0;

error:
    if (!IS_ERR(chan))
        pcc_mbox_free_channel(chan);
    return ret;
}

static void pcc_test_remove(struct platform_device *pdev)
{
    struct mbox_client *cl = dev_get_drvdata(&pdev->dev);
    struct pcc_mbox_chan *chan = cl->chan;

    // Free mailbox channel
    pcc_mbox_free_channel(chan);
}

static irqreturn_t pcc_test_irq(int irq, void *data)
{
    struct pcc_mbox_chan *chan = data;
    // Handle interrupt and process update data received through mailbox channel
    ...
    return IRQ_HANDLED;
}

static int pcc_test_rx_callback(struct mbox_client *cl, void *data)
{
    struct pcc_mbox_chan *chan = cl->chan;
    // Process update data received through mailbox channel
    ...
    return 0;
}

static struct platform_driver pcc_test_driver = {
    .driver = {
        .name = DRIVER_NAME,
    },
    .probe = pcc_test_probe,
    .remove = pcc_test_remove,
};

module_platform_driver(pcc_test_driver);
MODULE_LICENSE("GPL");
