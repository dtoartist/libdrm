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

#ifndef TCC_DRMIF_H_
#define TCC_DRMIF_H_

#include <xf86drm.h>
#include <stdint.h>
#include "tcc_drm.h"

struct tcc_device {
	int fd;
};

/*
 * Exynos Buffer Object structure.
 *
 * @dev: tcc device object allocated.
 * @handle: a gem handle to gem object created.
 * @flags: indicate memory allocation and cache attribute types.
 * @size: size to the buffer created.
 * @vaddr: user space address to a gem buffer mmaped.
 * @name: a gem global handle from flink request.
 */
struct tcc_bo {
	struct tcc_device	*dev;
	uint32_t		handle;
	uint32_t		flags;
	size_t			size;
	void			*vaddr;
	uint32_t		name;
};

/*
 * device related functions:
 */
struct tcc_device * tcc_device_create(int fd);
void tcc_device_destroy(struct tcc_device *dev);

/*
 * buffer-object related functions:
 */
struct tcc_bo * tcc_bo_create(struct tcc_device *dev,
		size_t size, uint32_t flags);
int tcc_bo_get_info(struct tcc_device *dev, uint32_t handle,
			size_t *size, uint32_t *flags);
void tcc_bo_destroy(struct tcc_bo *bo);
struct tcc_bo * tcc_bo_from_name(struct tcc_device *dev, uint32_t name);
int tcc_bo_get_name(struct tcc_bo *bo, uint32_t *name);
uint32_t tcc_bo_handle(struct tcc_bo *bo);
void * tcc_bo_map(struct tcc_bo *bo);
int tcc_prime_handle_to_fd(struct tcc_device *dev, uint32_t handle,
					int *fd);
int tcc_prime_fd_to_handle(struct tcc_device *dev, int fd,
					uint32_t *handle);

/*
 * Virtual Display related functions:
 */
int tcc_vidi_connection(struct tcc_device *dev, uint32_t connect,
				uint32_t ext, void *edid);

#endif /* TCC_DRMIF_H_ */
