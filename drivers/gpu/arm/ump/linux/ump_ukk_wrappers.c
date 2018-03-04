/*
 * Copyright (C) 2010-2014, 2016 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/**
 * @file ump_ukk_wrappers.c
 * Defines the wrapper functions which turn Linux IOCTL calls into _ukk_ calls
 */

#include <asm/uaccess.h>             /* user space access */

#include "ump_osk.h"
#include "ump_uk_types.h"
#include "ump_ukk.h"
#include "ump_kernel_common.h"

#ifdef CONFIG_COMPAT
int ump_copy_from_user( void * destination, const void * source, size_t num, int pointer_size )
{
        int ofs = sizeof(void *)-pointer_size;

        DBG_MSG(2, ("ump_copy_from_user: %d bit, ofs %i num %li, dest+ofs 0x%lx, dest 0x%lx source 0x%lx\n", pointer_size*8, ofs, num, (long unsigned int)(destination+ofs), (long unsigned int)destination, (long unsigned int)source));
        memset (destination, 0, ofs);
        return copy_from_user(destination+ofs, source, num-ofs);
}

int ump_copy_to_user( void * destination, const void * source, size_t num, int pointer_size)
{
        int ofs = sizeof(void *)-pointer_size;

        DBG_MSG(2, ("ump_copy_to_user: %d bit, ofs %i num %li, dest 0x%lx source 0x%lx, source+ofs 0x%lx\n", pointer_size*8, ofs, num, (long unsigned int)destination, (long unsigned int)source, (long unsigned int)(source+ofs)));
        return copy_to_user(destination, source+ofs, num-ofs);
}

#else
int ump_copy_from_user( void * destination, const void * source, size_t num, int pointer_size )
{
	return copy_from_user(destination, source, num);
}

int ump_copy_to_user( void * destination, const void * source, size_t num, int pointer_size)
{
	return copy_to_user(destination, source, num);
}
#endif

/*
 * IOCTL operation; Negotiate version of IOCTL API
 */
int __ump_get_api_version_wrapper(u32 __user *argument, struct ump_session_data *session_data, int pointer_size)
{
	_ump_uk_api_version_s version_info;
	_mali_osk_errcode_t err;

	/* Sanity check input parameters */
	if (NULL == argument || NULL == session_data) {
		MSG_ERR(("NULL parameter in ump_ioctl_get_api_version()\n"));
		return -ENOTTY;
	}

	/* Copy the user space memory to kernel space (so we safely can read it) */
	if (0 != ump_copy_from_user(&version_info, argument, sizeof(version_info), pointer_size)) {
		MSG_ERR(("copy_from_user() in ump_ioctl_get_api_version()\n"));
		return -EFAULT;
	}

	version_info.ctx = (void *) session_data;
	err = _ump_uku_get_api_version(&version_info);
	if (_MALI_OSK_ERR_OK != err) {
		MSG_ERR(("_ump_uku_get_api_version() failed in ump_ioctl_get_api_version()\n"));
		return ump_map_errcode(err);
	}

	version_info.ctx = NULL;

	/* Copy ouput data back to user space */
	if (0 != ump_copy_to_user(argument, &version_info, sizeof(version_info), pointer_size)) {
		MSG_ERR(("copy_to_user() failed in ump_ioctl_get_api_version()\n"));
		return -EFAULT;
	}

	return 0; /* success */
}

int ump_get_api_version_wrapper(u32 __user *argument, struct ump_session_data *session_data)
{
	return __ump_get_api_version_wrapper(argument, session_data, sizeof(void *));
}


/*
 * IOCTL operation; Release reference to specified UMP memory.
 */
int __ump_release_wrapper(u32 __user *argument, struct ump_session_data   *session_data, int pointer_size)
{
	_ump_uk_release_s release_args;
	_mali_osk_errcode_t err;

	/* Sanity check input parameters */
	if (NULL == session_data) {
		MSG_ERR(("NULL parameter in ump_ioctl_release()\n"));
		return -ENOTTY;
	}

	/* Copy the user space memory to kernel space (so we safely can read it) */
	if (0 != ump_copy_from_user(&release_args, argument, sizeof(release_args), pointer_size)) {
		MSG_ERR(("copy_from_user() in ump_ioctl_get_api_version()\n"));
		return -EFAULT;
	}

	release_args.ctx = (void *) session_data;
	err = _ump_ukk_release(&release_args);
	if (_MALI_OSK_ERR_OK != err) {
		MSG_ERR(("_ump_ukk_release() failed in ump_ioctl_release()\n"));
		return ump_map_errcode(err);
	}


	return 0; /* success */
}

int ump_release_wrapper(u32 __user *argument, struct ump_session_data   *session_data)
{
	return __ump_release_wrapper(argument, session_data, sizeof(void *));
}

/*
 * IOCTL operation; Return size for specified UMP memory.
 */
int __ump_size_get_wrapper(u32 __user *argument, struct ump_session_data   *session_data, int pointer_size)
{
	_ump_uk_size_get_s user_interaction;
	_mali_osk_errcode_t err;

	/* Sanity check input parameters */
	if (NULL == argument || NULL == session_data) {
		MSG_ERR(("NULL parameter in ump_ioctl_size_get()\n"));
		return -ENOTTY;
	}

	if (0 != ump_copy_from_user(&user_interaction, argument, sizeof(user_interaction), pointer_size)) {
		MSG_ERR(("copy_from_user() in ump_ioctl_size_get()\n"));
		return -EFAULT;
	}

	user_interaction.ctx = (void *) session_data;
	err = _ump_ukk_size_get(&user_interaction);
	if (_MALI_OSK_ERR_OK != err) {
		MSG_ERR(("_ump_ukk_size_get() failed in ump_ioctl_size_get()\n"));
		return ump_map_errcode(err);
	}

	user_interaction.ctx = NULL;

	if (0 != ump_copy_to_user(argument, &user_interaction, sizeof(user_interaction), pointer_size)) {
		MSG_ERR(("copy_to_user() failed in ump_ioctl_size_get()\n"));
		return -EFAULT;
	}

	return 0; /* success */
}

int ump_size_get_wrapper(u32 __user *argument, struct ump_session_data   *session_data)
{
	return __ump_size_get_wrapper(argument, session_data, sizeof(void *));
}

/*
 * IOCTL operation; Do cache maintenance on specified UMP memory.
 */
int __ump_msync_wrapper(u32 __user *argument, struct ump_session_data   *session_data, int pointer_size)
{
	_ump_uk_msync_s user_interaction;

	/* Sanity check input parameters */
	if (NULL == argument || NULL == session_data) {
		MSG_ERR(("NULL parameter in ump_ioctl_size_get()\n"));
		return -ENOTTY;
	}

	if (0 != ump_copy_from_user(&user_interaction, argument, sizeof(user_interaction), pointer_size)) {
		MSG_ERR(("copy_from_user() in ump_ioctl_msync()\n"));
		return -EFAULT;
	}

	user_interaction.ctx = (void *) session_data;

	_ump_ukk_msync(&user_interaction);

	user_interaction.ctx = NULL;

	if (0 != ump_copy_to_user(argument, &user_interaction, sizeof(user_interaction), pointer_size)) {
		MSG_ERR(("copy_to_user() failed in ump_ioctl_msync()\n"));
		return -EFAULT;
	}

	return 0; /* success */
}

int ump_msync_wrapper(u32 __user *argument, struct ump_session_data   *session_data)
{
	return __ump_msync_wrapper(argument, session_data, sizeof(void *));
}

int __ump_cache_operations_control_wrapper(u32 __user *argument, struct ump_session_data   *session_data, int pointer_size)
{
	_ump_uk_cache_operations_control_s user_interaction;

	/* Sanity check input parameters */
	if (NULL == argument || NULL == session_data) {
		MSG_ERR(("NULL parameter in ump_ioctl_size_get()\n"));
		return -ENOTTY;
	}

	if (0 != ump_copy_from_user(&user_interaction, argument, sizeof(user_interaction), pointer_size)) {
		MSG_ERR(("copy_from_user() in ump_ioctl_cache_operations_control()\n"));
		return -EFAULT;
	}

	user_interaction.ctx = (void *) session_data;

	_ump_ukk_cache_operations_control((_ump_uk_cache_operations_control_s *) &user_interaction);

	user_interaction.ctx = NULL;

#if 0  /* No data to copy back */
	if (0 != ump_copy_to_user(argument, &user_interaction, sizeof(user_interaction), pointer_size)) {
		MSG_ERR(("copy_to_user() failed in ump_ioctl_cache_operations_control()\n"));
		return -EFAULT;
	}
#endif
	return 0; /* success */
}

int ump_cache_operations_control_wrapper(u32 __user *argument, struct ump_session_data   *session_data)
{
	return __ump_cache_operations_control_wrapper(argument, session_data, sizeof(void *));
}

int __ump_switch_hw_usage_wrapper(u32 __user *argument, struct ump_session_data   *session_data, int pointer_size)
{
	_ump_uk_switch_hw_usage_s user_interaction;

	/* Sanity check input parameters */
	if (NULL == argument || NULL == session_data) {
		MSG_ERR(("NULL parameter in ump_ioctl_size_get()\n"));
		return -ENOTTY;
	}

	if (0 != ump_copy_from_user(&user_interaction, argument, sizeof(user_interaction), pointer_size)) {
		MSG_ERR(("copy_from_user() in ump_ioctl_switch_hw_usage()\n"));
		return -EFAULT;
	}

	user_interaction.ctx = (void *) session_data;

	_ump_ukk_switch_hw_usage(&user_interaction);

	user_interaction.ctx = NULL;

#if 0  /* No data to copy back */
	if (0 != ump_copy_to_user(argument, &user_interaction, sizeof(user_interaction), pointer_size)) {
		MSG_ERR(("copy_to_user() failed in ump_ioctl_switch_hw_usage()\n"));
		return -EFAULT;
	}
#endif
	return 0; /* success */
}

int ump_switch_hw_usage_wrapper(u32 __user *argument, struct ump_session_data   *session_data)
{
	return __ump_switch_hw_usage_wrapper(argument, session_data, sizeof(void *));
}

int __ump_lock_wrapper(u32 __user *argument, struct ump_session_data   *session_data, int pointer_size)
{
	_ump_uk_lock_s user_interaction;

	/* Sanity check input parameters */
	if (NULL == argument || NULL == session_data) {
		MSG_ERR(("NULL parameter in ump_ioctl_size_get()\n"));
		return -ENOTTY;
	}

	if (0 != ump_copy_from_user(&user_interaction, argument, sizeof(user_interaction), pointer_size)) {
		MSG_ERR(("copy_from_user() in ump_ioctl_switch_hw_usage()\n"));
		return -EFAULT;
	}

	user_interaction.ctx = (void *) session_data;

	_ump_ukk_lock(&user_interaction);

	user_interaction.ctx = NULL;

#if 0  /* No data to copy back */
	if (0 != ump_copy_to_user(argument, &user_interaction, sizeof(user_interaction), pointer_size)) {
		MSG_ERR(("copy_to_user() failed in ump_ioctl_switch_hw_usage()\n"));
		return -EFAULT;
	}
#endif

	return 0; /* success */
}

int ump_lock_wrapper(u32 __user *argument, struct ump_session_data   *session_data)
{
	return __ump_lock_wrapper(argument, session_data, sizeof(void *));
}

int __ump_unlock_wrapper(u32 __user *argument, struct ump_session_data   *session_data, int pointer_size)
{
	_ump_uk_unlock_s user_interaction;

	/* Sanity check input parameters */
	if (NULL == argument || NULL == session_data) {
		MSG_ERR(("NULL parameter in ump_ioctl_size_get()\n"));
		return -ENOTTY;
	}

	if (0 != ump_copy_from_user(&user_interaction, argument, sizeof(user_interaction), pointer_size)) {
		MSG_ERR(("copy_from_user() in ump_ioctl_switch_hw_usage()\n"));
		return -EFAULT;
	}

	user_interaction.ctx = (void *) session_data;

	_ump_ukk_unlock(&user_interaction);

	user_interaction.ctx = NULL;

#if 0  /* No data to copy back */
	if (0 != ump_copy_to_user(argument, &user_interaction, sizeof(user_interaction), pointer_size)) {
		MSG_ERR(("copy_to_user() failed in ump_ioctl_switch_hw_usage()\n"));
		return -EFAULT;
	}
#endif

	return 0; /* success */
}

int ump_unlock_wrapper(u32 __user *argument, struct ump_session_data   *session_data)
{
	return __ump_unlock_wrapper(argument, session_data, sizeof(void *));
}

#ifdef CONFIG_COMPAT
int ump_get_api_version_wrapper_32(u32 __user *argument, struct ump_session_data *session_data)
{
	return __ump_get_api_version_wrapper(argument, session_data, sizeof(_ump_uk_void_p_32));
}

int ump_release_wrapper_32(u32 __user *argument, struct ump_session_data   *session_data)
{
	return __ump_release_wrapper(argument, session_data, sizeof(_ump_uk_void_p_32));
}

int ump_size_get_wrapper_32(u32 __user *argument, struct ump_session_data   *session_data)
{
	return __ump_size_get_wrapper(argument, session_data, sizeof(_ump_uk_void_p_32));
}

#if 0
int ump_msync_wrapper_32(u32 __user *argument, struct ump_session_data   *session_data)
{
	return __ump_msync_wrapper(argument, session_data, sizeof(_ump_uk_void_p_32));
}
#endif

/*
 * IOCTL operation; Do cache maintenance on specified UMP memory.
 */
int ump_msync_wrapper_32(u32 __user *argument, struct ump_session_data   *session_data, int pointer_size)
{
	_ump_uk_msync_s user_interaction;
	_ump_uk_msync_32_s user_interaction_32;

	/* Sanity check input parameters */
	if (NULL == argument || NULL == session_data) {
		MSG_ERR(("NULL parameter in ump_ioctl_size_get()\n"));
		return -ENOTTY;
	}

	if (0 != copy_from_user(&user_interaction_32, argument, sizeof(user_interaction_32))) {
		MSG_ERR(("copy_from_user() in ump_ioctl_msync()\n"));
		return -EFAULT;
	}

	user_interaction.mapping   = (void *)(unsigned long)user_interaction_32.mapping;        /**< [in] mapping addr */
	user_interaction.address   = (void *)(unsigned long)user_interaction_32.address;        /**< [in] flush start addr */
	user_interaction.size      = user_interaction_32.size;           /**< [in] size to flush */
	user_interaction.op        = user_interaction_32.op;             /**< [in] flush operation */
	user_interaction.cookie    = user_interaction_32.cookie;         /**< [in] cookie stored with reference to the kernel mapping internals */
	user_interaction.secure_id = user_interaction_32.secure_id;      /**< [in] secure_id that identifies the ump buffer */
	user_interaction.is_cached = user_interaction_32.is_cached;      /**< [out] caching of CPU mappings */

	user_interaction.ctx = (void *) session_data;

	_ump_ukk_msync(&user_interaction);

	user_interaction.ctx = NULL;

	user_interaction_32.ctx       = (_ump_uk_void_p_32)(unsigned long)user_interaction.ctx;
	user_interaction_32.mapping   = (_ump_uk_void_p_32)(unsigned long)user_interaction.mapping;        /**< [in] mapping addr */
	user_interaction_32.address   = (_ump_uk_void_p_32)(unsigned long)user_interaction.address;        /**< [in] flush start addr */
	user_interaction_32.size      = user_interaction.size;           /**< [in] size to flush */
	user_interaction_32.op        = user_interaction.op;             /**< [in] flush operation */
	user_interaction_32.cookie    = user_interaction.cookie;         /**< [in] cookie stored with reference to the kernel mapping internals */
	user_interaction_32.secure_id = user_interaction.secure_id;      /**< [in] secure_id that identifies the ump buffer */
	user_interaction_32.is_cached = user_interaction.is_cached;      /**< [out] caching of CPU mappings */

	if (0 != copy_to_user(argument, &user_interaction_32, sizeof(user_interaction_32))) {
		MSG_ERR(("copy_to_user() failed in ump_ioctl_msync()\n"));
		return -EFAULT;
	}

	return 0; /* success */
}

int ump_cache_operations_control_wrapper_32(u32 __user *argument, struct ump_session_data   *session_data)
{
	return __ump_cache_operations_control_wrapper(argument, session_data, sizeof(_ump_uk_void_p_32));
}

int ump_switch_hw_usage_wrapper_32(u32 __user *argument, struct ump_session_data   *session_data)
{
	return __ump_switch_hw_usage_wrapper(argument, session_data, sizeof(_ump_uk_void_p_32));
}

int ump_lock_wrapper_32(u32 __user *argument, struct ump_session_data   *session_data)
{
	return __ump_lock_wrapper(argument, session_data, sizeof(_ump_uk_void_p_32));
}

int ump_unlock_wrapper_32(u32 __user *argument, struct ump_session_data   *session_data)
{
	return __ump_unlock_wrapper(argument, session_data, sizeof(_ump_uk_void_p_32));
}

#endif /* CONFIG_COMPAT */

