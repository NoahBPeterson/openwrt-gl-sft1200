#include <linux/module.h>
#include "start_core.h"
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/r4kcache.h>
#include <linux/firmware.h>
#include <linux/device.h>
#define FIRMWARE_SZ (CONFIG_FIRMWARE_SIZE * 1024)
#define SFAX8_DDR_BASE 0
#define BUF_SIZE (PAGE_SIZE)
#define MAX_TASKS 2
#define FIRMWARE_LOAD_OFFSET (CONFIG_FIRMWARE_LOAD_BASE + 0x0)
#define SF_TASK_LB 0
#define SF_TASK_HB 1
#define LMAC_SINGLE_BIN 0
static struct mutex g_mutex;
static LIST_HEAD(task_list_head);
static struct task_info *get_task_by_id(int task_id)
{
    struct task_info *cur;
    struct list_head *head;
    list_for_each(head, &task_list_head)
    {
        cur = list_entry(head, struct task_info, list);
        if (cur->task_id == task_id) {
            return cur;
        }
    }
    return NULL;
}
void task_dump_registers(int task_id, uint32_t fexception_base)
{
    unsigned int base_addr = fexception_base;
    unsigned int *backtrace = (void *)((unsigned int)(base_addr) - 0x400);
    unsigned int *gpr = (void *)((unsigned int)(base_addr) - 0x200), i, delta = 0;
    unsigned int *cp0_regs = (void *)((base_addr) - 0x100);
    int depth = 0;
    printk("umac %s-baseaddr=0x%x backtrace=0x%x gpr=0x%x cpu=0x%x\n", __func__,
            base_addr, (unsigned int)backtrace, (unsigned int)gpr, (unsigned int)cp0_regs);
    for (i = 0; i < 32; i++) {
        if (i == 26 || i == 27) {
            delta++;
            continue;
        }
        if (i % 4 == 0)
            printk(KERN_CONT "\n$%2d:\t", i);
        printk(KERN_CONT "%08x\t", gpr[i - delta]);
    }
    printk("epc: %08x\n", cp0_regs[2]);
    printk("ra: %08x\n", gpr[29]);
    printk("Status: %08x\n", cp0_regs[0]);
    printk("Cause: %08x\n", cp0_regs[1]);
    printk("BadVA: %08x\n", cp0_regs[3]);
    printk("Prid: %08x\n", cp0_regs[4]);
    printk("Call Trace:\n");
    while (*backtrace != 0 && depth < 100) {
        printk("[<0x%x>] <-\n", *backtrace);
        backtrace++;
        i++;
    }
}
EXPORT_SYMBOL(task_dump_registers);
#define LMAC_HEADER_LEN 128
#define LMAC_START_GP_OFFSET 0x8
#define LMAC_START_BAND_OFFSET (LMAC_START_GP_OFFSET + 0x4)
#define MIPS_RELOC 0x3
#define LMAC_EXCEPTION_OFFSET (0x22C)
#define LMAC_SHARE_MEM_BASE 0xb1000000
#define LMAC_SHARE_MEM_HIGH_OFFSET 0x400000
#define DEBUG_LOAD_TASK(a...) do {} while (0)
int load_task(char *path, int task_id, struct device *device)
{
    const struct firmware *fw = NULL;
    int err = 0;
    struct task_info *node;
    unsigned long addr = SFAX8_DDR_BASE + FIRMWARE_LOAD_OFFSET;
    int size = 0;
    bool add = true;
#if LMAC_SINGLE_BIN
    char header[LMAC_HEADER_LEN];
#endif
    if (FIRMWARE_SZ == 0) {
        printk("firmware size is set to zero, exit here.\n");
        return -EINVAL;
    }
    if (IS_ERR_OR_NULL(path)) {
        pr_err("Invalid path(%s) argument in %s\n", path ? path : "NULL", __func__);
        return -EINVAL;
    }
    if ((task_id < 0) || (task_id >= MAX_TASKS)) {
        pr_err("Invalid task_id(%d) argument in %s\n", task_id, __func__);
        return -EINVAL;
    }
    printk("%s, path : %s, task_id : %d\n", __func__, path, task_id);
    mutex_lock(&g_mutex);
    node = get_task_by_id(task_id);
    if (node) {
        if ((node->state == SF_TASK_RUNNING)) {
            pr_err("Target is already running.\n");
            mutex_unlock(&g_mutex);
            return -EBUSY;
        }
        add = false;
    } else {
        node = kmalloc(sizeof(struct task_info), GFP_KERNEL);
        if (!node) {
            pr_err("oom happen!\n");
            mutex_unlock(&g_mutex);
            return -ENOMEM;
        }
    }
    err = request_firmware(&fw, path, device);
    if(err)
    {
        printk("Can't get firmware file, path : %s!\n", path);
        kfree(node);
        mutex_unlock(&g_mutex);
        return -EINVAL;
    }
    node->entry_addr = addr + task_id * FIRMWARE_SZ / MAX_TASKS;
    addr = node->entry_addr;
 printk("node->entry_addr=%lx node=%p\n", node->entry_addr, node);
    blast_scache_range(CKSEG0ADDR(node->entry_addr),
                    CKSEG0ADDR(node->entry_addr) + FIRMWARE_SZ / MAX_TASKS);
    size = 0;
#if LMAC_SINGLE_BIN
 memcpy(header, fw->data, LMAC_HEADER_LEN);
    size = LMAC_HEADER_LEN;
#endif
    memcpy((void *)(CKSEG1ADDR(addr)), (void*)((unsigned int)fw->data + size), (unsigned int)fw->size - size);
    release_firmware(fw);
#if LMAC_SINGLE_BIN
    {
        unsigned long _monitor_flash_addr, _got_addr, _num_got_entries, _reset_vector, _gp, __rel_dyn_start, __rel_dyn_end,
                        _start_exception_text, _end_exception_text, _ftext_exception;
        int tmp;
        int cnt = 0;
        int offset;
        int *gop_p = NULL;
        int *dyn_p = NULL;
        int *dyn_end = NULL;
        header[8] = 0;
        tmp = kstrtoul(&header[0], 16, &_monitor_flash_addr);
        header[17] = 0;
        tmp = kstrtoul(&header[9], 16, &_got_addr);
        header[26] = 0;
        tmp = kstrtoul(&header[18], 16, &_num_got_entries);
        header[35] = 0;
        tmp = kstrtoul(&header[27], 16, &_reset_vector);
        header[44] = 0;
        tmp = kstrtoul(&header[36], 16, &_gp);
        DEBUG_LOAD_TASK("1=%x 2=%x 3=%x 4=%x 5=%x\n", _monitor_flash_addr, _got_addr, _num_got_entries, _reset_vector, _gp);
        header[53] = 0;
        tmp = kstrtoul(&header[45], 16, &__rel_dyn_start);
        header[62] = 0;
        tmp = kstrtoul(&header[54], 16, &__rel_dyn_end);
        DEBUG_LOAD_TASK("__rel_dyn_start=%x __rel_dyn_end=%x\n", __rel_dyn_start, __rel_dyn_end);
        header[71] = 0;
        tmp = kstrtoul(&header[63], 16, &_start_exception_text);
        header[80] = 0;
        tmp = kstrtoul(&header[72], 16, &_end_exception_text);
        header[89] = 0;
        tmp = kstrtoul(&header[81], 16, &_ftext_exception);
        DEBUG_LOAD_TASK("_start_exception_text=%x _end_exception_text=%x _ftext_exception=%x\n", _start_exception_text, _end_exception_text, _ftext_exception);
        offset = (int)(CKSEG0ADDR(node->entry_addr)) - (int)_monitor_flash_addr;
        gop_p = (int *)(CKSEG1ADDR(_got_addr + offset));
        DEBUG_LOAD_TASK("update-got base=%x\n", gop_p);
        gop_p += 2;
        cnt = 2;
        while (cnt < _num_got_entries) {
            tmp = readl((void *)gop_p);
            if ((tmp & LMAC_SHARE_MEM_BASE) != LMAC_SHARE_MEM_BASE) {
                writel(tmp + offset, (void *)(CKSEG1ADDR(gop_p)));
            } else {
                if (task_id == SF_TASK_HB) {
                    writel(tmp + LMAC_SHARE_MEM_HIGH_OFFSET, (void *)(CKSEG1ADDR(gop_p)));
                }
            }
            gop_p++;
            cnt++;
        }
        dyn_p = (int *)(CKSEG1ADDR(__rel_dyn_start + offset));
        dyn_end = (int *)(CKSEG1ADDR(__rel_dyn_end + offset));
        dyn_p += 2;
        while (dyn_p < dyn_end) {
            int rinfo = readl((void *)((int)dyn_p - 4));
            if ((rinfo & MIPS_RELOC) == MIPS_RELOC) {
                int linfo_p = readl((void *)((int)dyn_p - 8));
                int tmp = 0;
                linfo_p += offset;
                tmp = readl((void *)linfo_p);
                if (((int)tmp & LMAC_SHARE_MEM_BASE) != LMAC_SHARE_MEM_BASE) {
                    writel(tmp + offset, (void *)(CKSEG1ADDR(linfo_p)));
                    DEBUG_LOAD_TASK("write dyn addr=%x value=%x old=%x\n", linfo_p, tmp + offset, tmp);
                } else {
                    DEBUG_LOAD_TASK("skip sharemem dyn %x value=%x\n", (int)linfo_p, tmp);
                    if (task_id == SF_TASK_HB) {
                        writel(tmp + LMAC_SHARE_MEM_HIGH_OFFSET, (void *)(CKSEG1ADDR(linfo_p)));
                    }
                }
            }
            dyn_p += 2;
        }
        node->gp_addr = _gp + offset;
        memcpy((void *)(CKSEG1ADDR(_ftext_exception + offset)), (void *)(CKSEG1ADDR(_start_exception_text + offset)), _end_exception_text - _start_exception_text);
        writel(node->gp_addr, (void *)(CKSEG1ADDR(_ftext_exception + offset + LMAC_EXCEPTION_OFFSET)));
        DEBUG_LOAD_TASK("write addr=%x gp value=%x\n", CKSEG1ADDR(_ftext_exception + offset + LMAC_EXCEPTION_OFFSET), node->gp_addr);
    }
    writel(node->gp_addr, (void *)(CKSEG1ADDR(node->entry_addr + LMAC_START_GP_OFFSET)));
    DEBUG_LOAD_TASK("read start gp=%x\n", readl((void *)(CKSEG1ADDR(node->entry_addr + LMAC_START_GP_OFFSET))));
#endif
    writel(task_id, (void *)(CKSEG1ADDR(node->entry_addr + LMAC_START_BAND_OFFSET)));
    blast_inv_scache_range(CKSEG0ADDR(node->entry_addr),
                    CKSEG0ADDR(node->entry_addr) + FIRMWARE_SZ / MAX_TASKS);
    node->task_id = task_id;
    node->state = SF_TASK_READY;
 printk("task id=%d state=%d\n", task_id, node->state);
    plat_load_task(node);
    if (add)
        list_add(&node->list, &task_list_head);
    mutex_unlock(&g_mutex);
    return task_id;
}
EXPORT_SYMBOL(load_task);
int start_task(int task_id)
{
    struct task_info *node;
    printk("%s, %d\n", __func__, task_id);
    mutex_lock(&g_mutex);
    node = get_task_by_id(task_id);
    if (!node) {
        pr_err("%s : Task empty. Please run load_task first\n",
                        __func__);
        mutex_unlock(&g_mutex);
        return -EINVAL;
    }
    if (node->state == SF_TASK_RUNNING) {
        pr_warn("%s : Task is already running.\n", __func__);
        mutex_unlock(&g_mutex);
        return 0;
    }
    plat_start_task(node);
    node->state = SF_TASK_RUNNING;
    mutex_unlock(&g_mutex);
    return 0;
}
EXPORT_SYMBOL(start_task);
static int stop_task_l(int task_id)
{
    struct task_info *node;
    node = get_task_by_id(task_id);
    if (!node) {
        pr_err("%s : Task empty. Please run load_task first\n",
                        __func__);
        return -EINVAL;
    }
    if (node->state == SF_TASK_STOPPED) {
        pr_warn("%s : Task is already stopped.\n", __func__);
        return 0;
    }
    plat_stop_task(node);
    node->state = SF_TASK_STOPPED;
    return 0;
}
int stop_task(int task_id)
{
    int ret;
    printk("%s, %d\n", __func__, task_id);
    mutex_lock(&g_mutex);
    ret = stop_task_l(task_id);
    mutex_unlock(&g_mutex);
    return ret;
}
EXPORT_SYMBOL(stop_task);
void remove_task(int task_id)
{
    struct task_info *node;
    printk("%s, %d\n", __func__, task_id);
    mutex_lock(&g_mutex);
    node = get_task_by_id(task_id);
    if (!node) {
        pr_warn("Invalid task id.\n");
        mutex_unlock(&g_mutex);
        return;
    }
    if (node->state == SF_TASK_RUNNING)
        stop_task_l(task_id);
    plat_remove_task(node);
    list_del(&node->list);
    kfree(node);
    mutex_unlock(&g_mutex);
    return;
}
EXPORT_SYMBOL(remove_task);
static int startcore_init(void)
{
    {
        unsigned long addr = SFAX8_DDR_BASE + FIRMWARE_LOAD_OFFSET;
        memset((void *)(CKSEG1ADDR(addr)), 0xFF, FIRMWARE_SZ);
        printk("startcore init fill all memory!\n");
    }
    mutex_init(&g_mutex);
    writel(readl((void *)SYS_MISC_CTRL) | (1 << 3), (void *)SYS_MISC_CTRL);
    return 0;
}
static void startcore_exit(void)
{
    int i;
    for (i = 0; i < MAX_TASKS; i++) {
        remove_task(i);
        printk("Task %d removed.\n", i + 1);
    }
    mutex_destroy(&g_mutex);
    return;
}
module_init(startcore_init);
module_exit(startcore_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Start up cpu(s) for special task(s).");
