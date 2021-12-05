#include <linux/file.h>
#include <linux/anon_inodes.h>
#include "vdec_v4l2_buffer_ops.h"

const struct file_operations v4l2_file_fops = {};

static int is_v4l2_buf_file(struct file *file)
{
	return file->f_op == &v4l2_file_fops;
}

static int vdec_v4l_alloc_fd(void)
{
	int fd;
	struct file *file = NULL;

	fd = get_unused_fd_flags(O_CLOEXEC);
	if (fd < 0) {
		pr_err("v4l: get unused fd fail\n");
		return fd;
	}

	file = anon_inode_getfile("v4l2_meta_file", &v4l2_file_fops, NULL, 0);
	if (IS_ERR(file)) {
		put_unused_fd(fd);
		pr_err("v4l: get file faill\n");
		return PTR_ERR(file);
	}

	fd_install(fd, file);

	pr_info("v4l: fd %d, file %p, data %p\n",
		fd, file, file->private_data);

	return fd;
}

static int vdec_v4l_fd_install_data(int fd, void *data)
{
	struct file *file;

	file = fget(fd);

	if (!file) {
		pr_info("v4l: fget fd %d fail!, comm %s, pid %d\n",
			fd, current->comm, current->pid);
		return -EBADF;
	}

	if (!is_v4l2_buf_file(file)) {
		pr_info("v4l: v4l2 check fd fail!\n");
		return -EBADF;
	}

	file->private_data = data;

	return 0;
}

int vdec_v4l_binding_fd_and_vf(ulong v4l_handle, void *vf)
{
	int ret = 0, fd = -1;
	struct vdec_v4l2_buffer *v4l_buf =
		(struct vdec_v4l2_buffer *) v4l_handle;

	if (v4l_buf->m.vf_fd > 0)
		return 0;

	fd = vdec_v4l_alloc_fd();
	if (fd < 0) {
		pr_err("v4l: alloc fd fail %d.\n", fd);
		return fd;
	}

	ret = vdec_v4l_fd_install_data(fd, vf);
	if (ret < 0) {
		put_unused_fd(fd);
		pr_err("v4l: fd install data fail %d.\n", ret);
		return ret;
	}

	v4l_buf->m.vf_fd = fd;

	return 0;
}
EXPORT_SYMBOL(vdec_v4l_binding_fd_and_vf);

int vdec_v4l_get_buffer(struct aml_vcodec_ctx *ctx,
	struct vdec_v4l2_buffer **out)
{
	int ret = 0;

	if (ctx->drv_handle == 0)
		return -EIO;

	ret = ctx->dec_if->get_param(ctx->drv_handle,
		GET_PARAM_FREE_FRAME_BUFFER, out);

	return ret;
}
EXPORT_SYMBOL(vdec_v4l_get_buffer);

int vdec_v4l_set_pic_infos(struct aml_vcodec_ctx *ctx,
	struct aml_vdec_pic_infos *info)
{
	int ret = 0;

	if (ctx->drv_handle == 0)
		return -EIO;

	ret = ctx->dec_if->set_param(ctx->drv_handle,
		SET_PARAM_PIC_INFO, info);

	return ret;
}
EXPORT_SYMBOL(vdec_v4l_set_pic_infos);

int vdec_v4l_write_frame_sync(struct aml_vcodec_ctx *ctx)
{
	int ret = 0;

	if (ctx->drv_handle == 0)
		return -EIO;

	ret = ctx->dec_if->set_param(ctx->drv_handle,
		SET_PARAM_WRITE_FRAME_SYNC, NULL);

	return ret;
}
EXPORT_SYMBOL(vdec_v4l_write_frame_sync);

