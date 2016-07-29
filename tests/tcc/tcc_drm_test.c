/*
 * Copyright (C) 2013 Samsung Electronics Co.Ltd
 * Authors:
 *	Inki Dae <inki.dae@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

#include <sys/mman.h>
#include <linux/stddef.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <libkms.h>
#include <drm_fourcc.h>

#include "tcc_drm.h"
#include "tcc_drmif.h"

#define DRM_MODULE_NAME		"tcc"

static unsigned int screen_width, screen_height;

struct connector {
	uint32_t id;
	char mode_str[64];
	drmModeModeInfo *mode;
	drmModeEncoder *encoder;
	int crtc;
};

static void connector_find_mode(int fd, struct connector *c,
				drmModeRes *resources)
{
	drmModeConnector *connector;
	int i, j;

	/* First, find the connector & mode */
	c->mode = NULL;
	for (i = 0; i < resources->count_connectors; i++) {
		connector = drmModeGetConnector(fd, resources->connectors[i]);

		if (!connector) {
			fprintf(stderr, "could not get connector %i: %s\n",
				resources->connectors[i], strerror(errno));
			drmModeFreeConnector(connector);
			continue;
		}

		if (!connector->count_modes) {
			drmModeFreeConnector(connector);
			continue;
		}

		if (connector->connector_id != c->id) {
			drmModeFreeConnector(connector);
			continue;
		}

		for (j = 0; j < connector->count_modes; j++) {
			c->mode = &connector->modes[j];
			if (!strcmp(c->mode->name, c->mode_str))
				break;
		}

		/* Found it, break out */
		if (c->mode)
			break;

		drmModeFreeConnector(connector);
	}

	if (!c->mode) {
		fprintf(stderr, "failed to find mode \"%s\"\n", c->mode_str);
		return;
	}

	/* Now get the encoder */
	for (i = 0; i < resources->count_encoders; i++) {
		c->encoder = drmModeGetEncoder(fd, resources->encoders[i]);

		if (!c->encoder) {
			fprintf(stderr, "could not get encoder %i: %s\n",
				resources->encoders[i], strerror(errno));
			drmModeFreeEncoder(c->encoder);
			continue;
		}

		if (c->encoder->encoder_id  == connector->encoder_id)
			break;

		drmModeFreeEncoder(c->encoder);
	}

	if (c->crtc == -1)
		c->crtc = c->encoder->crtc_id;
}

static int drm_set_crtc(struct tcc_device *dev, struct connector *c,
			unsigned int fb_id)
{
	int ret;

	ret = drmModeSetCrtc(dev->fd, c->crtc,
			fb_id, 0, 0, &c->id, 1, c->mode);
	if (ret)
		drmMsg("failed to set mode: %s\n", strerror(errno));

	return ret;
}

static struct tcc_bo *tcc_create_buffer(struct tcc_device *dev,
						unsigned long size,
						unsigned int flags)
{
	struct tcc_bo *bo;

	bo = tcc_bo_create(dev, size, flags);
	if (!bo)
		return bo;

	if (!tcc_bo_map(bo)) {
		tcc_bo_destroy(bo);
		return NULL;
	}

	return bo;
}

static void tcc_destroy_buffer(struct tcc_bo *bo)
{
	tcc_bo_destroy(bo);
}

static void wait_for_user_input(int last)
{
	printf("press <ENTER> to %s\n", last ? "exit test application" :
			"skip to next test");

	getchar();
}

static void usage(char *name)
{
	fprintf(stderr, "usage: %s [-s]\n", name);
	fprintf(stderr, "-s <connector_id>@<crtc_id>:<mode>\n");
	exit(0);
}

extern char *optarg;
static const char optstr[] = "s:";

int main(int argc, char **argv)
{
	struct tcc_device *dev;
	struct tcc_bo *bo, *src;
	struct connector con;
	unsigned int fb_id;
	uint32_t handles[4] = {0}, pitches[4] = {0}, offsets[4] = {0};
	drmModeRes *resources;
	int ret, fd, c;

	memset(&con, 0, sizeof(struct connector));

	if (argc != 3) {
		usage(argv[0]);
		return -EINVAL;
	}

	while ((c = getopt(argc, argv, optstr)) != -1) {
		switch (c) {
		case 's':
			con.crtc = -1;
			if (sscanf(optarg, "%d:0x%64s",
						&con.id,
						con.mode_str) != 2 &&
					sscanf(optarg, "%d@%d:%64s",
						&con.id,
						&con.crtc,
						con.mode_str) != 3)
				usage(argv[0]);
			break;
		default:
			usage(argv[0]);
			break;
		}
	}

	fd = drmOpen(DRM_MODULE_NAME, NULL);
	if (fd < 0) {
		fprintf(stderr, "failed to open.\n");
		return fd;
	}

	dev = tcc_device_create(fd);
	if (!dev) {
		drmClose(dev->fd);
		return -EFAULT;
	}

	resources = drmModeGetResources(dev->fd);
	if (!resources) {
		fprintf(stderr, "drmModeGetResources failed: %s\n",
				strerror(errno));
		ret = -EFAULT;
		goto err_drm_close;
	}

	connector_find_mode(dev->fd, &con, resources);
	drmModeFreeResources(resources);

	if (!con.mode) {
		fprintf(stderr, "failed to find usable connector\n");
		ret = -EFAULT;
		goto err_drm_close;
	}

	screen_width = con.mode->hdisplay;
	screen_height = con.mode->vdisplay;

	if (screen_width == 0 || screen_height == 0) {
		fprintf(stderr, "failed to find sane resolution on connector\n");
		ret = -EFAULT;
		goto err_drm_close;
	}

	printf("screen width = %d, screen height = %d\n", screen_width,
			screen_height);

	bo = tcc_create_buffer(dev, screen_width * screen_height * 4, 0);
	if (!bo) {
		ret = -EFAULT;
		goto err_drm_close;
	}

	handles[0] = bo->handle;
	pitches[0] = screen_width * 4;
	offsets[0] = 0;

	ret = drmModeAddFB2(dev->fd, screen_width, screen_height,
				DRM_FORMAT_RGBA8888, handles,
				pitches, offsets, &fb_id, 0);
	if (ret < 0)
		goto err_destroy_buffer;

	memset(bo->vaddr, 0xff, screen_width * screen_height * 4);

	ret = drm_set_crtc(dev, &con, fb_id);
	if (ret < 0)
		goto err_rm_fb;

	wait_for_user_input(0);

	src = tcc_create_buffer(dev, screen_width * screen_height * 4, 0);
	if (!src) {
		ret = -EFAULT;
		goto err_rm_fb;
	}

	wait_for_user_input(1);

err_free_src:
	if (src)
		tcc_destroy_buffer(src);

err_rm_fb:
	drmModeRmFB(dev->fd, fb_id);

err_destroy_buffer:
	tcc_destroy_buffer(bo);

err_drm_close:
	drmClose(dev->fd);
	tcc_device_destroy(dev);

	return 0;
}
