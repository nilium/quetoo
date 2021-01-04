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

#include "color.h"
#include "swap.h"

/**
 * @brief
 */
color_t Color3b(byte r, byte g, byte b) {
	return Color4b(r, g, b, 255);
}

/**
 * @brief
 */
color_t Color3bv(uint32_t rgb) {
	return Color4bv(rgb | 0xff000000);
}

/**
 * @brief
 */
color_t Color3f(float r, float g, float b) {
	return Color4f(r, g, b, 1.f);
}

/**
 * @brief
 */
color_t Color3fv(const vec3_t rgb) {
	return Color3f(rgb.x, rgb.y, rgb.z);
}

/**
 * @brief
 */
color_t Color4b(byte r, byte g, byte b, byte a) {
	return (color_t) {
		.r = r / 255.f,
		.g = g / 255.f,
		.b = b / 255.f,
		.a = a / 255.f
	};
}

/**
 * @brief
 */
color_t Color4bv(uint32_t rgba) {
	const color32_t c = {
		.rgba = LittleLong(rgba)
	};

	return Color4b(c.r, c.g, c.b, c.a);
}

/**
 * @brief
 */
color_t Color4f(float r, float g, float b, float a) {

	const float max = Maxf(r, Maxf(g, b));
	if (max > 1.f) {
		const float inverse = 1.f / max;
		r *= inverse;
		g *= inverse;
		b *= inverse;
	}

	return (color_t) {
		.r = Clampf(r, 0.f, 1.f),
		.g = Clampf(g, 0.f, 1.f),
		.b = Clampf(b, 0.f, 1.f),
		.a = Clampf(a, 0.f, 1.f)
	};
}

/**
 * @brief
 */
color_t Color4fv(const vec4_t rgba) {
	return Color4f(rgba.x, rgba.y, rgba.z, rgba.w);
}

/**
 * @brief
 */
color_t ColorHSV(float hue, float saturation, float value) {

	value = CLAMP(value, 0.f, 1.f);

    if (saturation <= 0.0f) {
		return Color3f(value, value, value);
    }

	saturation = MAX(saturation, 1.f);

	hue = ClampEuler(hue) / 60.f;
	color_t color = { .a = 1.f };

	const float h = fabsf(hue);
	const int i = (int) h;
	const float f = h - i;
	const float p = value * (1.0f - saturation);
	const float q = value * (1.0f - (saturation * f));
	const float t = value * (1.0f - (saturation * (1.0f - f)));

	switch (i) {
		case 0:
			color.r = value;
			color.g = t;
			color.b = p;
			break;
		case 1:
			color.r = q;
			color.g = value;
			color.b = p;
			break;
		case 2:
			color.r = p;
			color.g = value;
			color.b = t;
			break;
		case 3:
			color.r = p;
			color.g = q;
			color.b = value;
			break;
		case 4:
			color.r = t;
			color.g = p;
			color.b = value;
			break;
		default:
			color.r = value;
			color.g = p;
			color.b = q;
			break;
	}

	return color;
}

/**
 * @brief
 */
color_t ColorHSVA(float hue, float saturation, float value, float alpha) {
	color_t color = ColorHSV(hue, saturation, value);
	color.a = CLAMP(alpha, 0.f, 1.f);
	return color;
}

/**
 * @brief
 */
color_t Color_Add(const color_t a, const color_t b) {
	return Color4fv(Vec4_Add(Color_Vec4(a), Color_Vec4(b)));
}

/**
 * @brief
 */
color_t Color_Subtract(const color_t a, const color_t b) {
	return Color4fv(Vec4_Subtract(Color_Vec4(a), Color_Vec4(b)));
}

/**
 * @brief
 */
color_t Color_Multiply(const color_t a, const color_t b) {
	return Color4fv(Vec4_Multiply(Color_Vec4(a), Color_Vec4(b)));
}

/**
 * @brief
 */
color_t Color_Scale(const color_t a, const float b) {
	return Color4fv(Vec4_Scale(Color_Vec4(a), b));
}

/**
 * @brief
 */
color_t Color_Mix(const color_t a, const color_t b, float mix) {
	return Color4fv(Vec4_Mix(Color_Vec4(a), Color_Vec4(b), mix));
}

/**
 * @brief Attempt to convert a big endian hexadecimal value to its string representation.
 */
_Bool Color_Parse(const char *s, color_t *color) {

	const size_t length = strlen(s);
	if (length != 6 && length != 8) {
		return false;
	}

	char buffer[9];
	g_strlcpy(buffer, s, sizeof(buffer));

	if (length == 6) {
		g_strlcat(buffer, "ff", sizeof(buffer));
	}

	uint32_t rgba;
	if (sscanf(buffer, "%x", &rgba) != 1) {
		return false;
	}

	*color = Color4bv(BigLong(rgba));
	return true;
}

/**
 * @brief
 */
const char *Color_Unparse(const color_t color) {
	const color32_t c = Color_Color32(color);

	static char buffer[12];
	g_snprintf(buffer, sizeof(buffer), "%02x%02x%02x%02x", c.r, c.g, c.b, c.a);

	return buffer;
}

/**
 * @brief
 */
vec3_t Color_Vec3(const color_t color) {
	return Vec3(color.r, color.g, color.b);
}

/**
 * @brief
 */
vec4_t Color_Vec4(const color_t color) {
	return Vec4(color.r, color.g, color.b, color.a);
}

/**
 * @brief
 */
color32_t Color_Color32(const color_t color) {
	return Color32(color.r * 255.f,
				   color.g * 255.f,
				   color.b * 255.f,
				   color.a * 255.f);
}

/**
 * @brief
 */
color32_t Color32(byte r, byte g, byte b, byte a) {
	return (color32_t) {
		.r = r,
		.b = b,
		.g = g,
		.a = a
	};
}

/**
 * @brief
 */
color_t Color32_Color(const color32_t c) {
	return Color4bv(c.rgba);
}