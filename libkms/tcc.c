/* tcc.c
 *
 * Copyright 2009 Samsung Electronics Co., Ltd.
 * Authors:
 *	SooChan Lim <sc1.lim@samsung.com>
 *      Sangjin LEE <lsj119@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "internal.h"

#include <sys/mman.h>
#include <sys/ioctl.h>
#include "xf86drm.h"

#include "libdrm_macros.h"
#include "tcc_drm.h"

struct tcc_bo
{
	struct kms_bo base;
	unsigned map_count;
};

static int
tcc_get_prop(struct kms_driver *kms, unsigned key, unsigned *out)
{
	switch (key) {
	case KMS_BO_TYPE:
		*out = KMS_BO_TYPE_SCANOUT_X8R8G8B8 | KMS_BO_TYPE_CURSOR_64X64_A8R8G8B8;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int
tcc_destroy(struct kms_driver *kms)
{
	free(kms);
	return 0;
}

static int
tcc_bo_create(struct kms_driver *kms,
		 const unsigned width, const unsigned height,
		 const enum kms_bo_type type, const unsigned *attr,
		 struct kms_bo **out)
{
	struct drm_tcc_gem_create arg;
	unsigned size, pitch;
	struct tcc_bo *bo;
	int i, ret;

	for (i = 0; attr[i]; i += 2) {
		switch (attr[i]) {
		case KMS_WIDTH:
		case KMS_HEIGHT:
		case KMS_BO_TYPE:
			break;
		default:
			return -EINVAL;
		}
	}

	bo = calloc(1, sizeof(*bo));
	if (!bo)
		return -ENOMEM;

	if (type == KMS_BO_TYPE_CURSOR_64X64_A8R8G8B8) {
		pitch = 64 * 4;
		size = 64 * 64 * 4;
	} else if (type == KMS_BO_TYPE_SCANOUT_X8R8G8B8) {
		pitch = width * 4;
		pitch = (pitch + 512 - 1) & ~(512 - 1);
		size = pitch * ((height + 4 - 1) & ~(4 - 1));
	} else {
		return -EINVAL;
	}

	memset(&arg, 0, sizeof(arg));
	arg.size = size;

	ret = drmCommandWriteRead(kms->fd, DRM_TCC_GEM_CREATE, &arg, sizeof(arg));
	if (ret)
		goto err_free;

	bo->base.kms = kms;
	bo->base.handle = arg.handle;
	bo->base.size = size;
	bo->base.pitch = pitch;

	*out = &bo->base;

	return 0;

err_free:
	free(bo);
	return ret;
}

static int
tcc_bo_get_prop(struct kms_bo *bo, unsigned key, unsigned *out)
{
	switch (key) {
	default:
		return -EINVAL;
	}
}

static int
tcc_bo_map(struct kms_bo *_bo, void **out)
{
	struct tcc_bo *bo = (struct tcc_bo *)_bo;
	struct drm_mode_map_dumb arg;
	void *map = NULL;
	int ret;

	if (bo->base.ptr) {
		bo->map_count++;
		*out = bo->base.ptr;
		return 0;
	}

	memset(&arg, 0, sizeof(arg));
	arg.handle = bo->base.handle;

	ret = drmIoctl(bo->base.kms->fd, DRM_IOCTL_MODE_MAP_DUMB, &arg);
	if (ret)
		return ret;

	map = drm_mmap(0, bo->base.size, PROT_READ | PROT_WRITE, MAP_SHARED, bo->base.kms->fd, arg.offset);
	if (map == MAP_FAILED)
		return -errno;

	bo->base.ptr = map;
	bo->map_count++;
	*out = bo->base.ptr;

	return 0;
}

static int
tcc_bo_unmap(struct kms_bo *_bo)
{
	struct tcc_bo *bo = (struct tcc_bo *)_bo;
	bo->map_count--;
	return 0;
}

static int
tcc_bo_destroy(struct kms_bo *_bo)
{
	struct tcc_bo *bo = (struct tcc_bo *)_bo;
	struct drm_gem_close arg;
	int ret;

	if (bo->base.ptr) {
		/* XXX Sanity check map_count */
		munmap(bo->base.ptr, bo->base.size);
		bo->base.ptr = NULL;
	}

	memset(&arg, 0, sizeof(arg));
	arg.handle = bo->base.handle;

	ret = drmIoctl(bo->base.kms->fd, DRM_IOCTL_GEM_CLOSE, &arg);
	if (ret)
		return -errno;

	free(bo);
	return 0;
}

drm_private int
tcc_create(int fd, struct kms_driver **out)
{
	struct kms_driver *kms;

	kms = calloc(1, sizeof(*kms));
	if (!kms)
		return -ENOMEM;

	kms->fd = fd;

	kms->bo_create = tcc_bo_create;
	kms->bo_map = tcc_bo_map;
	kms->bo_unmap = tcc_bo_unmap;
	kms->bo_get_prop = tcc_bo_get_prop;
	kms->bo_destroy = tcc_bo_destroy;
	kms->get_prop = tcc_get_prop;
	kms->destroy = tcc_destroy;
	*out = kms;

	return 0;
}
