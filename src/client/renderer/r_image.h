/*
 * Copyright(c) 1997-2001 id Software, Inc.
 * Copyright(c) 2002 The Quakeforge Project.
 * Copyright(c) 2006 Quetoo.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#pragma once

#include "r_types.h"

r_image_t *R_LoadImage(const char *name, r_image_type_t type);

#ifdef __R_LOCAL_H__

typedef struct {
	GLint filter_min;
	GLint filter_mag;

	GLfloat anisotropy;

	r_image_t *null;
	r_image_t *shell;
} r_image_state_t;

extern r_image_state_t r_image_state;

void R_FilterImage(r_image_t *image, GLenum format, byte *data);
void R_UploadImage(r_image_t *image, GLenum format, byte *data);
void R_Screenshot_f(void);
void R_DumpImage(const r_image_t *image, const char *output);
void R_DumpImages_f(void);
void R_InitImages(void);

void R_FreeImage(r_media_t *media);
_Bool R_RetainImage(r_media_t *self);

#endif /* __R_LOCAL_H__ */
