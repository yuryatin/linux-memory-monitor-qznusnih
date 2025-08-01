/*
 * qznusnih - A Linux kernel module for monitoring memory accesses
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
 #include <linux/kobject.h>
 #include <linux/perf_event.h>
 #include <linux/hw_breakpoint.h>
 #include <linux/types.h>
 #include <linux/moduleparam.h>
 #include <linux/kthread.h>
 #include <linux/delay.h>
 #include <linux/timekeeping.h>
 #include <linux/crypto.h>
 #include <crypto/hash.h>
 #include <linux/stacktrace.h>
 #include <linux/printk.h>
 
 #define SHA1_DIGEST_SIZE 20
 
 static struct perf_event * wpWrite = NULL;
 static struct perf_event * wpRead = NULL;
 
 struct perf_event_attr attr_w = {0};
 struct perf_event_attr attr_r = {0};
 
 static uint8_t theData = 0xDA; // the byte to watch
 
 //static uint8_t newP = 0x00; // for reading into it from theData in order to trigger READ access for testing
 
 //static struct task_struct * thread_u;
 //static struct task_struct * thread_w;
 
 enum _boolean { False, True };
 typedef enum _boolean boolean;
 
 static unsigned long userAddress;
 static pid_t userPID;
 static struct kobject * kobjParams;
 struct task_struct * taskToWatch = NULL;
 static struct kobj_attribute settingAttribute;
 
 /* I used this function for debugging purposes
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
 static int update_byte(void *data) {
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
     //kfree(desc);
     //crypto_free_shash(tfm);
 
     return 0;
 } */
 
 static void readCallback(struct perf_event * bp, struct perf_sample_data * data,
         struct pt_regs * regs) {
     pr_info("READ access of process %d to theData at %px\n\n", (int) userPID, (void *) userAddress);
     dump_stack();
     pr_info("\n");
     //read_dregs(True);
 }
 
 static void writeCallback(struct perf_event * bp, struct perf_sample_data * data,
         struct pt_regs * regs) {
     pr_info("WRITE access of process %d to theData at %px\n\n", (int) userPID, (void *) userAddress);
     dump_stack();
     pr_info("\n");
     //read_dregs(True);
 }
 
 /*
 static void emptyCallback(struct perf_event * bp, struct perf_sample_data * data,
     struct pt_regs * regs) {
     
 } */
 
 static ssize_t settingsStore(struct kobject * kobj, 
     struct kobj_attribute * attr, 
     const char * buf, size_t count) {
 
     unsigned long addr;
     pid_t pid;
 
     if (sscanf(buf, "%lx %d", &addr, &pid) != 2)
         return -EINVAL;
 
     userAddress = addr;
     userPID = pid;
 
     if (wpWrite) {
         unregister_hw_breakpoint(wpWrite);
         wpWrite = NULL;
     }
     if (wpRead) {
         unregister_hw_breakpoint(wpRead);
         wpRead = NULL;
     }
 
     attr_w.bp_addr = attr_r.bp_addr = userAddress;
     taskToWatch = pid_task(find_vpid(userPID), PIDTYPE_PID);
 
     if (!taskToWatch) {
         pr_err("Task with PID %d not found by qznusnih\n", (int) userPID);
         return -ESRCH;
     }
 
     wpWrite = register_user_hw_breakpoint(&attr_w, writeCallback, NULL, taskToWatch);
     if (IS_ERR(wpWrite)) {
         printk(KERN_ERR "register_user_hw_breakpoint failed for WRITE: %ld\n", PTR_ERR(wpWrite));
         //kthread_stop(thread_u);
         //kthread_stop(thread_w);
         sysfs_remove_file(kobjParams, &settingAttribute.attr);
         kobject_put(kobjParams);
         return PTR_ERR(wpWrite);
     }
 
     wpRead = register_user_hw_breakpoint(&attr_r, readCallback, NULL, taskToWatch);
     if (IS_ERR(wpRead)) {
         printk(KERN_ERR "register_user_hw_breakpoint failed for READ: %ld\n", PTR_ERR(wpRead));
         unregister_hw_breakpoint(wpWrite);
         //kthread_stop(thread_u);
         //kthread_stop(thread_w);
         sysfs_remove_file(kobjParams, &settingAttribute.attr);
         kobject_put(kobjParams);
         return PTR_ERR(wpRead);
     }
 
     pr_info("qznusnih received address to watch = 0x%lx for pid = %d\n", userAddress, userPID);
     return count;
 }
 
 static int __init qznusnih_init(void) {
     settingAttribute.attr.name = "set_address_to_debug";
     settingAttribute.attr.mode = 0220;
     settingAttribute.show = NULL;
     settingAttribute.store = settingsStore;
     kobjParams = kobject_create_and_add("qznusnih", kernel_kobj);
     if (!kobjParams) {
         pr_err("Failed to create kobject for qznusnih\n");
         return -ENOMEM;
     }
     int ret = sysfs_create_file(kobjParams, &settingAttribute.attr);
     if (ret) {
         pr_err("Failed to create sysfs file for qznusnih\n");
         kobject_put(kobjParams);
         return -ENOMEM;
     }
     //pr_info("Setting hardware breakpoints on theData: %px\n", &theData);
 
     /* I create an update thread for testing purposes
     thread_u = kthread_run(update_byte, NULL, "byte_update_thread");
     if (IS_ERR(thread_u)) {
         pr_err("Moduled failed to create thread_u\n");
         return PTR_ERR(thread_u);
     
     thread_w = kthread_run(watch_counters, NULL, "byte_update_thread");
     if (IS_ERR(thread_w)) {
         pr_err("tester failed to create thread_w\n");
         kthread_stop(thread_u);
         return PTR_ERR(thread_w);
     } */
     hw_breakpoint_init(&attr_w);
     hw_breakpoint_init(&attr_r);
     attr_w.bp_addr = attr_r.bp_addr = (uint32_t) &theData;
     attr_w.bp_len = attr_r.bp_len = HW_BREAKPOINT_LEN_1;
     attr_w.bp_type = HW_BREAKPOINT_W;
     // I experimentally confirmed on i686 under QEMU that when two debug registers are free, the order in which breakpoints are assigned to these free registers is preserved
     attr_r.bp_type = HW_BREAKPOINT_RW;
     // Although HW_BREAKPOINT_RW is intrinsically non-specific for reads, I experimentally confirmed on i686 under QEMU that when two debug registers are set to the same address, only one callback is triggered. This, together with the empirical fact that the assignment order is respected, allows precise separation of HW_BREAKPOINT_W and HW_BREAKPOINT_R when the debug registers are set in this order (write first, then read). Specifically, if HW_BREAKPOINT_W triggers first, HW_BREAKPOINT_RW does not.
     pr_info("\nqznusnih user-space memory monitor loaded with sysfs\ninterface at /sys/kernel/qznusnih/set_address_to_debug,\nwritable by root. Root can set the user-space address to\nmonitor by writing a string containing a hexadecimal address\nand a decimal PID, separated by a space: \"virtualAddress PID\"\n\n");
     return 0;
 }
 
 static void __exit qznusnih_exit(void) {
     //kthread_stop(thread_u);
     //kthread_stop(thread_w);
     if (wpWrite) {
         unregister_hw_breakpoint(wpWrite);
         wpWrite = NULL;
     }
     if (wpRead) {
         unregister_hw_breakpoint(wpRead);
         wpRead = NULL;
     }
     sysfs_remove_file(kobjParams, &settingAttribute.attr);
     kobject_put(kobjParams);
     pr_info("qznusnih user-space memory monitor unloaded\n");
 }
 
 module_init(qznusnih_init);
 module_exit(qznusnih_exit);
 
 MODULE_LICENSE("GPL");
 MODULE_AUTHOR("Alexander Yuryatin");
 MODULE_DESCRIPTION("This Linux kernel module monitors a configurable byte of memory and logs the backtrace that led to the access, with separate handling for read and write operations.");
 MODULE_VERSION("0.0.4");
 MODULE_ALIAS("qznusnih");
 