// SPDX-License-Identifier: GPL-2.0
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <asm/setup.h>

#ifdef CONFIG_PROC_BEGONIA_CMDLINE
static char patched_cmdline[COMMAND_LINE_SIZE];
#endif

#ifdef KSU_SUSFS_SPOOF_CMDLINE_OR_BOOTCONFIG
extern int susfs_spoof_cmdline_or_bootconfig(struct seq_file *m);
#endif

static int cmdline_proc_show(struct seq_file *m, void *v)
{
#ifdef KSU_SUSFS_SPOOF_CMDLINE_OR_BOOTCONFIG
	if (!susfs_spoof_cmdline_or_bootconfig(m)) {
		seq_putc(m, '\n');
		return 0;
	}
#endif
#ifndef CONFIG_PROC_BEGONIA_CMDLINE
	seq_printf(m, "%s\n", saved_command_line);
#else
	seq_printf(m, "%s\n", patched_cmdline);
#endif
	return 0;
}

static int cmdline_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, cmdline_proc_show, NULL);
}

static const struct file_operations cmdline_proc_fops = {
	.open		= cmdline_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

#ifdef CONFIG_PROC_BEGONIA_CMDLINE
static void append_cmdline(char *cmd, const char *flag_val) {
    if (strlen(cmd) + strlen(flag_val) + 1 < COMMAND_LINE_SIZE) {
        strncat(cmd, " ", COMMAND_LINE_SIZE - strlen(cmd) - 1);
        strncat(cmd, flag_val, COMMAND_LINE_SIZE - strlen(cmd) - 1);
    }
}

static bool check_flag(char *cmd, const char *flag, const char *val) {
    size_t f_len, v_len;
    char *f_pos, *v_pos, *v_end;
    bool ret = false;

    f_pos = strstr(cmd, flag);
    if (!f_pos) {
        return false;
    }

    f_len = strlen(flag);
    v_len = strlen(val);
    v_pos = f_pos + f_len;
    v_end = v_pos + strcspn(v_pos, " ");

    if ((v_end - v_pos) == v_len && !memcmp(v_pos, val, v_len)) {
        ret = true;
    }

    return ret;
}

static void replace_cmdline_param(char *cmdline, const char *key, const char *value) {
    char *param_start = strstr(cmdline, key);

    if (param_start) {
        char *param_end = strchr(param_start, ' ');
        if (param_end) {
            // Remove existing parameter
            memmove(param_start, param_end + 1, strlen(param_end));
        } else {
            // Null-terminate if it's the last parameter
            *param_start = '\0';
        }
    }

    // Append the new parameter
    append_cmdline(cmdline, key);
    append_cmdline(cmdline, value);
}

static void patch_begonia_cmdline(char *cmdline)
{
	if (!check_flag(cmdline, "console=", "ttyS0,921600n1")) {
		replace_cmdline_param(cmdline, "console=", "ttyMT3,921600n1");
	}
	if (!check_flag(cmdline, "vmalloc=", "400")) {
		replace_cmdline_param(cmdline, "vmalloc=", "496M");
	}
        append_cmdline(cmdline, "slub_max_order=0");
        append_cmdline(cmdline, "slub_debug=0");
}
#endif

static int __init proc_cmdline_init(void)
{
#ifdef CONFIG_PROC_BEGONIA_CMDLINE
	strcpy(patched_cmdline, saved_command_line);

	patch_begonia_cmdline(patched_cmdline);
#endif

	proc_create("cmdline", 0, NULL, &cmdline_proc_fops);
	return 0;
}
fs_initcall(proc_cmdline_init);
