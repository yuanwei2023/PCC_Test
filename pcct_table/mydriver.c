#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/mailbox_client.h>
#include <acpi/pcc.h>
#include <acpi/cppc_acpi.h>

static struct platform_device *device;

struct cppc_workaround_oem_info {
        char oem_id[ACPI_OEM_ID_SIZE + 1];
        char oem_table_id[ACPI_OEM_TABLE_ID_SIZE + 1];
        u32 oem_revision;
};

static struct cppc_workaround_oem_info wa_info[] = {
        {
                .oem_id         = "HISI  ",
                .oem_table_id   = "HIP07   ",
                .oem_revision   = 0,
        }, {
                .oem_id         = "HISI  ",
                .oem_table_id   = "HIP08   ",
                .oem_revision   = 0,
        }
};

static void cppc_check_hisi_workaround(void)
{
        struct acpi_table_header *tbl;
        acpi_status status = AE_OK;
        int i;

        printk(KERN_ALERT "cppc_check_hisi_workaround called 111 \n");
        //status = acpi_get_table(ACPI_SIG_PCCT, 0, &tbl);
        status = acpi_get_table(ACPI_SIG_PCCT, 0, &tbl);
        if (ACPI_FAILURE(status) || !tbl) {
                pr_err("acpi_get_table failed!\n");
                return;
        }

        for (i = 0; i < ARRAY_SIZE(wa_info); i++) {
                if (!memcmp(wa_info[i].oem_id, tbl->oem_id, ACPI_OEM_ID_SIZE) &&
                    !memcmp(wa_info[i].oem_table_id, tbl->oem_table_id, ACPI_OEM_TABLE_ID_SIZE) &&
                    wa_info[i].oem_revision == tbl->oem_revision) {
                        break;
                }
        }
        printk(KERN_ALERT "cppc_check_hisi_workaround called .oem_id = %s,.oem_table_id = %s \n",tbl->oem_id, tbl->oem_table_id);

        acpi_put_table(tbl);
}

static int demo_probe(struct platform_device *pdev)
{
    printk(KERN_ALERT "Demo driver probe function called\n");
    cppc_check_hisi_workaround();
    return 0;
}

static int demo_remove(struct platform_device *pdev)
{
    printk(KERN_ALERT "Demo driver remove function called\n");
    return 0;
}

static struct platform_device demo_device = {
    .name = "demo_device",
    .id = -1, /* auto-assign id */
};

#define PCC_DRIVER              "pcct_driver"
static struct platform_driver demo_driver = {
    .driver = {
        .name = PCC_DRIVER,
        .owner = THIS_MODULE,
    },
    .probe = demo_probe,
    .remove = demo_remove,
};

static int __init demo_init(void)
{
    int ret = 0;


    printk(KERN_ALERT "Demo driver init function called\n");
    ret = platform_driver_register(&demo_driver);
    if (ret) {
        printk(KERN_ALERT "Failed to register demo driver\n");
    }

   // ret = platform_device_register(&demo_device);
    device = platform_device_register_simple(PCC_DRIVER, -1, NULL, 0);
    printk(KERN_ALERT "Demo driver loaded successfully\n");
    return ret;
}

static void __exit demo_exit(void)
{
    printk(KERN_ALERT "Demo driver exit function called\n");
    platform_driver_unregister(&demo_driver);
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Demo driver");

