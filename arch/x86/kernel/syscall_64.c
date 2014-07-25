/* System call table for x86-64. */

#include <linux/linkage.h>
#include <linux/sys.h>
#include <linux/cache.h>
#include <linux/moduleparam.h>
#undef MODULE_PARAM_PREFIX
#define MODULE_PARAM_PREFIX "syscall."
#include <linux/bug.h>
#include <linux/init.h>
#include <asm/asm-offsets.h>
#include <asm/syscall.h>
#include <asm/alternative.h>

#define __SYSCALL_COMMON(nr, sym, compat) __SYSCALL_64(nr, sym, compat)

#ifdef CONFIG_X86_X32_ABI
# define __SYSCALL_X32(nr, sym, compat) __SYSCALL_64(nr, sym, compat)
#else
# define __SYSCALL_X32(nr, sym, compat) /* nothing */
#endif

#define __SYSCALL_64(nr, sym, compat) extern asmlinkage void sym(void) ;
#include <asm/syscalls_64.h>
#undef __SYSCALL_64

#define __SYSCALL_64(nr, sym, compat) [nr] = sym,

extern void sys_ni_syscall(void);

asmlinkage const sys_call_ptr_t sys_call_table[__NR_syscall_max+1] = {
	/*
	 * Smells like a compiler bug -- it doesn't work
	 * when the & below is removed.
	 */
	[0 ... __NR_syscall_max] = &sys_ni_syscall,
#include <asm/syscalls_64.h>
};

#ifdef CONFIG_X86_X32_ABI

/* Maybe enable x32 syscalls */

bool x32_enabled = !IS_ENABLED(CONFIG_X86_X32_DISABLED);
module_param_named(x32, x32_enabled, bool, 0444);

extern char system_call_fast_compare_end[], system_call_fast_compare[],
	system_call_trace_compare_end[], system_call_trace_compare[],
	system_call_mask_compare_end[], system_call_mask_compare[];

static int __init x32_enable(void)
{
	BUG_ON(system_call_fast_compare_end - system_call_fast_compare != 10);
	BUG_ON(system_call_trace_compare_end - system_call_trace_compare != 10);
	BUG_ON(system_call_mask_compare_end - system_call_mask_compare != 10);

	if (x32_enabled) {
		text_poke_early(system_call_fast_compare,
				system_call_mask_compare, 10);
		text_poke_early(system_call_trace_compare,
				system_call_mask_compare, 10);
#ifdef CONFIG_X86_X32_DISABLED
		pr_info("Enabled x32 syscalls\n");
#endif
	}
#ifndef CONFIG_X86_X32_DISABLED
	else
		pr_info("Disabled x32 syscalls\n");
#endif

	return 0;
}
late_initcall(x32_enable);

#endif
