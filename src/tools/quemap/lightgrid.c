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

#include "bsp.h"
#include "lightmap.h"
#include "lightgrid.h"
#include "points.h"
#include "qlight.h"

typedef struct {
	vec3_t stu_mins, stu_maxs;
	vec3i_t size;

	mat4_t matrix;
	mat4_t inverse_matrix;

	size_t num_luxels;
	luxel_t *luxels;
} lightgrid_t;

static lightgrid_t lg;

/**
 * @brief
 */
static void BuildLightgridMatrices(void) {

	const bsp_model_t *world = bsp_file.models;

	Matrix4x4_CreateTranslate(&lg.matrix, -world->mins.x, -world->mins.y, -world->mins.z);
	Matrix4x4_ConcatScale3(&lg.matrix,
						   1.0 / BSP_LIGHTGRID_LUXEL_SIZE,
						   1.0 / BSP_LIGHTGRID_LUXEL_SIZE,
						   1.0 / BSP_LIGHTGRID_LUXEL_SIZE);

	Matrix4x4_Invert_Full(&lg.inverse_matrix, &lg.matrix);
}

/**
 * @brief
 */
static void BuildLightgridExtents(void) {

	const bsp_model_t *world = bsp_file.models;

	const vec3_t mins = Vec3(world->mins.x, world->mins.y, world->mins.z);
	const vec3_t maxs = Vec3(world->maxs.x, world->maxs.y, world->maxs.z);

	Matrix4x4_Transform(&lg.matrix, mins.xyz, lg.stu_mins.xyz);
	Matrix4x4_Transform(&lg.matrix, maxs.xyz, lg.stu_maxs.xyz);

	for (int32_t i = 0; i < 3; i++) {
		lg.size.xyz[i] = floorf(lg.stu_maxs.xyz[i] - lg.stu_mins.xyz[i]) + 2;
	}
}

/**
 * @brief
 */
static void BuildLightgridLuxels(void) {

	lg.num_luxels = lg.size.x * lg.size.y * lg.size.z;

	if (lg.num_luxels > MAX_BSP_LIGHTGRID_LUXELS) {
		Com_Error(ERROR_FATAL, "MAX_BSP_LIGHTGRID_LUXELS\n");
	}

	lg.luxels = Mem_TagMalloc(lg.num_luxels * sizeof(luxel_t), MEM_TAG_LIGHTGRID);

	luxel_t *l = lg.luxels;

	for (int32_t u = 0; u < lg.size.z; u++) {
		for (int32_t t = 0; t < lg.size.y; t++) {
			for (int32_t s = 0; s < lg.size.x; s++, l++) {

				l->s = s;
				l->t = t;
				l->u = u;
			}
		}
	}
}

/**
 * @brief
 */
static int32_t ProjectLightgridLuxel(luxel_t *l, float soffs, float toffs, float uoffs) {

	const float padding_s = ((lg.stu_maxs.x - lg.stu_mins.x) - lg.size.x) * 0.5;
	const float padding_t = ((lg.stu_maxs.y - lg.stu_mins.y) - lg.size.y) * 0.5;
	const float padding_u = ((lg.stu_maxs.z - lg.stu_mins.z) - lg.size.z) * 0.5;

	const float s = lg.stu_mins.x + padding_s + l->s + 0.5 + soffs;
	const float t = lg.stu_mins.y + padding_t + l->t + 0.5 + toffs;
	const float u = lg.stu_mins.z + padding_u + l->u + 0.5 + uoffs;

	const vec3_t stu = Vec3(s, t, u);
	Matrix4x4_Transform(&lg.inverse_matrix, stu.xyz, l->origin.xyz);

	return Light_PointContents(l->origin, 0);
}

/**
 * @brief Authors a .map file which can be imported into Radiant to view the luxel projections.
 */
static void DebugLightgridLuxels(void) {
#if 0
	const char *path = va("maps/%s.lightgrid.map", map_base);

	file_t *file = Fs_OpenWrite(path);
	if (file == NULL) {
		Com_Warn("Failed to open %s\n", path);
		return;
	}

	luxel_t *l = lg.luxels;
	for (size_t i = 0; i < lg.num_luxels; i++, l++) {

		ProjectLightgridLuxel(l, 0.0, 0.0, 0.0);

		Fs_Print(file, "{\n");
		Fs_Print(file, "  \"classname\" \"info_luxel\"\n");
		Fs_Print(file, "  \"origin\" \"%g %g %g\"\n", l->origin.x, l->origin.y, l->origin.z);
		Fs_Print(file, "  \"s\" \"%d\"\n", l->s);
		Fs_Print(file, "  \"t\" \"%d\"\n", l->t);
		Fs_Print(file, "  \"u\" \"%d\"\n", l->u);
		Fs_Print(file, "}\n");
	}

	Fs_Close(file);
#endif
}

/**
 * @brief
 */
size_t BuildLightgrid(void) {

	memset(&lg, 0, sizeof(lg));

	BuildLightgridMatrices();

	BuildLightgridExtents();

	BuildLightgridLuxels();

	DebugLightgridLuxels();

	return lg.num_luxels;
}

/**
 * @brief Iterates the specified lights, accumulating color and direction to the appropriate buffers.
 * @param lights The lights to iterate.
 * @param luxel The luxel to light.
 * @param scale A scalar applied to both light and direction.
 */
static void LightLuxel(const GPtrArray *lights, luxel_t *luxel, float scale) {

	for (guint i = 0; i < lights->len; i++) {

		const light_t *light = g_ptr_array_index(lights, i);

		float dist_squared = 0.f;
		switch (light->type) {
			case LIGHT_SUN:
				break;
			default:
				dist_squared = Vec3_DistanceSquared(light->origin, luxel->origin);
				break;
		}

		if (light->atten != LIGHT_ATTEN_NONE) {
			if (dist_squared > light->radius * light->radius) {
				continue;
			}
		}

		const float dist = sqrtf(dist_squared);

		vec3_t dir;
		if (light->type == LIGHT_SUN) {
			dir = Vec3_Negate(light->normal);
		} else {
			dir = Vec3_Normalize(Vec3_Subtract(light->origin, luxel->origin));
		}

		float intensity = Clampf(light->radius, 0.f, MAX_WORLD_COORD);

		switch (light->type) {
			case LIGHT_SPOT: {
				const float cone_dot = Vec3_Dot(dir, light->normal);
				const float thresh = cosf(light->theta);
				const float smooth = 0.03f;
				intensity *= Smoothf(thresh - smooth, thresh + smooth, cone_dot);
			}
				break;
			default:
				break;
		}

		float atten = 1.f;

		switch (light->atten) {
			case LIGHT_ATTEN_NONE:
				break;
			case LIGHT_ATTEN_LINEAR:
				atten = Clampf(1.f - dist / light->radius, 0.f, 1.f);
				break;
			case LIGHT_ATTEN_INVERSE_SQUARE:
				atten = Clampf(1.f - dist / light->radius, 0.f, 1.f);
				atten *= atten;
				break;
		}

		intensity *= atten;

		if (intensity <= 0.f) {
			continue;
		}

		if (light->type == LIGHT_AMBIENT) {

			const bsp_model_t *world = bsp_file.models;

			const float padding_s = (world->maxs.x - world->mins.x) - (lg.size.x * BSP_LIGHTGRID_LUXEL_SIZE) * 0.5f;
			const float padding_t = (world->maxs.y - world->mins.y) - (lg.size.y * BSP_LIGHTGRID_LUXEL_SIZE) * 0.5f;
			const float padding_u = (world->maxs.z - world->mins.z) - (lg.size.z * BSP_LIGHTGRID_LUXEL_SIZE) * 0.5f;

			const float s = lg.stu_mins.x + padding_s + luxel->s + 0.5f;
			const float t = lg.stu_mins.y + padding_t + luxel->t + 0.5f;
			const float u = lg.stu_mins.z + padding_u + luxel->u + 0.5f;

			const vec3_t points[] = CUBE_8;
			const float ao_radius = 64.f;

			float occlusion = 0.f;
			float sample_fraction = 1.f / lengthof(points);

			for (size_t i = 0; i < lengthof(points); i++) {

				vec3_t sample = points[i];

				// Add some jitter to hide undersampling
				sample.x += RandomRangef(-.04f, .04f);
				sample.y += RandomRangef(-.04f, .04f);
				sample.z += RandomRangef(-.04f, .04f);

				// Scale the sample and move it into position
				sample = Vec3_Scale(sample, ao_radius);

				sample.x += s;
				sample.y += t;
				sample.z += u;

				vec3_t point;
				Matrix4x4_Transform(&lg.inverse_matrix, sample.xyz, point.xyz);

				const cm_trace_t trace = Light_Trace(luxel->origin, point, 0, CONTENTS_SOLID);

				occlusion += sample_fraction * trace.fraction;
			}

			intensity *= 1.f - (1.f - occlusion) * (1.f - occlusion);

		} else if (light->type == LIGHT_SUN) {

			const vec3_t sun_origin = Vec3_Add(luxel->origin, Vec3_Scale(light->normal, -MAX_WORLD_DIST));

			cm_trace_t trace = Light_Trace(luxel->origin, sun_origin, 0, CONTENTS_SOLID);
			if (!(trace.texinfo && (trace.texinfo->flags & SURF_SKY))) {
				float exposure = 0.f;

				const int32_t num_samples = ceilf(light->size / LIGHT_SIZE_STEP);
				for (int32_t i = 0; i < num_samples; i++) {

					const vec3_t points[] = CUBE_8;
					for (size_t j = 0; j < lengthof(points); j++) {

						const vec3_t point = Vec3_Add(sun_origin, Vec3_Scale(points[j], i * LIGHT_SIZE_STEP));

						trace = Light_Trace(luxel->origin, point, 0, CONTENTS_SOLID);
						if (!(trace.texinfo && (trace.texinfo->flags & SURF_SKY))) {
							continue;
						}

						exposure += 1.f / num_samples;
						break;
					}
				}

				intensity *= exposure;
			}

		} else {
			cm_trace_t trace = Light_Trace(luxel->origin, light->origin, 0, CONTENTS_SOLID);
			if (trace.fraction < 1.f) {
				float exposure = 0.f;

				const int32_t num_samples = ceilf(light->size / LIGHT_SIZE_STEP);
				for (int32_t i = 0; i < num_samples; i++) {

					const vec3_t points[] = CUBE_8;
					for (size_t j = 0; j < lengthof(points); j++) {

						const vec3_t point = Vec3_Add(light->origin, Vec3_Scale(points[j], (i + 1) * LIGHT_SIZE_STEP));

						trace = Light_Trace(luxel->origin, point, 0, CONTENTS_SOLID);
						if (trace.fraction < 1.f) {
							continue;
						}

						exposure += 1.f / num_samples;
						break;
					}
				}

				intensity *= exposure;
			}
		}

		intensity *= scale;

		if (intensity <= 0.f) {
			continue;
		}

		switch (light->type) {
			case LIGHT_INVALID:
				break;
			case LIGHT_AMBIENT:
				luxel->ambient = Vec3_Add(luxel->ambient, Vec3_Scale(light->color, intensity));
				break;
			case LIGHT_SUN:
			case LIGHT_POINT:
			case LIGHT_SPOT:
			case LIGHT_PATCH:
				luxel->diffuse = Vec3_Add(luxel->diffuse, Vec3_Scale(light->color, intensity));
				luxel->direction = Vec3_Add(luxel->direction, Vec3_Scale(dir, intensity));
				break;
			case LIGHT_INDIRECT:
				luxel->radiosity[bounce] = Vec3_Add(luxel->radiosity[bounce], Vec3_Scale(light->color, intensity));
				break;
		}
	}
}

/**
 * @brief
 */
void DirectLightgrid(int32_t luxel_num) {

	const vec3_t offsets[] = {
		Vec3(+0.00f, +0.00f, +0.00f),
		Vec3(-0.25f, -0.25f, -0.25f), Vec3(-0.25f, +0.25f, -0.25f),
		Vec3(+0.25f, -0.25f, -0.25f), Vec3(+0.25f, +0.25f, -0.25f),
		Vec3(-0.25f, -0.25f, +0.25f), Vec3(-0.25f, +0.25f, +0.25f),
		Vec3(+0.25f, -0.25f, +0.25f), Vec3(+0.25f, +0.25f, +0.25f),
	};

	luxel_t *l = &lg.luxels[luxel_num];

	for (size_t i = 0; i < lengthof(offsets); i++) {

		const float soffs = offsets[i].x;
		const float toffs = offsets[i].y;
		const float uoffs = offsets[i].z;

		if (ProjectLightgridLuxel(l, soffs, toffs, uoffs) == CONTENTS_SOLID) {
			continue;
		}

		const GPtrArray *lights = leaf_lights[Cm_PointLeafnum(l->origin, 0)];
		if (!lights) {
			continue;
		}

		LightLuxel(lights, l, 1.f);
		break;
	}
}

/**
 * @brief
 */
void IndirectLightgrid(int32_t luxel_num) {

	const vec3_t offsets[] = {
		Vec3(+0.00f, +0.00f, +0.00f),
		Vec3(-0.25f, -0.25f, -0.25f), Vec3(-0.25f, +0.25f, -0.25f),
		Vec3(+0.25f, -0.25f, -0.25f), Vec3(+0.25f, +0.25f, -0.25f),
		Vec3(-0.25f, -0.25f, +0.25f), Vec3(-0.25f, +0.25f, +0.25f),
		Vec3(+0.25f, -0.25f, +0.25f), Vec3(+0.25f, +0.25f, +0.25f),
	};
	const size_t num_offsets = antialias ? lengthof(offsets) : 1;

	luxel_t *l = &lg.luxels[luxel_num];

	for (size_t i = 0; i < num_offsets; i++) {

		const float soffs = offsets[i].x;
		const float toffs = offsets[i].y;
		const float uoffs = offsets[i].z;

		if (ProjectLightgridLuxel(l, soffs, toffs, uoffs) == CONTENTS_SOLID) {
			continue;
		}

		const GPtrArray *lights = leaf_lights[Cm_PointLeafnum(l->origin, 0)];
		if (!lights) {
			continue;
		}

		LightLuxel(lights, l, 1.f);
	}
}

/**
 * @brief
 */
static void FogLuxel(GArray *fogs, luxel_t *l, float scale) {

	const fog_t *fog = (fog_t *) fogs->data;
	for (guint i = 0; i < fogs->len; i++, fog++) {

		float intensity = 1.f;

		switch (fog->type) {
			case FOG_VOLUME:
				if (!PointInsideFog(l->origin, fog)) {
					intensity = 0.f;
				}
				break;
			default:
				break;
		}

		intensity *= fog->density + RandomRangef(-fog->noise, fog->noise);

		intensity = Clampf(intensity, 0.f, 1.f);

		if (intensity == 0.f) {
			continue;
		}

		const float diffuse = Clampf(Vec3_Length(l->diffuse) / DEFAULT_LIGHT, 0.f, 1.f);

		const float absorption = Clampf(diffuse * fog->absorption, 0.f, 1.f);

		const vec3_t color = Vec3_Mix(fog->color, l->diffuse, absorption);

		switch (fog->type) {
			case FOG_INVALID:
				break;
			case FOG_GLOBAL:
			case FOG_VOLUME:
				l->fog = Vec4_Add(l->fog, Vec3_ToVec4(color, intensity));
				break;
		}
	}
}

/**
 * @brief
 */
void FogLightgrid(int32_t luxel_num) {

	const vec3_t offsets[] = {
		Vec3(+0.00f, +0.00f, +0.00f),
		Vec3(-0.25f, -0.25f, -0.25f), Vec3(-0.25f, +0.25f, -0.25f),
		Vec3(+0.25f, -0.25f, -0.25f), Vec3(+0.25f, +0.25f, -0.25f),
		Vec3(-0.25f, -0.25f, +0.25f), Vec3(-0.25f, +0.25f, +0.25f),
		Vec3(+0.25f, -0.25f, +0.25f), Vec3(+0.25f, +0.25f, +0.25f),
	};
	const size_t num_offsets = antialias ? lengthof(offsets) : 1;

	luxel_t *l = &lg.luxels[luxel_num];

	for (size_t i = 0; i < num_offsets; i++) {

		const float soffs = offsets[i].x;
		const float toffs = offsets[i].y;
		const float uoffs = offsets[i].z;

		if (ProjectLightgridLuxel(l, soffs, toffs, uoffs) == CONTENTS_SOLID) {
			continue;
		}

		FogLuxel(fogs, l, 1.f);

		l->fog = Vec3_ToVec4(ColorFilter(Vec4_XYZ(l->fog)), Clampf(l->fog.w, 0.f, 1.f));
	}
}

/**
 * @brief
 */
void FinalizeLightgrid(int32_t luxel_num) {

	luxel_t *l = &lg.luxels[luxel_num];

	for (int32_t i = 0; i < num_bounces; i++) {
		l->ambient = Vec3_Add(l->ambient, l->radiosity[i]);
	}

	l->ambient = Vec3_Scale(l->ambient, 1.f / 255.f);
	l->ambient = ColorFilter(l->ambient);

	l->diffuse = Vec3_Scale(l->diffuse, 1.f / 255.f);
	l->diffuse = ColorFilter(l->diffuse);

	l->direction = Vec3_Add(l->direction, Vec3_Up());
	l->direction = Vec3_Normalize(l->direction);
}

/**
 * @brief
 */
static SDL_Surface *CreateLightgridSurfaceFrom(void *pixels, int32_t w, int32_t h) {
	return SDL_CreateRGBSurfaceWithFormatFrom(pixels, w, h, 24, w * BSP_LIGHTMAP_BPP, SDL_PIXELFORMAT_RGB24);
}

/**
 * @brief
 */
static SDL_Surface *CreateFogSurfaceFrom(void *pixels, int32_t w, int32_t h) {
	return SDL_CreateRGBSurfaceWithFormatFrom(pixels, w, h, 32, w * BSP_FOG_BPP, SDL_PIXELFORMAT_RGBA32);
}

/**
 * @brief
 */
void EmitLightgrid(void) {

	const size_t lightgrid_bytes = lg.num_luxels * BSP_LIGHTGRID_TEXTURES * BSP_LIGHTGRID_BPP;
	const size_t fog_bytes = lg.num_luxels * BSP_FOG_TEXTURES * BSP_FOG_BPP;

	bsp_file.lightgrid_size = (int32_t) (sizeof(bsp_lightgrid_t) + lightgrid_bytes + fog_bytes);

	Bsp_AllocLump(&bsp_file, BSP_LUMP_LIGHTGRID, bsp_file.lightgrid_size);

	bsp_file.lightgrid->size = lg.size;

	byte *out = (byte *) bsp_file.lightgrid + sizeof(bsp_lightgrid_t);

	byte *out_ambient = out + 0 * lg.num_luxels * BSP_LIGHTGRID_BPP;
	byte *out_diffuse = out + 1 * lg.num_luxels * BSP_LIGHTGRID_BPP;
	byte *out_direction = out + 2 * lg.num_luxels * BSP_LIGHTGRID_BPP;
	byte *out_fog = out + 3 * lg.num_luxels * BSP_LIGHTGRID_BPP;

	const luxel_t *l = lg.luxels;
	for (int32_t u = 0; u < lg.size.z; u++) {

		SDL_Surface *ambient = CreateLightgridSurfaceFrom(out_ambient, lg.size.x, lg.size.y);
		SDL_Surface *diffuse = CreateLightgridSurfaceFrom(out_diffuse, lg.size.x, lg.size.y);
		SDL_Surface *direction = CreateLightgridSurfaceFrom(out_direction, lg.size.x, lg.size.y);
		SDL_Surface *fog = CreateFogSurfaceFrom(out_fog, lg.size.x, lg.size.y);

		for (int32_t t = 0; t < lg.size.y; t++) {
			for (int32_t s = 0; s < lg.size.x; s++, l++) {

				for (int32_t i = 0; i < BSP_LIGHTGRID_BPP; i++) {
					*out_ambient++ = (byte) Clampf(l->ambient.xyz[i] * 255.f, 0.f, 255.f);
					*out_diffuse++ = (byte) Clampf(l->diffuse.xyz[i] * 255.f, 0.f, 255.f);
					*out_direction++ = (byte) Clampf((l->direction.xyz[i] + 1.f) * 0.5f * 255.f, 0.f, 255.f);
				}

				for (int32_t i = 0; i < BSP_FOG_BPP; i++) {
					*out_fog++ = (byte) Clampf(l->fog.xyzw[i] * 255.f, 0.f, 255.f);
				}
			}
		}

//		IMG_SavePNG(ambient, va("/tmp/%s_lg_ambient_%d.png", map_base, u));
//		IMG_SavePNG(diffuse, va("/tmp/%s_lg_diffuse_%d.png", map_base, u));
//		IMG_SavePNG(direction, va("/tmp/%s_lg_direction_%d.png", map_base, u));
//		IMG_SavePNG(fog, va("/tmp/%s_lg_fog_%d.png", map_base, u));

		SDL_FreeSurface(ambient);
		SDL_FreeSurface(diffuse);
		SDL_FreeSurface(direction);
		SDL_FreeSurface(fog);
	}
}
