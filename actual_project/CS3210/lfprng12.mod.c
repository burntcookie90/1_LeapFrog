#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__attribute_used__
__attribute__((section("__versions"))) = {
	{ 0x70591326, "struct_module" },
	{ 0x859204af, "sscanf" },
	{ 0x945bc6a7, "copy_from_user" },
	{ 0x6067a146, "memcpy" },
	{ 0x1d26aa98, "sprintf" },
	{ 0xd6ee688f, "vmalloc" },
	{ 0x6f24cc5f, "create_proc_entry" },
	{ 0x858745ac, "remove_proc_entry" },
	{ 0xdd132261, "printk" },
};

static const char __module_depends[]
__attribute_used__
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "AA197DA8F7C1046382E66CD");
