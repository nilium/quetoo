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

#include "cl_local.h"

/**
 * @brief Handles mouse button events by dispatching them as key events.
 */
void Cl_MouseButtonEvent(const SDL_Event *event) {
	SDL_Event e;
	memset(&e, 0, sizeof(e));

	e.type = event->type == SDL_MOUSEBUTTONUP ? SDL_KEYUP : SDL_KEYDOWN;
	e.key.keysym.scancode = SDL_SCANCODE_MOUSE1 + (event->button.button - 1);
	e.key.keysym.sym = SDL_SCANCODE_TO_KEYCODE(e.key.keysym.scancode);

	Cl_KeyEvent(&e);
}

/**
 * @brief
 */
void Cl_MouseWheelEvent(const SDL_Event *event) {

	if (event->wheel.y == 0)
		return;

	switch (cls.key_state.dest) {
		case KEY_UI:
			break;

		case KEY_CONSOLE: {
			const int64_t scroll = cl_console.scroll + event->wheel.y;
			cl_console.scroll = Clamp(scroll, 0, (int64_t) console_state.len);
		}
			break;

		case KEY_GAME: {
			SDL_Event e;
			memset(&e, 0, sizeof(e));

			e.type = SDL_KEYDOWN;
			e.key.keysym.scancode = (SDL_Scancode) (event->wheel.y > 0 ? SDL_SCANCODE_MOUSE5 : SDL_SCANCODE_MOUSE4);
			e.key.keysym.sym = SDL_SCANCODE_TO_KEYCODE(e.key.keysym.scancode);

			Cl_KeyEvent(&e);

			e.type = SDL_KEYUP;

			Cl_KeyEvent(&e);
		}
			break;

		default:
			break;
	}
}

/**
 * @brief
 */
void Cl_MouseMotionEvent(const SDL_Event *event) {

	if (cls.key_state.dest != KEY_GAME)
		return;

	const int32_t mx = event->motion.x;
	const int32_t my = event->motion.y;

	if (m_sensitivity->modified) { // clamp sensitivity
		m_sensitivity->value = Clamp(m_sensitivity->value, 0.1, 20.0);
		m_sensitivity->modified = false;
	}

	if (m_interpolate->value) { // interpolate movements
		cls.mouse_state.x = (mx + cls.mouse_state.old_x) * 0.5;
		cls.mouse_state.y = (my + cls.mouse_state.old_y) * 0.5;
	} else {
		cls.mouse_state.x = mx;
		cls.mouse_state.y = my;
	}

	cls.mouse_state.old_x = mx;
	cls.mouse_state.old_y = my;

	const r_pixel_t cx = r_context.window_width * 0.5;
	const r_pixel_t cy = r_context.window_height * 0.5;

	if (cls.state == CL_ACTIVE) {

		cls.mouse_state.x -= cx; // first normalize to center
		cls.mouse_state.y -= cy;

		cls.mouse_state.x *= m_sensitivity->value; // then amplify
		cls.mouse_state.y *= m_sensitivity->value;

		if (m_invert->value) // and finally invert
			cls.mouse_state.y = -cls.mouse_state.y;

		// add horizontal and vertical movement
		cl.angles[YAW] -= m_yaw->value * cls.mouse_state.x;
		cl.angles[PITCH] += m_pitch->value * cls.mouse_state.y;
	}

	SDL_WarpMouseInWindow(r_context.window, cx, cy);
}
