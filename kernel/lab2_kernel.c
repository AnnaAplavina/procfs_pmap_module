#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/sched/signal.h>
#include <linux/rcupdate.h>
#include <linux/mm.h> // Добавлен заголовочный файл для работы с памятью

#define PROCFS_FILE "pmap_info"

static struct proc_dir_entry *proc_file;

// Global variable to store the last received PID
static int last_received_pid = -1;
// Global variable to store the last received message
static char *message = NULL;
static char *message_vms = NULL;

static ssize_t procfile_write(struct file *file, const char __user *buffer,
                               size_t count, loff_t *data) {
    int pid;
    ssize_t ret;

    // Dynamically allocate memory for the message
    kfree(message);
    kfree(message_vms);
    message = kmalloc(count + 10, GFP_KERNEL);  // Add extra space for "hello %d\n"
    message_vms = kmalloc(1024 * 1024, GFP_KERNEL);
    message_vms[0] = '\0';

    if (!message) {
        pr_err("Failed to allocate memory for the message\n");
        return -ENOMEM;
    }

    // Copy the PID from the user buffer
    if (copy_from_user(&pid, buffer, sizeof(pid))) {
        pr_err("Failed to copy PID from user\n");
        return -EFAULT;
    }

    last_received_pid = pid;

    // Format the message
    snprintf(message, count + 10, "hello %d\n", last_received_pid);

    struct task_struct *task = pid_task(find_vpid(last_received_pid), PIDTYPE_PID);
    if (!task) {
        pr_err("Failed to find task with PID: %d\n", last_received_pid);
        ret = -EINVAL;
        goto out;
    }
    struct mm_struct *mm = task->mm;

    if (!mm) {
        pr_err("No user memory space for the task\n");
        ret = -EINVAL;
        goto out;
    }

    struct vm_area_struct *vma;

    vma = mm->mmap;
while (vma) {
    char temp_buffer[20];
    snprintf(temp_buffer, sizeof(temp_buffer), "%lx", vma->vm_start);
    strncat(message_vms, temp_buffer, sizeof(temp_buffer));

    unsigned long virtual_mapping_size = vma->vm_end - vma->vm_start;
    unsigned long virtual_mapping_size_kB = virtual_mapping_size / 1024;
    pr_info(" Virtual Mapping Size: %lu", virtual_mapping_size_kB);
    char size_info[128]; // Adjust the size accordingly
    snprintf(size_info, sizeof(size_info), " %luK", virtual_mapping_size_kB);
    strncat(message_vms, size_info, 1024 * 1024 - strlen(message_vms));

    // Extracting and formatting permission flags
    char permission_flags[7]; // Assuming "rwx" + "-" for readability
    permission_flags[0] = ' ';
    permission_flags[1] = (vma->vm_flags & VM_READ) ? 'r' : '-';
    permission_flags[2] = (vma->vm_flags & VM_WRITE) ? 'w' : '-';
    permission_flags[3] = (vma->vm_flags & VM_EXEC) ? 'x' : '-';
    permission_flags[4] = (vma->vm_flags & VM_SHARED) ? 's' : '-';
    permission_flags[5] = (vma->vm_flags & VM_MAYSHARE) ? 'm' : '-';
    permission_flags[6] = '\0'; // Null-terminate the string
    pr_info(" Permission Flags: %s", permission_flags);
    strncat(message_vms, permission_flags, 1024 * 1024 - strlen(message_vms));

    if (vma->vm_file) {
        if (vma->vm_flags & (VM_GROWSDOWN | VM_GROWSUP)) {
            pr_info(" [ heap ]");
            strncat(message_vms, " [ heap ]\n", 1024 * 1024 - strlen(message_vms));
        } else {
            pr_info(" %s", vma->vm_file->f_path.dentry->d_name.name);
            char filename[256]; // Adjust the size accordingly
            snprintf(filename, sizeof(filename), " %s\n", vma->vm_file->f_path.dentry->d_name.name);
            strncat(message_vms, filename, 1024 * 1024 - strlen(message_vms));
        }
    } else {
        pr_info(" [ anon ]");
        strncat(message_vms, " [ anon ]\n", 1024 * 1024 - strlen(message_vms));
    }
    vma = vma->vm_next;
}


    pr_info("Received PID: %d\n", last_received_pid);

    ret = count;

    out:
    return ret;
}

static ssize_t procfile_read(struct file *file, char __user *buffer,
                              size_t count, loff_t *offset) {
    ssize_t ret;

    pr_info("message_vms content: %s\n", message_vms);
    pr_info("hellooooo");

    if (!message) {
        // No message received yet
        ret = 0;
    } else {
        // Copy the last received message to the user buffer
        //ret = simple_read_from_buffer(buffer, count, offset, message, strlen(message));
        ret = simple_read_from_buffer(buffer, count, offset, message_vms, strlen(message_vms));
    }

    return ret;
}

static const struct proc_ops proc_file_fops = {
    .proc_read = procfile_read,
    .proc_write = procfile_write,
};

static int __init lab2_kernel_init(void) {
    proc_file = proc_create(PROCFS_FILE, 0666, NULL, &proc_file_fops);

    if (!proc_file) {
        pr_alert("Can not create file in /proc\n");
        return -ENOMEM;
    }

    pr_info("Module loaded\n");

    return 0;
}

static void __exit lab2_kernel_exit(void) {
    kfree(message);  // Free dynamically allocated memory
    remove_proc_entry(PROCFS_FILE, NULL);
    pr_info("Module unloaded\n");
}

module_init(lab2_kernel_init);
module_exit(lab2_kernel_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Anna Aplavina");
