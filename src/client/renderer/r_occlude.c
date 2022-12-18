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

/**
 * @brief The occlusion queries.
 */
static struct {
	GLuint query_objects[MAX_OCCLUSION_QUERIES];

	GLuint vertex_array;
	GLuint vertex_buffer;
	GLuint elements_buffer;
} r_occlusion_queries;

/**
 * @brief
 */
_Bool R_OccludeBox(const r_view_t *view, const box3_t bounds) {

	if (!r_occlude->integer) {
		return false;
	}

	if (view->type == VIEW_PLAYER_MODEL) {
		return false;
	}

	const r_occlusion_query_t *q = view->occlusion_queries;
	for (int32_t i = 0; i < view->num_occlusion_queries; i++, q++) {

		if (Box3_Contains(q->bounds, bounds) && q->status == QUERY_OCCLUDED) {
			return true;
		}
	}

	return false;
}

/**
 * @brief
 */
_Bool R_OccludeSphere(const r_view_t *view, const vec3_t origin, float radius) {
	return R_OccludeBox(view, Box3_FromCenterRadius(origin, radius));
}

/**
 * @brief
 */
void R_AddOcclusionQuery(r_view_t *view, const box3_t bounds) {

	if (view->num_occlusion_queries == MAX_OCCLUSION_QUERIES) {
		Com_Warn("MAX_OCCLUSION_QUERIES\n");
		return;
	}

	if (!r_occlude->integer) {
		return;
	}

	if (R_CullBox(view, bounds)) {
		return;
	}

	r_occlusion_query_t *q = &view->occlusion_queries[view->num_occlusion_queries];
	q->name = r_occlusion_queries.query_objects[view->num_occlusion_queries];
	q->bounds = Box3_Expand(bounds, 1.f);
	q->status = QUERY_INVALID;
	Box3_ToPoints(q->bounds, q->vertexes);

	view->num_occlusion_queries++;
}

/**
 * @brief Draws any pending occlusion queries in the specified view.
 * @remarks This is called multiple times per frame, as some queries are added late.
 */
void R_DrawOcclusionQueries(r_view_t *view) {

	if (!r_occlude->integer) {
		return;
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask(GL_FALSE);

	glUseProgram(r_depth_pass_program.name);

	glBindVertexArray(r_occlusion_queries.vertex_array);
	glEnableVertexAttribArray(r_depth_pass_program.in_position);

	glBindBuffer(GL_ARRAY_BUFFER, r_occlusion_queries.vertex_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_occlusion_queries.elements_buffer);

	r_occlusion_query_t *q = view->occlusion_queries;
	for (int32_t i = 0; i < view->num_occlusion_queries; i++, q++) {

		if (q->status == QUERY_PENDING) {
			continue;
		}

		if (Box3_ContainsPoint(Box3_Expand(q->bounds, 1.f), view->origin)) {
			q->status = QUERY_VISIBLE;
			continue;
		}

		if (R_CullBox(view, q->bounds)) {
			q->status = QUERY_CULLED;
			continue;
		}

		q->status = QUERY_PENDING;

		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(q->vertexes), q->vertexes);

		glBeginQuery(GL_ANY_SAMPLES_PASSED, q->name);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (GLvoid *) 0);
		glEndQuery(GL_ANY_SAMPLES_PASSED);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	glUseProgram(0);

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	R_GetError(NULL);
}

/**
 * @brief
 */
void R_UpdateOcclusionQueries(r_view_t *view) {

	if (!r_occlude->integer) {
		return;
	}

	r_occlusion_query_t *q = view->occlusion_queries;
	for (int32_t i = 0; i < view->num_occlusion_queries; i++, q++) {

		if (q->status == QUERY_PENDING) {

			GLint available;
			glGetQueryObjectiv(q->name, GL_QUERY_RESULT_AVAILABLE, &available);

			if (available || r_occlude->integer == 2) {

				GLint result;
				glGetQueryObjectiv(q->name, GL_QUERY_RESULT, &result);

				if (result) {
					q->status = QUERY_VISIBLE;
				} else {
					q->status = QUERY_OCCLUDED;
				}
			}
		}

		switch (q->status) {
			case QUERY_VISIBLE:
				r_stats.count_occlusion_queries_visible++;
				break;
			case QUERY_OCCLUDED:
				r_stats.count_occlusion_queries_occluded++;
				break;
			case QUERY_PENDING:
				r_stats.count_occlusion_queries_pending++;
				break;
			default:
				break;
		}
	}
}

/**
 * @brief
 */
void R_InitOcclusionQueries(void) {

	memset(&r_occlusion_queries, 0, sizeof(r_occlusion_queries));

	glGenVertexArrays(1, &r_occlusion_queries.vertex_array);
	glBindVertexArray(r_occlusion_queries.vertex_array);

	glGenBuffers(1, &r_occlusion_queries.vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, r_occlusion_queries.vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3_t[8]), NULL, GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3_t), (void *) 0);

	glGenBuffers(1, &r_occlusion_queries.elements_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_occlusion_queries.elements_buffer);

	const GLuint elements[] = {
		// bottom
		0, 1, 3, 0, 3, 2,
		// top
		6, 7, 4, 7, 5, 4,
		// front
		4, 5, 0, 5, 1, 0,
		// back
		7, 6, 3, 6, 2, 3,
		// left
		6, 4, 2, 4, 0, 2,
		// right
		5, 7, 1, 7, 3, 1,
	};

	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glGenQueries(MAX_OCCLUSION_QUERIES, r_occlusion_queries.query_objects);

	R_GetError(NULL);
}

/**
 * @brief
 */
void R_ShutdownOcclusionQueries(void) {

	glDeleteQueries(MAX_OCCLUSION_QUERIES, r_occlusion_queries.query_objects);

	glDeleteVertexArrays(1, &r_occlusion_queries.vertex_array);

	glDeleteBuffers(1, &r_occlusion_queries.vertex_buffer);
	glDeleteBuffers(1, &r_occlusion_queries.elements_buffer);

	R_GetError(NULL);
}
