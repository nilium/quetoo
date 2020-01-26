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

#include "r_local.h"

#define MAX_ACTIVE_LIGHTS        10

#define TEXTURE_DIFFUSE           0
#define TEXTURE_NORMALMAP         1
#define TEXTURE_GLOSSMAP          2
#define TEXTURE_LIGHTMAP          3
#define TEXTURE_DELUXEMAP         4
#define TEXTURE_STAINMAP          5

#define TEXTURE_MASK_DIFFUSE     (1 << TEXTURE_DIFFUSE)
#define TEXTURE_MASK_NORMALMAP   (1 << TEXTURE_NORMALMAP)
#define TEXTURE_MASK_GLOSSMAP    (1 << TEXTURE_GLOSSMAP)
#define TEXTURE_MASK_LIGHTMAP    (1 << TEXTURE_LIGHTMAP)
#define TEXTURE_MASK_DELUXEMAP   (1 << TEXTURE_DELUXEMAP)
#define TEXTURE_MASK_STAINMAP    (1 << TEXTURE_STAINMAP)
#define TEXTURE_MASK_ALL          0xff

/**
 * @brief The program.
 */
static struct {
	GLuint name;

	GLint in_position;
	GLint in_normal;
	GLint in_tangent;
	GLint in_bitangent;
	GLint in_diffuse;
	GLint in_lightmap;
	GLint in_color;

	GLint projection;
	GLint model_view;
	GLint normal;

	GLint textures;

	GLint texture_diffuse;
	GLint texture_normalmap;
	GLint texture_glossmap;
	GLint texture_lightmap;

	GLint contents;

	GLint alpha_threshold;

	GLint brightness;
	GLint contrast;
	GLint saturation;
	GLint gamma;
	
	GLint modulate;

	GLint bump;
	GLint parallax;
	GLint hardness;
	GLint specular;

	GLint light_positions[MAX_ACTIVE_LIGHTS];
	GLint light_colors[MAX_ACTIVE_LIGHTS];

	GLint fog_parameters;
	GLint fog_color;

	GLint caustics;
} r_bsp_program;

/**
 * @brief
 */
static void R_DrawBspElements(const r_bsp_draw_elements_t *draw) {

	const r_material_t *material = draw->texinfo->material;

	GLint textures = TEXTURE_MASK_DIFFUSE;

	glActiveTexture(GL_TEXTURE0 + TEXTURE_DIFFUSE);
	glBindTexture(GL_TEXTURE_2D, material->diffuse->texnum);

	if (material->normalmap) {
		textures |= TEXTURE_MASK_NORMALMAP;

		glActiveTexture(GL_TEXTURE0 + TEXTURE_NORMALMAP);
		glBindTexture(GL_TEXTURE_2D, material->normalmap->texnum);
	}

	if (material->glossmap) {
		textures |= TEXTURE_MASK_GLOSSMAP;

		glActiveTexture(GL_TEXTURE0 + TEXTURE_GLOSSMAP);
		glBindTexture(GL_TEXTURE_2D, material->glossmap->texnum);
	}

	if (draw->lightmap) {
		textures |= TEXTURE_MASK_LIGHTMAP;
		textures |= TEXTURE_MASK_DELUXEMAP;
		textures |= TEXTURE_MASK_STAINMAP;

		glActiveTexture(GL_TEXTURE0 + TEXTURE_LIGHTMAP);
		glBindTexture(GL_TEXTURE_2D_ARRAY, draw->lightmap->atlas->texnum);
	}

	switch (r_draw_bsp_lightmaps->integer) {
		case 1:
			textures = TEXTURE_MASK_LIGHTMAP;
			break;
		case 2:
			textures = TEXTURE_MASK_DELUXEMAP;
			break;
	}

	glUniform1i(r_bsp_program.textures, textures);

	glDrawElements(GL_TRIANGLES, draw->num_elements, GL_UNSIGNED_INT, draw->elements);

	r_view.num_bsp_draw_elements++;
}

/**
 * @brief
 */
static void R_DrawBspNode(const r_entity_t *e, const r_bsp_node_t *node) {

	if (node->contents != CONTENTS_NODE) {
		return;
	}

	if (e == NULL) {
		if (node->vis_frame != r_locals.vis_frame) {
			return;
		}

		if (R_CullBox(node->mins, node->maxs)) {
			return;
		}
	}

	const vec_t dist = Cm_DistanceToPlane(r_view.origin, node->plane);

	int32_t side;
	if (dist > SIDE_EPSILON) {
		side = 0;
	} else {
		side = 1;
	}

	R_DrawBspNode(e, node->children[side]);

	if (r_draw_bsp_nodes->value) {
		const vec4_t colors[] = {
			{ 0.8, 0.2, 0.2, 1.0 },
			{ 0.2, 0.8, 0.2, 1.0 },
			{ 0.2, 0.2, 0.8, 1.0 },
			{ 0.8, 0.8, 0.2, 1.0 },
			{ 0.2, 0.8, 0.8, 1.0 },
			{ 0.8, 0.2, 0.8, 1.0 },
			{ 0.8, 0.4, 0.2, 1.0 },
			{ 0.4, 0.8, 0.2, 1.0 },
			{ 0.2, 0.4, 0.8, 1.0 },
		};

		const ptrdiff_t color = node - r_model_state.world->bsp->nodes;

		glVertexAttrib4fv(r_bsp_program.in_color, colors[color % lengthof(colors)]);
	}

	glUniform1i(r_bsp_program.contents, node->contents);

	const r_bsp_draw_elements_t *draw = node->draw_elements;
	for (int32_t i = 0; i < node->num_draw_elements; i++, draw++) {

		if (draw->texinfo->flags & SURF_SKY) {
			continue;
		}

		if (draw->texinfo->flags & SURF_MATERIAL) {
			continue;
		}

		R_DrawBspElements(draw);
	}

	r_view.num_bsp_nodes++;

	R_DrawBspNode(e, node->children[!side]);
}

/**
 * @brief
 */
static void R_DrawBspEntity(const r_entity_t *e) {

	if (R_CullBspInlineModel(e)) {
		return;
	}

	matrix4x4_t transform;
	Matrix4x4_CreateFromEntity(&transform, e->origin, e->angles, e->scale);

	matrix4x4_t model_view;
	Matrix4x4_Concat(&model_view, &r_locals.model_view, &transform);

	matrix4x4_t normal;
	Matrix4x4_Invert_Full(&normal, &model_view);
	Matrix4x4_Transpose(&normal, &normal);

	glUniformMatrix4fv(r_bsp_program.model_view, 1, GL_FALSE, (GLfloat *) model_view.m);
	glUniformMatrix4fv(r_bsp_program.normal, 1, GL_FALSE, (GLfloat *) normal.m);

	R_DrawBspNode(e, e->model->bsp_inline->head_node);
}

/**
 * @brief
 */
void R_DrawWorld(void) {

	glEnable(GL_DEPTH_TEST);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	if (r_draw_wireframe->value) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	glUseProgram(r_bsp_program.name);

	glUniformMatrix4fv(r_bsp_program.projection, 1, GL_FALSE, (GLfloat *) r_locals.projection3D.m);
	glUniformMatrix4fv(r_bsp_program.model_view, 1, GL_FALSE, (GLfloat *) r_locals.model_view.m);
	glUniformMatrix4fv(r_bsp_program.normal, 1, GL_FALSE, (GLfloat *) r_locals.inverse_transpose_model_view.m);

	glUniform1f(r_bsp_program.alpha_threshold, 0.f);

	glUniform1f(r_bsp_program.brightness, r_brightness->value);
	glUniform1f(r_bsp_program.contrast, r_contrast->value);
	glUniform1f(r_bsp_program.saturation, r_saturation->value);
	glUniform1f(r_bsp_program.gamma, r_gamma->value);
	glUniform1f(r_bsp_program.modulate, r_modulate->value);

	const r_bsp_model_t *bsp = R_WorldModel()->bsp;

	glBindVertexArray(bsp->vertex_array);

	glBindBuffer(GL_ARRAY_BUFFER, bsp->vertex_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bsp->elements_buffer);

	glEnableVertexAttribArray(r_bsp_program.in_position);
	glEnableVertexAttribArray(r_bsp_program.in_normal);
	glEnableVertexAttribArray(r_bsp_program.in_tangent);
	glEnableVertexAttribArray(r_bsp_program.in_bitangent);
	glEnableVertexAttribArray(r_bsp_program.in_diffuse);
	glEnableVertexAttribArray(r_bsp_program.in_lightmap);
	glEnableVertexAttribArray(r_bsp_program.in_color);

	if (r_draw_bsp_nodes->value) {
		glDisableVertexAttribArray(r_bsp_program.in_color);
	}

	R_DrawBspNode(NULL, bsp->nodes);

	const r_entity_t *e = r_view.entities;
	for (int32_t i = 0; i < r_view.num_entities; i++, e++) {
		if (e->model && e->model->type == MOD_BSP_INLINE) {
			R_DrawBspEntity(e);
		}
	}

	glActiveTexture(GL_TEXTURE0);

	if (r_draw_wireframe->value) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	glCullFace(GL_FRONT);
	glDisable(GL_CULL_FACE);

	glDisable(GL_DEPTH_TEST);

	R_GetError(NULL);
}

/**
 * @brief
 */
void R_InitBspProgram(void) {

	memset(&r_bsp_program, 0, sizeof(r_bsp_program));

	r_bsp_program.name = R_LoadProgram(
			&MakeShaderDescriptor(GL_VERTEX_SHADER, "bsp_vs.glsl"),
			&MakeShaderDescriptor(GL_FRAGMENT_SHADER, "color_filter.glsl", "bsp_fs.glsl"),
			NULL);

	glUseProgram(r_bsp_program.name);

	r_bsp_program.in_position = glGetAttribLocation(r_bsp_program.name, "in_position");
	r_bsp_program.in_normal = glGetAttribLocation(r_bsp_program.name, "in_normal");
	r_bsp_program.in_tangent = glGetAttribLocation(r_bsp_program.name, "in_tangent");
	r_bsp_program.in_bitangent = glGetAttribLocation(r_bsp_program.name, "in_bitangent");
	r_bsp_program.in_diffuse = glGetAttribLocation(r_bsp_program.name, "in_diffuse");
	r_bsp_program.in_lightmap = glGetAttribLocation(r_bsp_program.name, "in_lightmap");
	r_bsp_program.in_color = glGetAttribLocation(r_bsp_program.name, "in_color");

	r_bsp_program.projection = glGetUniformLocation(r_bsp_program.name, "projection");
	r_bsp_program.model_view = glGetUniformLocation(r_bsp_program.name, "model_view");
	r_bsp_program.normal = glGetUniformLocation(r_bsp_program.name, "normal");

	r_bsp_program.textures = glGetUniformLocation(r_bsp_program.name, "textures");
	r_bsp_program.texture_diffuse = glGetUniformLocation(r_bsp_program.name, "texture_diffuse");
	r_bsp_program.texture_normalmap = glGetUniformLocation(r_bsp_program.name, "texture_normalmap");
	r_bsp_program.texture_glossmap = glGetUniformLocation(r_bsp_program.name, "texture_glossmap");
	r_bsp_program.texture_lightmap = glGetUniformLocation(r_bsp_program.name, "texture_lightmap");

	r_bsp_program.contents = glGetUniformLocation(r_bsp_program.contents, "contents");

	r_bsp_program.alpha_threshold = glGetUniformLocation(r_bsp_program.name, "alpha_threshold");

	r_bsp_program.brightness = glGetUniformLocation(r_bsp_program.name, "brightness");
	r_bsp_program.contrast = glGetUniformLocation(r_bsp_program.name, "contrast");
	r_bsp_program.saturation = glGetUniformLocation(r_bsp_program.name, "saturation");
	r_bsp_program.gamma = glGetUniformLocation(r_bsp_program.name, "gamma");
	r_bsp_program.modulate = glGetUniformLocation(r_bsp_program.name, "modulate");

	r_bsp_program.bump = glGetUniformLocation(r_bsp_program.name, "bump");
	r_bsp_program.parallax = glGetUniformLocation(r_bsp_program.name, "parallax");
	r_bsp_program.hardness = glGetUniformLocation(r_bsp_program.name, "hardness");
	r_bsp_program.specular = glGetUniformLocation(r_bsp_program.name, "specular");

	for (size_t i = 0; i < lengthof(r_bsp_program.light_positions); i++) {
		r_bsp_program.light_positions[i] = glGetUniformLocation(r_bsp_program.name, va("light_positions[%zd]", i));
		r_bsp_program.light_colors[i] = glGetUniformLocation(r_bsp_program.name, va("light_colors[%zd]", i));
	}

	r_bsp_program.fog_parameters = glGetUniformLocation(r_bsp_program.name, "fog_parameters");
	r_bsp_program.fog_color = glGetUniformLocation(r_bsp_program.name, "fog_color");

	r_bsp_program.caustics = glGetUniformLocation(r_bsp_program.name, "caustics");

	glUniform1i(r_bsp_program.texture_diffuse, TEXTURE_DIFFUSE);
	glUniform1i(r_bsp_program.texture_normalmap, TEXTURE_NORMALMAP);
	glUniform1i(r_bsp_program.texture_glossmap, TEXTURE_GLOSSMAP);
	glUniform1i(r_bsp_program.texture_lightmap, TEXTURE_LIGHTMAP);

	R_GetError(NULL);
}

/**
 * @brief
 */
void R_ShutdownBspProgram(void) {

	glDeleteProgram(r_bsp_program.name);

	r_bsp_program.name = 0;
}
