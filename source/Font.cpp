/* Font.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Font.h"

#include "Color.h"
#include "Files.h"
#include "ImageBuffer.h"
#include "Screen.h"

#include <algorithm>
#include <cmath>

using namespace std;

namespace {
	bool showUnderlines = false;
	const int TOTAL_TAB_STOPS = 8;
	
	const Font::Layout defaultParams;
}



Font::Font()
	: shader(), vao(0), vbo(0), scaleI(0), centerI(0), sizeI(0), colorI(0),
	screenRawWidth(0), screenRawHeight(0), screenZoom(100), cr(nullptr),
	fontDescName(), refDescName(), context(nullptr), layout(nullptr), lang(nullptr),
	pixelSize(0), fontRawHeight(0), surfaceWidth(256), surfaceHeight(64), underlineThickness(1),
	cache()
{
	lang = pango_language_from_string("en");
	SetUpShader();
	UpdateSurfaceSize();
	
	cache.SetUpdateInterval(3600);
}



Font::~Font()
{
	if(cr)
		cairo_destroy(cr);
	if(layout)
		g_object_unref(layout);
}



void Font::SetFontDescription(const std::string &desc)
{
	fontDescName = desc;
	UpdateFontDesc();
}




void Font::SetLayoutReference(const std::string &desc)
{
	refDescName = desc;
	UpdateFontDesc();
}



void Font::SetPixelSize(int size)
{
	pixelSize = size;
	UpdateFontDesc();
}



void Font::SetLanguage(const std::string &langCode)
{
	lang = pango_language_from_string(langCode.c_str());
	UpdateFontDesc();
}



void Font::Draw(const std::string &str, const Point &point, const Color &color,
	const Layout *params) const
{
	DrawAliased(str, ViewFromRaw(round(RawFromView(point.X()))),
		ViewFromRaw(round(RawFromView(point.Y()))), color, params);
}



void Font::DrawAliased(const std::string &str, double x, double y, const Color &color,
	const Layout *params) const
{
	if(str.empty())
		return;
	
	if(Screen::Zoom() != screenZoom)
	{
		screenZoom = Screen::Zoom();
		UpdateFontDesc();
	}
	
	const RenderedText &text = Render(str, params);
	if(!text.texture)
		return;
	
	glUseProgram(shader.Object());
	glBindVertexArray(vao);
	
	// Update the texture.
	glBindTexture(GL_TEXTURE_2D, text.texture);
	
	// Update the scale, only if the screen size has changed.
	if(Screen::RawWidth() != screenRawWidth || Screen::RawHeight() != screenRawHeight)
	{
		screenRawWidth = Screen::RawWidth();
		screenRawHeight = Screen::RawHeight();
		GLfloat scale[2] = {2.f / screenRawWidth, -2.f / screenRawHeight};
		glUniform2fv(scaleI, 1, scale);
	}
	
	// Update the center.
	Point center = Point(RawFromView(x), RawFromView(y)) + text.center;
	glUniform2f(centerI, center.X(), center.Y());
	
	// Update the size.
	glUniform2f(sizeI, text.width, text.height);
	
	// Update the color.
	glUniform4fv(colorI, 1, color.Get());
	
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
	glUseProgram(0);
}



int Font::Height(const std::string &str, const Layout *params) const
{
	if(str.empty())
		return 0;
	
	const RenderedText &text = Render(str, params);
	if(!text.texture)
		return 0;
	return ViewFromRawCeil(text.height);
}



int Font::Width(const std::string &str, const Layout *params) const
{
	return ViewFromRawCeil(RawWidth(str, params));
}



int Font::Height() const
{
	return ViewFromRawCeil(fontRawHeight);
}



void Font::ShowUnderlines(bool show)
{
	showUnderlines = show;
}



void Font::UpdateSurfaceSize() const
{
	if(cr)
		cairo_destroy(cr);
	
	cairo_surface_t *sf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, surfaceWidth, surfaceHeight);
	cr = cairo_create(sf);
	cairo_surface_destroy(sf);
	
	if(layout)
		g_object_unref(layout);
	layout = pango_cairo_create_layout(cr);
	context = pango_layout_get_context(layout);
	
	pango_layout_set_wrap(layout, PANGO_WRAP_WORD);
	
	UpdateFontDesc();
}


void Font::UpdateFontDesc() const
{
	if(pixelSize <= 0)
		return;
	
	// Get font descriptions.
	PangoFontDescription *fontDesc = pango_font_description_from_string(fontDescName.c_str());
	PangoFontDescription *refDesc = pango_font_description_from_string(refDescName.c_str());
	
	// Set the pixel size.
	const int fontSize = RawFromViewFloor(pixelSize) * PANGO_SCALE;
	pango_font_description_set_absolute_size(fontDesc, fontSize);
	pango_font_description_set_absolute_size(refDesc, fontSize);
	
	// Update the context.
	pango_context_set_language(context, lang);
	
	// Update the layout.
	pango_layout_set_font_description(layout, fontDesc);
	
	// Update layout parameters.
	PangoFontMetrics *metrics = pango_context_get_metrics(context, refDesc, lang);
	const int ascent = pango_font_metrics_get_ascent(metrics);
	const int descent = pango_font_metrics_get_descent(metrics);
	underlineThickness = pango_font_metrics_get_underline_thickness(metrics) / PANGO_SCALE;
	fontRawHeight = (ascent + descent + PANGO_SCALE - 1) / PANGO_SCALE;
	
	// Clean up.
	pango_font_metrics_unref(metrics);
	pango_font_description_free(fontDesc);
	pango_font_description_free(refDesc);
	cache.Clear();
	
	// Tab Stop
	space = RawWidth(" ");
	const int tabSize = 4 * space * PANGO_SCALE;
	PangoTabArray *tb = pango_tab_array_new(TOTAL_TAB_STOPS, FALSE);
	for(int i = 0; i < TOTAL_TAB_STOPS; ++i)
		pango_tab_array_set_tab(tb, i, PANGO_TAB_LEFT, i * tabSize);
	pango_layout_set_tabs(layout, tb);
	pango_tab_array_free(tb);
}



// Replaces straight quotation marks with curly ones.
string Font::ReplaceCharacters(const string &str)
{
	string buf;
	buf.reserve(str.length());
	bool isAfterWhitespace = true;
	bool isTag = false;
	for(size_t pos = 0; pos < str.length(); ++pos)
	{
		if(isTag)
		{
			if(str[pos] == '>')
				isTag = false;
			buf.append(1, str[pos]);
		}
		else
		{
			// U+2018 LEFT_SINGLE_QUOTATION_MARK
			// U+2019 RIGHT_SINGLE_QUOTATION_MARK
			// U+201C LEFT_DOUBLE_QUOTATION_MARK
			// U+201D RIGHT_DOUBLE_QUOTATION_MARK
			if(str[pos] == '\'')
				buf.append(isAfterWhitespace ? "\xE2\x80\x98" : "\xE2\x80\x99");
			else if(str[pos] == '"')
				buf.append(isAfterWhitespace ? "\xE2\x80\x9C" : "\xE2\x80\x9D");
			else
				buf.append(1, str[pos]);
			isAfterWhitespace = (str[pos] == ' ');
			isTag = (str[pos] == '<');
		}
	}
	return buf;
}



string Font::RemoveAccelerator(const string &str)
{
	string dest;
	bool afterAccel = false;
	for(char c : str)
	{
		if(c == '_')
		{
			if(afterAccel)
				dest += c;
			afterAccel = !afterAccel;
		}
		else
		{
			dest += c;
			afterAccel = false;
		}
	}
	return dest;
}



// Render the text.
const Font::RenderedText &Font::Render(const string &str, const Layout *params) const
{
	if(!params)
		params = &defaultParams;
	
	// Convert to raw coodinates.
	Layout rawParams(*params);
	if(params->width != numeric_limits<int>::max())
		rawParams.width = RawFromView(params->width);
	if(params->lineHeight != DEFAULT_LINE_HEIGHT)
		rawParams.lineHeight = RawFromViewFloor(params->lineHeight);
	if(params->paragraphBreak != 0)
		rawParams.paragraphBreak = RawFromViewFloor(params->paragraphBreak);
	
	// Return if already cached.
	const CacheKey key(str, rawParams, showUnderlines);
	auto cached = cache.Use(key);
	if(cached.second)
		return *cached.first;
	
	// Truncate
	pango_layout_set_width(layout, rawParams.width * PANGO_SCALE);
	PangoEllipsizeMode ellipsize;
	switch(rawParams.truncate)
	{
	case TRUNC_NONE:
		ellipsize = PANGO_ELLIPSIZE_NONE;
		break;
	case TRUNC_FRONT:
		ellipsize = PANGO_ELLIPSIZE_START;
		break;
	case TRUNC_MIDDLE:
		ellipsize = PANGO_ELLIPSIZE_MIDDLE;
		break;
	case TRUNC_BACK:
		ellipsize = PANGO_ELLIPSIZE_END;
		break;
	default:
		ellipsize = PANGO_ELLIPSIZE_NONE;
	}
	pango_layout_set_ellipsize(layout, ellipsize);
	
	// Align and justification
	PangoAlignment align;
	gboolean justify;
	switch(rawParams.align)
	{
	case LEFT:
		align = PANGO_ALIGN_LEFT;
		justify = FALSE;
		break;
	case CENTER:
		align = PANGO_ALIGN_CENTER;
		justify = FALSE;
		break;
	case RIGHT:
		align = PANGO_ALIGN_RIGHT;
		justify = FALSE;
		break;
	case JUSTIFIED:
		align = PANGO_ALIGN_LEFT;
		justify = TRUE;
		break;
	default:
		align = PANGO_ALIGN_LEFT;
		justify = FALSE;
	}
	pango_layout_set_justify(layout, justify);
	pango_layout_set_alignment(layout, align);
	
	// Replaces straight quotation marks with curly ones.
	const string text = ReplaceCharacters(str);
	
	// Keyboard Accelerator
	char *removeMarkupText;
	const char *rawText;
	vector<char> charBuffer(text.length() + 1);
	PangoAttrList *al = nullptr;
	GError *error = nullptr;
	const char accel = showUnderlines ? '_' : '\0';
	const string &nonAccelText = RemoveAccelerator(text);
	const string &parseText = showUnderlines ? text : nonAccelText;
	if(pango_parse_markup(parseText.c_str(), -1, accel, &al, &removeMarkupText, 0, &error))
		rawText = removeMarkupText;
	else
	{
		if(error->message)
			Files::LogError(error->message);
		rawText = nonAccelText.c_str();
	}
	
	// Set the text and attributes to layout.
	pango_layout_set_text(layout, rawText, -1);
	pango_layout_set_attributes(layout, al);
	pango_attr_list_unref(al);
	
	// Check the image buffer size.
	int textWidth;
	int textHeight;
	pango_layout_get_pixel_size(layout, &textWidth, &textHeight);
	// Pango draws a PANGO_UNDERLINE_LOW at bottom + underlineThickness.
	textHeight += 2 * underlineThickness;
	bool changeRequest = false;
	if(surfaceWidth < textWidth)
	{
		surfaceWidth *= (textWidth / surfaceWidth) + 1;
		changeRequest = true;
	}
	// Height needs some margins.
	int heightMargin128 = 128;
	const bool isDefaultLineHeight = rawParams.lineHeight == DEFAULT_LINE_HEIGHT;
	const bool isDefaultSkip = isDefaultLineHeight && rawParams.paragraphBreak == 0;
	if(!isDefaultSkip)
	{
		const int l = isDefaultLineHeight ? fontRawHeight : rawParams.lineHeight;
		heightMargin128 = (l + rawParams.paragraphBreak) * 128 / fontRawHeight;
	}
	if(surfaceHeight < textHeight * heightMargin128 / 128)
	{
		surfaceHeight *= (textHeight * heightMargin128 / 128 / surfaceHeight) + 1;
		changeRequest = true;
	}
	if(changeRequest)
	{
		UpdateSurfaceSize();
		return Render(str, params);
	}
	
	// Render
	cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
	if(isDefaultSkip)
	{
		cairo_move_to(cr, 0, 0);
		pango_cairo_show_layout(cr, layout);
	}
	else
	{
		// Control line skips and paragraph breaks manually.
		const char *layoutText = pango_layout_get_text(layout);
		PangoLayoutIter *it = pango_layout_get_iter(layout);
		int y0 = pango_layout_iter_get_baseline(it);
		int baselineY = y0 / PANGO_SCALE;
		int sumExtraY = 0;
		cairo_move_to(cr, 0, baselineY);
		pango_cairo_update_layout(cr, layout);
		PangoLayoutLine *line = pango_layout_iter_get_line_readonly(it);
		pango_cairo_show_layout_line(cr, line);
		while(pango_layout_iter_next_line(it))
		{
			const int y1 = pango_layout_iter_get_baseline(it);
			const int index = pango_layout_iter_get_index(it);
			const int diffY = (y1 - y0) / PANGO_SCALE;
			if(layoutText[index] == '\0')
			{
				sumExtraY -= diffY;
				break;
			}
			int add = isDefaultLineHeight ? diffY : rawParams.lineHeight;
			if(layoutText[index-1] == '\n')
				add += rawParams.paragraphBreak;
			baselineY += add;
			sumExtraY += add - diffY;
			cairo_move_to(cr, 0, baselineY);
			pango_cairo_update_layout(cr, layout);
			line = pango_layout_iter_get_line_readonly(it);
			pango_cairo_show_layout_line(cr, line);
			y0 = y1;
		}
		textHeight += sumExtraY + rawParams.paragraphBreak;
		pango_layout_iter_free(it);
	}
	
	// Copy to image buffer and clear the surface.
	cairo_surface_t *sf = cairo_get_target(cr);
	cairo_surface_flush(sf);
	ImageBuffer image;
	image.Allocate(textWidth, textHeight);
	uint32_t *src = reinterpret_cast<uint32_t*>(cairo_image_surface_get_data(sf));
	uint32_t *dest = image.Pixels();
	const int stride = surfaceWidth - textWidth;
	const int maxHeight = min(textHeight, surfaceHeight);
	for(int y = 0; y < maxHeight; ++y)
	{
		for(int x = 0; x < textWidth; ++x)
		{
			*dest = *src;
			*src = 0;
			++dest;
			++src;
		}
		src += stride;
	}
	cairo_surface_mark_dirty(sf);
	
	// Try to reuse an old texture.
	GLuint texture = 0;
	auto recycled = cache.Recycle();
	if(recycled.second)
		texture = recycled.first.texture;
	
	// Record rendered text.
	RenderedText &renderedText = recycled.first;
	renderedText.texture = texture;
	renderedText.width = textWidth;
	renderedText.height = maxHeight;
	renderedText.center.X() = .5 * textWidth;
	renderedText.center.Y() = .5 * maxHeight;
	
	// Upload the image as a texture.
	if(!renderedText.texture)
		glGenTextures(1, &renderedText.texture);
	glBindTexture(GL_TEXTURE_2D, renderedText.texture);
	const auto &cachedText = cache.New(key, move(renderedText));
	
	// Use linear interpolation and no wrapping.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	// Upload the image data.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, // target, mipmap level, internal format,
		textWidth, maxHeight, 0, // width, height, border,
		GL_BGRA, GL_UNSIGNED_BYTE, image.Pixels()); // input format, data type, data.
	
	// Unbind the texture.
	glBindTexture(GL_TEXTURE_2D, 0);
	
	return cachedText;
}



void Font::SetUpShader()
{
	static const char *vertexCode =
		// Parameter: Convert pixel coordinates to GL coordinates (-1 to 1).
		"uniform vec2 scale;\n"
		// Parameter: Position of the top left corner of the texture in pixels.
		"uniform vec2 center;\n"
		// Parameter: Size of the texture in pixels.
		"uniform vec2 size;\n"
		
		// Input: Coordinate from the VBO.
		"in vec2 vert;\n"
		
		// Output: Texture coordinate for the fragment shader.
		"out vec2 texCoord;\n"
		
		"void main() {\n"
		"  gl_Position = vec4((center + vert * size) * scale, 0, 1);\n"
		"  texCoord = vert + vec2(.5, .5);\n"
		"}\n";
	
	static const char *fragmentCode =
		// Parameter: Texture with the text.
		"uniform sampler2D tex;\n"
		// Parameter: Color to apply to the text.
		"uniform vec4 color = vec4(1, 1, 1, 1);\n"
		
		// Input: Texture coordinate from the vertex shader.
		"in vec2 texCoord;\n"
		
		// Output: Color for the screen.
		"out vec4 finalColor;\n"
		
		"void main() {\n"
		"  finalColor = color * texture(tex, texCoord);\n"
		"}\n";
	
	shader = Shader(vertexCode, fragmentCode);
	scaleI = shader.Uniform("scale");
	centerI = shader.Uniform("center");
	sizeI = shader.Uniform("size");
	colorI = shader.Uniform("color");
	
	// The texture always comes from texture unit 0.
	glUseProgram(shader.Object());
	glUniform1i(shader.Uniform("tex"), 0);
	glUseProgram(0);
	
	// Create the VAO and VBO.
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	
	// Triangle strip.
	GLfloat vertexData[] = {
		-.5f, -.5f,
		-.5f,  .5f,
		 .5f, -.5f,
		 .5f,  .5f
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
	
	glEnableVertexAttribArray(shader.Attrib("vert"));
	glVertexAttribPointer(shader.Attrib("vert"), 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);
	
	// Unbind the VBO and VAO.
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	
	// We must update the screen size next time we draw.
	screenRawWidth = 0;
	screenRawHeight = 0;
	screenZoom = 100;
}



int Font::RawWidth(const std::string &str, const Layout *params) const
{
	if(str.empty())
		return 0;
	
	const RenderedText &text = Render(str, params);
	if(!text.texture)
		return 0;
	return text.width;
}



double Font::RawFromView(double xy) const
{
	return xy * screenZoom / 100.0;
}



int Font::RawFromView(int xy) const
{
	return (xy * screenZoom + 50) / 100;
}



int Font::RawFromViewCeil(int xy) const
{
	return (xy * screenZoom + 99) / 100;
}



int Font::RawFromViewFloor(int xy) const
{
	return xy * screenZoom / 100;
}



double Font::ViewFromRaw(double xy) const
{
	return xy * 100.0 / screenZoom;
}



int Font::ViewFromRaw(int xy) const
{
	return (xy * 100 + screenZoom / 2)/ screenZoom;
}



int Font::ViewFromRawCeil(int xy) const
{
	return (xy * 100 + screenZoom - 1) / screenZoom;
}



int Font::ViewFromRawFloor(int xy) const
{
	return (xy * 100 + screenZoom - 1) / screenZoom;
}
