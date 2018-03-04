/*
 * Copyright (C) 2010-2016 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include <linux/fs.h>       /* file system operations */
#include <asm/uaccess.h>    /* user space access */

#include "mali_ukk.h"
#include "mali_osk.h"
#include "mali_kernel_common.h"
#include "mali_session.h"
#include "mali_ukk_wrappers.h"

int mem_alloc_wrapper(struct mali_session_data *session_data, _mali_uk_alloc_mem_s __user *uargs)
{
	_mali_uk_alloc_mem_s kargs;
	_mali_osk_errcode_t err;

	MALI_CHECK_NON_NULL(uargs, -EINVAL);
	MALI_CHECK_NON_NULL(session_data, -EINVAL);

	if (0 != copy_from_user(&kargs, uargs, sizeof(_mali_uk_alloc_mem_s))) {
		return -EFAULT;
	}
	kargs.ctx = (uintptr_t)session_data;

	err = _mali_ukk_mem_allocate(&kargs);

	if (_MALI_OSK_ERR_OK != err) {
		return map_errcode(err);
	}

	if (0 != put_user(kargs.backend_handle, &uargs->backend_handle)) {
		return -EFAULT;
	}

	return 0;
}

int mem_free_wrapper(struct mali_session_data *session_data, _mali_uk_free_mem_s __user *uargs)
{
	_mali_uk_free_mem_s kargs;
	_mali_osk_errcode_t err;

	MALI_CHECK_NON_NULL(uargs, -EINVAL);
	MALI_CHECK_NON_NULL(session_data, -EINVAL);

	if (0 != copy_from_user(&kargs, uargs, sizeof(_mali_uk_free_mem_s))) {
		return -EFAULT;
	}
	kargs.ctx = (uintptr_t)session_data;

	err = _mali_ukk_mem_free(&kargs);

	if (_MALI_OSK_ERR_OK != err) {
		return map_errcode(err);
	}

	if (0 != put_user(kargs.free_pages_nr, &uargs->free_pages_nr)) {
		return -EFAULT;
	}

	return 0;
}

int mem_bind_wrapper(struct mali_session_data *session_data, _mali_uk_bind_mem_s __user *uargs)
{
	_mali_uk_bind_mem_s kargs;
	_mali_osk_errcode_t err;

	MALI_CHECK_NON_NULL(uargs, -EINVAL);
	MALI_CHECK_NON_NULL(session_data, -EINVAL);

	if (0 != copy_from_user(&kargs, uargs, sizeof(_mali_uk_bind_mem_s))) {
		return -EFAULT;
	}
	kargs.ctx = (uintptr_t)session_data;

	err = _mali_ukk_mem_bind(&kargs);

	if (_MALI_OSK_ERR_OK != err) {
		return map_errcode(err);
	}

	return 0;
}

int mem_unbind_wrapper(struct mali_session_data *session_data, _mali_uk_unbind_mem_s __user *uargs)
{
	_mali_uk_unbind_mem_s kargs;
	_mali_osk_errcode_t err;

	MALI_CHECK_NON_NULL(uargs, -EINVAL);
	MALI_CHECK_NON_NULL(session_data, -EINVAL);

	if (0 != copy_from_user(&kargs, uargs, sizeof(_mali_uk_unbind_mem_s))) {
		return -EFAULT;
	}
	kargs.ctx = (uintptr_t)session_data;

	err = _mali_ukk_mem_unbind(&kargs);

	if (_MALI_OSK_ERR_OK != err) {
		return map_errcode(err);
	}

	return 0;
}


int mem_cow_wrapper(struct mali_session_data *session_data, _mali_uk_cow_mem_s __user *uargs)
{
	_mali_uk_cow_mem_s kargs;
	_mali_osk_errcode_t err;

	MALI_CHECK_NON_NULL(uargs, -EINVAL);
	MALI_CHECK_NON_NULL(session_data, -EINVAL);

	if (0 != copy_from_user(&kargs, uargs, sizeof(_mali_uk_cow_mem_s))) {
		return -EFAULT;
	}
	kargs.ctx = (uintptr_t)session_data;

	err = _mali_ukk_mem_cow(&kargs);

	if (_MALI_OSK_ERR_OK != err) {
		return map_errcode(err);
	}

	if (0 != put_user(kargs.backend_handle, &uargs->backend_handle)) {
		return -EFAULT;
	}

	return 0;
}

int mem_cow_modify_range_wrapper(struct mali_session_data *session_data, _mali_uk_cow_modify_range_s __user *uargs)
{
	_mali_uk_cow_modify_range_s kargs;
	_mali_osk_errcode_t err;

	MALI_CHECK_NON_NULL(uargs, -EINVAL);
	MALI_CHECK_NON_NULL(session_data, -EINVAL);

	if (0 != copy_from_user(&kargs, uargs, sizeof(_mali_uk_cow_modify_range_s))) {
		return -EFAULT;
	}
	kargs.ctx = (uintptr_t)session_data;

	err = _mali_ukk_mem_cow_modify_range(&kargs);

	if (_MALI_OSK_ERR_OK != err) {
		return map_errcode(err);
	}

	if (0 != put_user(kargs.change_pages_nr, &uargs->change_pages_nr)) {
		return -EFAULT;
	}
	return 0;
}


int mem_resize_mem_wrapper(struct mali_session_data *session_data, _mali_uk_mem_resize_s __user *uargs)
{
	_mali_uk_mem_resize_s kargs;
	_mali_osk_errcode_t err;

	MALI_CHECK_NON_NULL(uargs, -EINVAL);
	MALI_CHECK_NON_NULL(session_data, -EINVAL);

	if (0 != copy_from_user(&kargs, uargs, sizeof(_mali_uk_mem_resize_s))) {
		return -EFAULT;
	}
	kargs.ctx = (uintptr_t)session_data;

	err = _mali_ukk_mem_resize(&kargs);

	if (_MALI_OSK_ERR_OK != err) {
		return map_errcode(err);
	}

	return 0;
}

int mem_write_safe_wrapper(struct mali_session_data *session_data, _mali_uk_mem_write_safe_s __user *uargs)
{
	_mali_uk_mem_write_safe_s kargs;
	_mali_osk_errcode_t err;

	MALI_CHECK_NON_NULL(uargs, -EINVAL);
	MALI_CHECK_NON_NULL(session_data, -EINVAL);

	if (0 != copy_from_user(&kargs, uargs, sizeof(_mali_uk_mem_write_safe_s))) {
		return -EFAULT;
	}

	kargs.ctx = (uintptr_t)session_data;

	/* Check if we can access the buffers */
	if (!access_ok(VERIFY_WRITE, kargs.dest, kargs.size)
	    || !access_ok(VERIFY_READ, kargs.src, kargs.size)) {
		return -EINVAL;
	}

	/* Check if size wraps */
	if ((kargs.size + kargs.dest) <= kargs.dest
	    || (kargs.size + kargs.src) <= kargs.src) {
		return -EINVAL;
	}

	err = _mali_ukk_mem_write_safe(&kargs);
	if (_MALI_OSK_ERR_OK != err) {
		return map_errcode(err);
	}

	if (0 != put_user(kargs.size, &uargs->size)) {
		return -EFAULT;
	}

	return 0;
}



int mem_query_mmu_page_table_dump_size_wrapper(struct mali_session_data *session_data, _mali_uk_query_mmu_page_table_dump_size_s __user *uargs)
{
	_mali_uk_query_mmu_page_table_dump_size_s kargs;
	_mali_osk_errcode_t err;

	MALI_CHECK_NON_NULL(uargs, -EINVAL);
	MALI_CHECK_NON_NULL(session_data, -EINVAL);

	kargs.ctx = (uintptr_t)session_data;

	err = _mali_ukk_query_mmu_page_table_dump_size(&kargs);
	if (_MALI_OSK_ERR_OK != err) return map_errcode(err);

	if (0 != put_user(kargs.size, &uargs->size)) return -EFAULT;

	return 0;
}

int mem_dump_mmu_page_table_wrapper(struct mali_session_data *session_data, _mali_uk_dump_mmu_page_table_s __user *uargs)
{
	_mali_uk_dump_mmu_page_table_s kargs;
	_mali_osk_errcode_t err;
	void __user *user_buffer;
	void *buffer = NULL;
	int rc = -EFAULT;

	/* validate input */
	MALI_CHECK_NON_NULL(uargs, -EINVAL);
	/* the session_data pointer was validated by caller */

	if (0 != copy_from_user(&kargs, uargs, sizeof(_mali_uk_dump_mmu_page_table_s)))
		goto err_exit;

	user_buffer = (void __user *)(uintptr_t)kargs.buffer;
	if (!access_ok(VERIFY_WRITE, user_buffer, kargs.size))
		goto err_exit;

	/* allocate temporary buffer (kernel side) to store mmu page table info */
	if (kargs.size <= 0)
		return -EINVAL;
	/* Allow at most 8MiB buffers, this is more than enough to dump a fully
	 * populated page table. */
	if (kargs.size > SZ_8M)
		return -EINVAL;

	buffer = (void *)(uintptr_t)_mali_osk_valloc(kargs.size);
	if (NULL == buffer) {
		rc = -ENOMEM;
		goto err_exit;
	}

	kargs.ctx = (uintptr_t)session_data;
	kargs.buffer = (uintptr_t)buffer;
	err = _mali_ukk_dump_mmu_page_table(&kargs);
	if (_MALI_OSK_ERR_OK != err) {
		rc = map_errcode(err);
		goto err_exit;
	}

	/* copy mmu page table info back to user space and update pointers */
	if (0 != copy_to_user(user_buffer, buffer, kargs.size))
		goto err_exit;

	kargs.register_writes = kargs.register_writes -
				(uintptr_t)buffer + (uintptr_t)user_buffer;
	kargs.page_table_dump = kargs.page_table_dump -
				(uintptr_t)buffer + (uintptr_t)user_buffer;

	if (0 != copy_to_user(uargs, &kargs, sizeof(kargs)))
		goto err_exit;

	rc = 0;

err_exit:
	if (buffer) _mali_osk_vfree(buffer);
	return rc;
}

int mem_usage_get_wrapper(struct mali_session_data *session_data, _mali_uk_profiling_memory_usage_get_s __user *uargs)
{
	_mali_osk_errcode_t err;
	_mali_uk_profiling_memory_usage_get_s kargs;

	MALI_CHECK_NON_NULL(uargs, -EINVAL);
	MALI_CHECK_NON_NULL(session_data, -EINVAL);

	if (0 != copy_from_user(&kargs, uargs, sizeof(_mali_uk_profiling_memory_usage_get_s))) {
		return -EFAULT;
	}

	kargs.ctx = (uintptr_t)session_data;
	err = _mali_ukk_mem_usage_get(&kargs);
	if (_MALI_OSK_ERR_OK != err) {
		return map_errcode(err);
	}

	kargs.ctx = (uintptr_t)NULL; /* prevent kernel address to be returned to user space */
	if (0 != copy_to_user(uargs, &kargs, sizeof(_mali_uk_profiling_memory_usage_get_s))) {
		return -EFAULT;
	}

	return 0;
}

int mem_map_ext_wrapper(struct mali_session_data *session_data, _mali_uk_map_external_mem_s __user *argument)
{
	_mali_uk_bind_mem_s kargs;
	_mali_uk_map_external_mem_s uk_args;
	_mali_osk_errcode_t err_code;

	/* validate input */
	/* the session_data pointer was validated by caller */
	MALI_CHECK_NON_NULL(argument, -EINVAL);

	/* get call arguments from user space. copy_from_user returns how many bytes which where NOT copied */
	if (0 != copy_from_user(&uk_args, (void __user *)argument, sizeof(_mali_uk_map_external_mem_s))) {
		return -EFAULT;
	}

        kargs.ctx = uk_args.ctx;                                        /**< [in,out] user-kernel context (trashed on output) */
        kargs.vaddr = uk_args.mali_address;                                      /**< [in] mali address to map the physical memory to */
        kargs.size = uk_args.size;                                       /**< [in] size */
        kargs.flags = _MALI_MEMORY_BIND_BACKEND_EXTERNAL_MEMORY;                                      /**< [in] see_MALI_MEMORY_BIND_BACKEND_* */
        kargs.padding = 0;                                    /** padding for 32/64 struct alignment */
        kargs.mem_union.bind_ext_memory.phys_addr = uk_args.phys_addr;                  /**< [in] phys_addr */
        kargs.mem_union.bind_ext_memory.rights = uk_args.rights;                     /**< [in] rights necessary for accessing memory */
        kargs.mem_union.bind_ext_memory.flags = uk_args.flags;                      /**< [in] flags, see \ref _MALI_MAP_EXTERNAL_MAP_GUARD_PAGE */

	kargs.ctx = (uintptr_t)session_data;
	err_code = _mali_ukk_mem_bind(&kargs);


	if (0 != put_user(kargs.vaddr, &argument->cookie)) {
		if (_MALI_OSK_ERR_OK == err_code) {
			/* Rollback */
                        _mali_uk_unbind_mem_s kargs_unmap;

                        kargs_unmap.ctx = (uintptr_t)session_data;                                        /**< [in,out] user-kernel context (trashed on output) */
                        kargs_unmap.vaddr = kargs.vaddr;                                      /**< [in] mali address to map the physical memory to */
                        kargs_unmap.flags = _MALI_MEMORY_BIND_BACKEND_EXTERNAL_MEMORY;                                      /**< [in] see_MALI_MEMORY_BIND_BACKEND_* */
                        err_code = _mali_ukk_mem_unbind(&kargs_unmap);
			if (_MALI_OSK_ERR_OK != err_code) {
				MALI_DEBUG_PRINT(4, ("reverting _mali_ukk_unmap_external_mem, as a result of failing put_user(), failed\n"));
			}
		}
		return -EFAULT;
	}

	/* Return the error that _mali_ukk_free_big_block produced */
	return map_errcode(err_code);
}

int mem_unmap_ext_wrapper(struct mali_session_data *session_data, _mali_uk_unmap_external_mem_s __user *argument)
{
	_mali_uk_unbind_mem_s kargs;
	_mali_uk_unmap_external_mem_s uk_args;
	_mali_osk_errcode_t err_code;

	/* validate input */
	/* the session_data pointer was validated by caller */
	MALI_CHECK_NON_NULL(argument, -EINVAL);

	/* get call arguments from user space. copy_from_user returns how many bytes which where NOT copied */
	if (0 != copy_from_user(&uk_args, (void __user *)argument, sizeof(_mali_uk_unmap_external_mem_s))) {
		return -EFAULT;
	}

        kargs.ctx = uk_args.ctx;                                        /**< [in,out] user-kernel context (trashed on output) */
        kargs.vaddr = uk_args.cookie;                                      /**< [in] mali address to map the physical memory to */
        kargs.flags = _MALI_MEMORY_BIND_BACKEND_EXTERNAL_MEMORY;                                      /**< [in] see_MALI_MEMORY_BIND_BACKEND_* */

        kargs.ctx = (uintptr_t)session_data;
        err_code = _mali_ukk_mem_unbind(&kargs);

	/* Return the error that _mali_ukk_free_big_block produced */
	return map_errcode(err_code);
}

#if defined(CONFIG_MALI400_UMP)
int mem_release_ump_wrapper(struct mali_session_data *session_data, _mali_uk_release_ump_mem_s __user *argument)
{
	_mali_uk_unbind_mem_s kargs;
	_mali_uk_release_ump_mem_s uk_args;
	_mali_osk_errcode_t err_code;

	/* validate input */
	/* the session_data pointer was validated by caller */
	MALI_CHECK_NON_NULL(argument, -EINVAL);

	/* get call arguments from user space. copy_from_user returns how many bytes which where NOT copied */
	if (0 != copy_from_user(&uk_args, (void __user *)argument, sizeof(_mali_uk_release_ump_mem_s))) {
		return -EFAULT;
	}

        kargs.ctx = uk_args.ctx;                                        /**< [in,out] user-kernel context (trashed on output) */
        kargs.vaddr = uk_args.cookie;                                      /**< [in] mali address to map the physical memory to */
	kargs.flags = _MALI_MEMORY_BIND_BACKEND_UMP;                                      /**< [in] see_MALI_MEMORY_BIND_BACKEND_* */

        kargs.ctx = (uintptr_t)session_data;
        err_code = _mali_ukk_mem_unbind(&kargs);

	/* Return the error that _mali_ukk_free_big_block produced */
	return map_errcode(err_code);
}

int mem_attach_ump_wrapper(struct mali_session_data *session_data, _mali_uk_attach_ump_mem_s __user *argument)
{
	_mali_uk_bind_mem_s kargs;
	_mali_uk_attach_ump_mem_s uk_args;
	_mali_osk_errcode_t err_code;

	/* validate input */
	/* the session_data pointer was validated by caller */
	MALI_CHECK_NON_NULL(argument, -EINVAL);

	/* get call arguments from user space. copy_from_user returns how many bytes which where NOT copied */
	if (0 != copy_from_user(&uk_args, (void __user *)argument, sizeof(_mali_uk_attach_ump_mem_s))) {
		return -EFAULT;
	}

        kargs.ctx = uk_args.ctx;                                        /**< [in,out] user-kernel context (trashed on output) */
        kargs.vaddr = uk_args.mali_address;                                      /**< [in] mali address to map the physical memory to */
        kargs.size = uk_args.size;                                       /**< [in] size */
        kargs.flags = _MALI_MEMORY_BIND_BACKEND_UMP;                                      /**< [in] see_MALI_MEMORY_BIND_BACKEND_* */
        kargs.padding = 0;                                    /** padding for 32/64 struct alignment */
        kargs.mem_union.bind_ump.secure_id = (int)uk_args.secure_id;                  /**< [in] secure id */
        kargs.mem_union.bind_ump.rights = uk_args.rights;                     /**< [in] rights necessary for accessing memory */
        kargs.mem_union.bind_ump.flags = uk_args.flags;                      /**< [in] flags, see \ref _MALI_MAP_EXTERNAL_MAP_GUARD_PAGE */

        kargs.ctx = (uintptr_t)session_data;

        err_code = _mali_ukk_mem_bind(&kargs);

	if (0 != put_user(kargs.vaddr, &argument->cookie)) {
		if (_MALI_OSK_ERR_OK == err_code) {
			/* Rollback */
			_mali_uk_unbind_mem_s kargs_unmap;

			kargs_unmap.ctx = (uintptr_t)session_data;                                        /**< [in,out] user-kernel context (trashed on output) */
			kargs_unmap.vaddr = kargs.vaddr;                                      /**< [in] mali address to map the physical memory to */
			kargs_unmap.flags = _MALI_MEMORY_BIND_BACKEND_UMP;                                      /**< [in] see_MALI_MEMORY_BIND_BACKEND_* */
			err_code = _mali_ukk_mem_unbind(&kargs_unmap);
			if (_MALI_OSK_ERR_OK != err_code) {
				MALI_DEBUG_PRINT(4, ("reverting _mali_ukk_attach_mem, as a result of failing put_user(), failed\n"));
			}
		}
		return -EFAULT;
	}

	/* Return the error that _mali_ukk_map_external_ump_mem produced */
	return map_errcode(err_code);
}
#endif /* CONFIG_MALI400_UMP */

int profiling_memory_usage_get_wrapper_v600(struct mali_session_data *session_data, _mali_uk_profiling_memory_usage_get_v600_s __user *uargs)
{
	_mali_osk_errcode_t err;
	_mali_uk_profiling_memory_usage_get_v600_s kargs;

	MALI_CHECK_NON_NULL(uargs, -EINVAL);
	MALI_CHECK_NON_NULL(session_data, -EINVAL);

	if (0 != copy_from_user(&kargs, uargs, sizeof(_mali_uk_profiling_memory_usage_get_v600_s))) {
		return -EFAULT;
	}

	kargs.ctx = (uintptr_t)session_data;
	err = _mali_ukk_profiling_memory_usage_get_v600(&kargs);
	if (_MALI_OSK_ERR_OK != err) {
		return map_errcode(err);
	}

	kargs.ctx = (uintptr_t)NULL; /* prevent kernel address to be returned to user space */
	if (0 != copy_to_user(uargs, &kargs, sizeof(_mali_uk_profiling_memory_usage_get_v600_s))) {
		return -EFAULT;
	}

	return 0;
}

