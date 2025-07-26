/*
 * lmm_qznusnih - A Linux kernel module for monitoring memory accesses
 * Copyright (C) 2025 Alexander Yuryatin
 * 
 * https://github.com/yuryatin/linux-memory-monitor-qznusnih.git
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <linux/types.h>
#include <linux/moduleparam.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/timekeeping.h>
#include <linux/crypto.h>
#include <crypto/hash.h>

#define SHA1_DIGEST_SIZE 20

static struct perf_event ** wpWrite = NULL;
static struct perf_event ** wpRead = NULL;
static uint8_t theData = 0xDA; // the byte to watch

static uint8_t newP = 0x00; // for reading into it from theData in order to trigger READ access for testing

static struct task_struct * thread_u;
static struct task_struct * thread_w;

enum _boolean { False, True };
typedef enum _boolean boolean;

// I used this function for debugging purposes
static void read_dregs(boolean trap) {
    unsigned long dr0, dr1, dr2, dr3, dr6, dr7;

    asm volatile(
        "mov %%dr0, %0\n\t"
        "mov %%dr1, %1\n\t"
        "mov %%dr2, %2\n\t"
        "mov %%dr3, %3\n\t"
        "mov %%dr6, %4\n\t"
        "mov %%dr7, %5\n\t"
        : "=r"(dr0), "=r"(dr1), "=r"(dr2), "=r"(dr3), "=r"(dr6), "=r"(dr7)
    );
    if(trap) {
	pr_info("Debug register values at TRAP:\n");
	pr_info(" \tDR0 = 0x%lx\t", dr0);
    	pr_info(" \tDR1 = 0x%lx\t", dr1);
    	pr_info(" \tDR2 = 0x%lx\n", dr2);
    	pr_info(" \tDR3 = 0x%lx\t", dr3);
    	pr_info(" \tDR6 = 0x%lx\t", dr6);
   	pr_info(" \tDR7 = 0x%lx\n", dr7);
    }
    else {
	pr_info("Debug register values:\n");
   	pr_info(" DR0 = 0x%lx\t", dr0);
   	pr_info(" DR1 = 0x%lx\t", dr1);
    	pr_info(" DR2 = 0x%lx\n", dr2);
   	pr_info(" DR3 = 0x%lx\t", dr3);
    	pr_info(" DR6 = 0x%lx\t", dr6);
    	pr_info(" DR7 = 0x%lx\n", dr7);
    }
}

// The function I used for internal testing
static int update_byte(void *data)
{
    char hash[SHA1_DIGEST_SIZE];
    struct crypto_shash * tfm;
    struct shash_desc * desc;
    int ret;

    newP = 0x98;

    ssleep(1);

    tfm = crypto_alloc_shash("sha1", 0, 0);
    if (IS_ERR(tfm)) {
        pr_err("Failed to allocate tfm\n");
        return PTR_ERR(tfm);
    }

    desc = kmalloc(sizeof(*desc) + crypto_shash_descsize(tfm), GFP_KERNEL);
    if (!desc) {
        crypto_free_shash(tfm);
        return -ENOMEM;
    }

    desc->tfm = tfm;

    uint8_t final = 0;
    uint8_t toread = 0;
    int counter = 0;

    while (!kthread_should_stop()) {
        time64_t now = ktime_get_real_seconds();

        // I use this simple hash so that the byte appears quasi-random with a uniform distribution of its consecutive values, but its non-random modification can still be verified in the log output
        ret = crypto_shash_digest(desc, (const u8 *) &now, sizeof(now), hash);

        final = 0;
        for (int i = 0; i < SHA1_DIGEST_SIZE; i++)
            final ^= hash[i];

        if(!(counter % 2)) {
            //pr_info("1 updated theData to: 0x%02x at virtual kernel address at 0x%p\n", final, &theData);
	        theData = final;
            //pr_info("2 updated theData to: 0x%02x at virtual kernel address at 0x%p\n", final, &theData);
        } else {
            //pr_info("1 read operation with getting 0x%02x at virtual kernel address at 0x%p\n", toread, &theData);
            toread = theData;
            //pr_info("2 read operation with getting 0x%02x at virtual kernel address at 0x%p\n", toread, &theData);
        }

	if (!(counter % 10)) read_dregs(False);
	ssleep(1);
	++counter;
    }
    kfree(desc);
    crypto_free_shash(tfm);

    return 0;
}

static void readCallback(struct perf_event * bp,
                                  struct perf_sample_data * data,
                                  struct pt_regs * regs) {
    pr_info("READ/WRITE access to theData at %px\n", &theData);
    read_dregs(True);
}

static void writeCallback(struct perf_event * bp,
                                   struct perf_sample_data * data,
                                   struct pt_regs * regs) {
    pr_info("WRITE access to theData at %px\n", &theData);
    read_dregs(True);
}

static void emptyCallback(struct perf_event * bp,
    struct perf_sample_data * data,
    struct pt_regs * regs) {
    
}

static int __init lmm_qznusnih_init(void) {
    struct perf_event_attr attr_w = {0};
    struct perf_event_attr attr_r = {0};
    pr_info("Setting hardware breakpoints on theData: %px\n", &theData);

    // I create an update thread for testing purposes
    thread_u = kthread_run(update_byte, NULL, "byte_update_thread");
    if (IS_ERR(thread_u)) {
        pr_err("Moduled failed to create thread_u\n");
        return PTR_ERR(thread_u);
    }
    //thread_w = kthread_run(watch_counters, NULL, "byte_update_thread");
    //if (IS_ERR(thread_w)) {
    //    pr_err("tester failed to create thread_w\n");
    //	  kthread_stop(thread_u);
    //    return PTR_ERR(thread_w);
    //}
    hw_breakpoint_init(&attr_w);
    hw_breakpoint_init(&attr_r);
    attr_w.bp_addr = attr_r.bp_addr = (uint32_t) &theData;
    attr_w.bp_len = attr_r.bp_len = HW_BREAKPOINT_LEN_1;
    attr_w.bp_type = HW_BREAKPOINT_W;
    // I experimentally confirmed on i686 under QEMU that when two debug registers are free, the order in which breakpoints are assigned to these free registers is preserved
    attr_r.bp_type = HW_BREAKPOINT_RW;
    // Although HW_BREAKPOINT_RW is intrinsically non-specific for reads, I experimentally confirmed on i686 under QEMU that when two debug registers are set to the same address, only one callback is triggered. This, together with the empirical fact that the assignment order is respected, allows precise separation of HW_BREAKPOINT_W and HW_BREAKPOINT_R when the debug registers are set in this order (write first, then read). Specifically, if HW_BREAKPOINT_W triggers first, HW_BREAKPOINT_RW does not.

    wpWrite = register_wide_hw_breakpoint(&attr_w, writeCallback, NULL);
    if (IS_ERR(wpWrite)) {
        printk(KERN_ERR "register_wide_hw_breakpoint failed for WRITE: %ld\n", PTR_ERR(wpWrite));
	    kthread_stop(thread_u);
	    kthread_stop(thread_w);	
        return PTR_ERR(wpWrite);
    }

    wpRead = register_wide_hw_breakpoint(&attr_r, readCallback, NULL);
    if (IS_ERR(wpRead)) {
        printk(KERN_ERR "register_wide_hw_breakpoint failed for READ: %ld\n", PTR_ERR(wpRead));
        unregister_wide_hw_breakpoint(wpWrite);
	    kthread_stop(thread_u);
	    kthread_stop(thread_w);	
        return PTR_ERR(wpRead);
    }

    pr_info("Watchpoints registered successfully\n");
    return 0;
}

static void __exit lmm_qznusnih_exit(void) {
    kthread_stop(thread_u);
    // kthread_stop(thread_w);
    unregister_wide_hw_breakpoint(wpWrite);
    unregister_wide_hw_breakpoint(wpRead);
    pr_info("Watchpoints unregistered\n");
}

module_init(lmm_qznusnih_init);
module_exit(lmm_qznusnih_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alexander Yuryatin");
MODULE_DESCRIPTION("This Linux kernel module monitors a configurable byte of memory and logs the backtrace that led to the access, with separate handling for read and write operations.");
MODULE_VERSION("0.0.1");
MODULE_ALIAS("lmm_qznusnih");