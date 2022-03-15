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

uniform sampler2DArray texture_material;
uniform sampler2DArray texture_lightmap;
uniform sampler2D texture_stage;
uniform sampler2D texture_warp;
uniform sampler3D texture_lightgrid_ambient;
uniform sampler3D texture_lightgrid_diffuse;
uniform sampler3D texture_lightgrid_direction;
uniform sampler3D texture_lightgrid_caustics;
uniform sampler3D texture_lightgrid_fog;

uniform int entity;

uniform int bicubic;

uniform material_t material;
uniform stage_t stage;

in vertex_data {
	vec3 model;
	vec3 position;
	vec3 normal;
	vec3 tangent;
	vec3 bitangent;
	vec2 diffusemap;
	vec2 lightmap;
	vec3 lightgrid;
	vec4 color;
} vertex;

layout (location = 0) out vec4 out_color;
layout (location = 1) out vec4 out_bloom;

/**
 * @brief Samples the lightmap and stainmap with either bilinear or bicubic sampling.
 */
vec4 sample_lightmap(int index) {
	if (bicubic > 0) {
		return texture_bicubic(texture_lightmap, vec3(vertex.lightmap, index));
	} else {
		return texture(texture_lightmap, vec3(vertex.lightmap, index));
	}
}

/**
 * @brief
 */
void main(void) {

	mat3 tbn = mat3(normalize(vertex.tangent), normalize(vertex.bitangent), normalize(vertex.normal));

	vec3 view_dir = normalize(-vertex.position);

	if ((stage.flags & STAGE_MATERIAL) == STAGE_MATERIAL) {

		vec4 diffusemap = texture(texture_material, vec3(vertex.diffusemap, 0));
		diffusemap *= vertex.color;

		if (diffusemap.a < material.alpha_test) {
			discard;
		}

		vec3 normalmap = texture(texture_material, vec3(vertex.diffusemap, 1)).xyz;
		vec4 glossmap = texture(texture_material, vec3(vertex.diffusemap, 2));

		vec3 ambient = sample_lightmap(0).rgb;
		vec3 diffuse = sample_lightmap(1).rgb;
		vec3 direction = sample_lightmap(2).xyz;
		vec3 caustics = sample_lightmap(3).rgb;
		vec3 stainmap = sample_lightmap(4).rgb;

		vec3 roughness = vec3(material.roughness, material.roughness, 1.0);
		vec3 hardness = glossmap.rgb * material.hardness;
		float specularity = glossmap.a * pow(material.specularity + 1.0, 3.0);

		vec3 normal = normalize(tbn * (normalize(normalmap * 2.0 - 1.0) * roughness));
		direction = normalize(tbn * normalize(direction * 2.0 - 1.0));

		if (entity > 0) {
			ambient = mix(ambient, texture(texture_lightgrid_ambient, vertex.lightgrid).rgb, .666);
			diffuse = mix(diffuse, texture(texture_lightgrid_diffuse, vertex.lightgrid).rgb, .666);

			vec3 dir = texture(texture_lightgrid_direction, vertex.lightgrid).xyz;
			dir = normalize((view * vec4(dir * 2.0 - 1.0, 0.0)).xyz);
			direction = normalize(mix(direction, dir, .666));

			caustics = texture(texture_lightgrid_caustics, vertex.lightgrid).rgb;
		}

		ambient *= max(0.0, dot(vertex.normal, normal));
		diffuse *= max(0.0, dot(direction, normal));

		vec3 specular = diffuse * hardness * pow(max(0.0, dot(reflect(-direction, normal), view_dir)), specularity);

		caustic_light(vertex.model, caustics, diffuse);

		dynamic_light(vertex.position, normal, specularity, diffuse, specular);

		out_color = diffusemap;

		out_color.rgb = clamp(out_color.rgb * (ambient + diffuse) * stainmap * modulate, 0.0, 1.0);
		out_color.rgb = clamp(out_color.rgb + specular * stainmap * modulate, 0.0, 1.0);

		lightgrid_fog(out_color, texture_lightgrid_fog, vertex.position, vertex.lightgrid);
		global_fog(out_color, vertex.position);

		out_bloom.rgb = clamp(out_color.rgb * material.bloom - 1.0, 0.0, 1.0);
		out_bloom.a = out_color.a;

		if (lightmaps == 1) {
			out_color.rgb = sample_lightmap(0).rgb + sample_lightmap(1).rgb;
		} else {
			out_color = postprocess(out_color);
		}

	} else {

		vec2 st = vertex.diffusemap;

		if ((stage.flags & STAGE_WARP) == STAGE_WARP) {
			st += texture(texture_warp, st + vec2(ticks * stage.warp.x * 0.000125)).xy * stage.warp.y;
		}

		vec4 effect = texture(texture_stage, st);

		effect *= vertex.color;

		if ((stage.flags & STAGE_LIGHTMAP) == STAGE_LIGHTMAP) {
			vec3 ambient = sample_lightmap(0).rgb;
			vec3 diffuse = sample_lightmap(1).rgb;

			effect.rgb *= (ambient + diffuse) * modulate;
		}

		if ((stage.flags & STAGE_FOG) == STAGE_FOG) {
			lightgrid_fog(effect, texture_lightgrid_fog, vertex.position, vertex.lightgrid);
			global_fog(effect, vertex.position);
		}

		out_color = effect;

		// postprocessing
		out_color = postprocess(out_color);
	}

	// debugging

	#if 0
	// draw lightgrid texel borders
	vec4 raster = lightgrid_raster(vertex.lightgrid.xyz, length(vertex.position));
	out_color.rgb = mix(out_color.rgb, raster.rgb, raster.a * 0.5);
	#endif

	#if 0
	// draw vertex tangents
	out_color.rgb = (vertex.tangent.xyz + 1) * 0.5;
	#endif

	#if 0
	// draw flat lightmaps
	out_color.rgb = sample_lightmap(0).rgb + sample_lightmap(1).rgb;
	#endif
}
