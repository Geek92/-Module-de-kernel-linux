#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

#ifdef CONFIG_UNWINDER_ORC
#include <asm/orc_header.h>
ORC_HEADER;
#endif

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x122c3a7e, "_printk" },
	{ 0x2cf56265, "__dynamic_pr_debug" },
	{ 0xe2964344, "__wake_up" },
	{ 0x8d81c75e, "module_put" },
	{ 0x144b1fd4, "remove_proc_entry" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0xc3aaf0a9, "__put_user_1" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x36d0c4e2, "try_module_get" },
	{ 0xdf38b26c, "pcpu_hot" },
	{ 0xe2c17b5d, "__SCT__might_resched" },
	{ 0xfe487975, "init_wait_entry" },
	{ 0x1000e51, "schedule" },
	{ 0x8c26d495, "prepare_to_wait_event" },
	{ 0x92540fbf, "finish_wait" },
	{ 0x5dcda0cb, "noop_llseek" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x167e7f9d, "__get_user_1" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0x87a21cb3, "__ubsan_handle_out_of_bounds" },
	{  0xbffa7, "proc_create" },
	{ 0x632b0a82, "proc_set_size" },
	{ 0x5104d1fd, "proc_set_user" },
	{ 0xebbf458e, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "54ECDB7965AE96459D8F160");
