/*
 * Copyright (C) 2012 Samsung Electronics Co., Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Inki Dae <inki.dae@samsung.com>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <sys/mman.h>
#include <linux/stddef.h>

#include <xf86drm.h>

#include "libdrm_macros.h"
#include "tcc_drm.h"
#include "tcc_drmif.h"

/*
 * Create tcc drm device object.
 *
 * @fd: file descriptor to tcc drm driver opened.
 *
 * if true, return the device object else NULL.
 */
struct tcc_device * tcc_device_create(int fd)
{
	struct tcc_device *dev;

	dev = calloc(sizeof(*dev), 1);
	if (!dev) {
		fprintf(stderr, "failed to create device[%s].\n",
				strerror(errno));
		return NULL;
	}

	dev->fd = fd;

	return dev;
}

/*
 * Destroy tcc drm device object
 *
 * @dev: tcc drm device object.
 */
void tcc_device_destroy(struct tcc_device *dev)
{
	free(dev);
}

/*
 * Create a tcc buffer object to tcc drm device.
 *
 * @dev: tcc drm device object.
 * @size: user-desired size.
 * flags: user-desired memory type.
 *	user can set one or more types among several types to memory
 *	allocation and cache attribute types. and as default,
 *	TCC_BO_NONCONTIG and TCC-BO_NONCACHABLE types would
 *	be used.
 *
 * if true, return a tcc buffer object else NULL.
 */
struct tcc_bo * tcc_bo_create(struct tcc_device *dev,
					       size_t size, uint32_t flags)
{
	struct tcc_bo *bo;
	struct drm_tcc_gem_create req = {
		.size = size,
		.flags = flags,
	};

	if (size == 0) {
		fprintf(stderr, "invalid size.\n");
		goto fail;
	}

	bo = calloc(sizeof(*bo), 1);
	if (!bo) {
		fprintf(stderr, "failed to create bo[%s].\n",
				strerror(errno));
		goto err_free_bo;
	}

	bo->dev = dev;

	if (drmIoctl(dev->fd, DRM_IOCTL_TCC_GEM_CREATE, &req)){
		fprintf(stderr, "failed to create gem object[%s].\n",
				strerror(errno));
		goto err_free_bo;
	}

	bo->handle = req.handle;
	bo->size = size;
	bo->flags = flags;

	return bo;

err_free_bo:
	free(bo);
fail:
	return NULL;
}

/*
 * Get information to gem region allocated.
 *
 * @dev: tcc drm device object.
 * @handle: gem handle to request gem info.
 * @size: size to gem object and returned by kernel side.
 * @flags: gem flags to gem object and returned by kernel side.
 *
 * with this function call, you can get flags and size to gem handle
 * through bo object.
 *
 * if true, return 0 else negative.
 */
int tcc_bo_get_info(struct tcc_device *dev, uint32_t handle,
				  size_t *size, uint32_t *flags)
{
	int ret;
	struct drm_tcc_gem_info req = {
		.handle = handle,
	};

	ret = drmIoctl(dev->fd, DRM_IOCTL_TCC_GEM_GET, &req);
	if (ret < 0) {
		fprintf(stderr, "failed to get gem object information[%s].\n",
				strerror(errno));
		return ret;
	}

	*size = req.size;
	*flags = req.flags;

	return 0;
}

/*
 * Destroy a tcc buffer object.
 *
 * @bo: a tcc buffer object to be destroyed.
 */
void tcc_bo_destroy(struct tcc_bo *bo)
{
	if (!bo)
		return;

	if (bo->vaddr)
		munmap(bo->vaddr, bo->size);

	if (bo->handle) {
		struct drm_gem_close req = {
			.handle = bo->handle,
		};

		drmIoctl(bo->dev->fd, DRM_IOCTL_GEM_CLOSE, &req);
	}

	free(bo);
}


/*
 * Get a tcc buffer object from a gem global object name.
 *
 * @dev: a tcc device object.
 * @name: a gem global object name exported by another process.
 *
 * this interface is used to get a tcc buffer object from a gem
 * global object name sent by another process for buffer sharing.
 *
 * if true, return a tcc buffer object else NULL.
 *
 */
struct tcc_bo *
tcc_bo_from_name(struct tcc_device *dev, uint32_t name)
{
	struct tcc_bo *bo;
	struct drm_gem_open req = {
		.name = name,
	};

	bo = calloc(sizeof(*bo), 1);
	if (!bo) {
		fprintf(stderr, "failed to allocate bo[%s].\n",
				strerror(errno));
		return NULL;
	}

	if (drmIoctl(dev->fd, DRM_IOCTL_GEM_OPEN, &req)) {
		fprintf(stderr, "failed to open gem object[%s].\n",
				strerror(errno));
		goto err_free_bo;
	}

	bo->dev = dev;
	bo->name = name;
	bo->handle = req.handle;

	return bo;

err_free_bo:
	free(bo);
	return NULL;
}

/*
 * Get a gem global object name from a gem object handle.
 *
 * @bo: a tcc buffer object including gem handle.
 * @name: a gem global object name to be got by kernel driver.
 *
 * this interface is used to get a gem global object name from a gem object
 * handle to a buffer that wants to share it with another process.
 *
 * if true, return 0 else negative.
 */
int tcc_bo_get_name(struct tcc_bo *bo, uint32_t *name)
{
	if (!bo->name) {
		struct drm_gem_flink req = {
			.handle = bo->handle,
		};
		int ret;

		ret = drmIoctl(bo->dev->fd, DRM_IOCTL_GEM_FLINK, &req);
		if (ret) {
			fprintf(stderr, "failed to get gem global name[%s].\n",
					strerror(errno));
			return ret;
		}

		bo->name = req.name;
	}

	*name = bo->name;

	return 0;
}

uint32_t tcc_bo_handle(struct tcc_bo *bo)
{
	return bo->handle;
}

/*
 * Mmap a buffer to user space.
 *
 * @bo: a tcc buffer object including a gem object handle to be mmapped
 *	to user space.
 *
 * if true, user pointer mmaped else NULL.
 */
void *tcc_bo_map(struct tcc_bo *bo)
{
	if (!bo->vaddr) {
		struct tcc_device *dev = bo->dev;
		struct drm_mode_map_dumb arg;
		void *map = NULL;
		int ret;

		memset(&arg, 0, sizeof(arg));
		arg.handle = bo->handle;

		ret = drmIoctl(dev->fd, DRM_IOCTL_MODE_MAP_DUMB, &arg);
		if (ret) {
			fprintf(stderr, "failed to map dumb buffer[%s].\n",
				strerror(errno));
			return NULL;
		}

		map = drm_mmap(0, bo->size, PROT_READ | PROT_WRITE, MAP_SHARED,
				dev->fd, arg.offset);

		if (map != MAP_FAILED)
			bo->vaddr = map;
	}

	return bo->vaddr;
}

/*
 * Export gem object to dmabuf as file descriptor.
 *
 * @dev: tcc device object
 * @handle: gem handle to export as file descriptor of dmabuf
 * @fd: file descriptor returned from kernel
 *
 * @return: 0 on success, -1 on error, and errno will be set
 */
int
tcc_prime_handle_to_fd(struct tcc_device *dev, uint32_t handle, int *fd)
{
	return drmPrimeHandleToFD(dev->fd, handle, 0, fd);
}

/*
 * Import file descriptor into gem handle.
 *
 * @dev: tcc device object
 * @fd: file descriptor of dmabuf to import
 * @handle: gem handle returned from kernel
 *
 * @return: 0 on success, -1 on error, and errno will be set
 */
int
tcc_prime_fd_to_handle(struct tcc_device *dev, int fd, uint32_t *handle)
{
	return drmPrimeFDToHandle(dev->fd, fd, handle);
}



/*
 * Request Wireless Display connection or disconnection.
 *
 * @dev: a tcc device object.
 * @connect: indicate whether connectoin or disconnection request.
 * @ext: indicate whether edid data includes extentions data or not.
 * @edid: a pointer to edid data from Wireless Display device.
 *
 * this interface is used to request Virtual Display driver connection or
 * disconnection. for this, user should get a edid data from the Wireless
 * Display device and then send that data to kernel driver with connection
 * request
 *
 * if true, return 0 else negative.
 */
int
tcc_vidi_connection(struct tcc_device *dev, uint32_t connect,
		       uint32_t ext, void *edid)
{
	struct drm_tcc_vidi_connection req = {
		.connection	= connect,
		.extensions	= ext,
		.edid		= (uint64_t)(uintptr_t)edid,
	};
	int ret;

	ret = drmIoctl(dev->fd, DRM_IOCTL_TCC_VIDI_CONNECTION, &req);
	if (ret) {
		fprintf(stderr, "failed to request vidi connection[%s].\n",
				strerror(errno));
		return ret;
	}

	return 0;
}

/*
 * Request change tcc hardware overlay priority.
 *
 * @dev: a tcc device object.
 * @ovp: a value to overlay priority value from user space.
 *
 * this interface is used to request changing tcc hardware overlay priority
 * for this, user should set properly overlay value by tcc documentation.
 *
 * if true, return 0 else negative.
 */
int
tcc_lcd_ovp(struct tcc_device *dev, uint32_t ovp)
{
	struct drm_tcc_lcd_ovp req = {
		.ovp_order = ovp,
	};
	int ret;

	ret = drmIoctl(dev->fd, DRM_IOCTL_TCC_LCD_OVP, &req);
	if (ret) {
		fprintf(stderr, "failed to request change tcc overlay priority[%s].\n",
				strerror(errno));
		return ret;
	}

	return 0;
}
