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

#include "cg_local.h"

#define LIGHT_INTENSITY 0.125

static cg_light_t cg_lights[MAX_LIGHTS];

/**
 * @brief
 */
void Cg_AddLight(const cg_light_t *s) {
	size_t i;

	if (s->decay > 1000)
		cg_lights[0].decay = 0;

	for (i = 0; i < lengthof(cg_lights); i++)
		if (cg_lights[i].radius == 0.0) {
			break;
		}

	if (i == lengthof(cg_lights)) {
		cgi.Debug("MAX_LIGHTS reached\n");
		return;
	}

	cg_lights[i] = *s;

	if (cg_lights[i].intensity == 0.0) {
		cg_lights[i].intensity = LIGHT_INTENSITY;
	}

	cg_lights[i].time = cgi.client->unclamped_time;
}

/**
 * @brief
 */
void Cg_AddLights(void) {

	cg_light_t *l = cg_lights;
	for (int32_t i = 0; i < MAX_LIGHTS; i++, l++) {

		const uint32_t expiration = l->time + l->decay;
		if (expiration < cgi.client->unclamped_time) {
			l->radius = 0.0;
			continue;
		}

		r_light_t out = {
			.origin = Vec3_ToVec4(l->origin, l->radius),
			.color = Vec3_ToVec4(l->color, l->intensity)
		};

		if (l->decay) {
			out.color.w = (expiration - cgi.client->unclamped_time) / (float) (l->decay);
		}

		cgi.AddLight(&out);
	}
}
