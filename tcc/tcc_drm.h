/* tcc_drm.h
 *
 * Copyright (C) 2015-2016 Telechips
 *
 * Author: Telechips Inc.
 * Based on Samsung Exynos code
 *
 *---------------------------------------------------------------------------
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 * Authors:
 *	Inki Dae <inki.dae@samsung.com>
 *	Joonyoung Shim <jy0922.shim@samsung.com>
 *	Seung-Woo Kim <sw0312.kim@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef _TCC_DRM_H_
#define _TCC_DRM_H_

#include "drm.h"

/**
 * User-desired buffer creation information structure.
 *
 * @size: user-desired memory allocation size.
 *	- this size value would be page-aligned internally.
 * @flags: user request for setting memory type or cache attributes.
 * @handle: returned a handle to created gem object.
 *	- this handle will be set by gem module of kernel side.
 */
struct drm_tcc_gem_create {
	uint64_t size;
	unsigned int flags;
	unsigned int handle;
};

/**
 * A structure to gem information.
 *
 * @handle: a handle to gem object created.
 * @flags: flag value including memory type and cache attribute and
 *	this value would be set by driver.
 * @size: size to memory region allocated by gem and this size would
 *	be set by driver.
 */
struct drm_tcc_gem_info {
	unsigned int handle;
	unsigned int flags;
	uint64_t size;
};

/**
 * A structure for user connection request of virtual display.
 *
 * @connection: indicate whether doing connetion or not by user.
 * @extensions: if this value is 1 then the vidi driver would need additional
 *      128bytes edid data.
 * @edid: the edid data pointer from user side.
 */
struct drm_tcc_vidi_connection {
    unsigned int connection;
    unsigned int extensions;
    uint64_t edid;
};


/* memory type definitions. */
enum e_drm_tcc_gem_mem_type {
	/* Physically Continuous memory and used as default. */
	TCC_BO_CONTIG	= 0 << 0,
	/* Physically Non-Continuous memory. */
	TCC_BO_NONCONTIG	= 1 << 0,
	/* non-cachable mapping and used as default. */
	TCC_BO_NONCACHABLE	= 0 << 1,
	/* cachable mapping. */
	TCC_BO_CACHABLE	= 1 << 1,
	/* write-combine mapping. */
	TCC_BO_WC		= 1 << 2,
	TCC_BO_MASK = TCC_BO_NONCONTIG | TCC_BO_CACHABLE | TCC_BO_WC
};

/* tcc specific overlay priority change. */
struct drm_tcc_lcd_ovp {
    unsigned int ovp_order;
};

#define DRM_TCC_GEM_CREATE		0x00
/* Reserved 0x03 ~ 0x05 for tcc specific gem ioctl */
#define DRM_TCC_GEM_GET		0x04
#define DRM_TCC_LCD_OVP		0x06
#define DRM_TCC_VIDI_CONNECTION	0x07

#define DRM_IOCTL_TCC_GEM_CREATE		DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_TCC_GEM_CREATE, struct drm_tcc_gem_create)

#define DRM_IOCTL_TCC_GEM_GET	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_TCC_GEM_GET,	struct drm_tcc_gem_info)

#define DRM_IOCTL_TCC_LCD_OVP	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_TCC_LCD_OVP,	struct drm_tcc_lcd_ovp)

#define DRM_IOCTL_TCC_VIDI_CONNECTION	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_TCC_VIDI_CONNECTION, struct drm_tcc_vidi_connection)

#endif /* _UAPI_TCC_DRM_H_ */
