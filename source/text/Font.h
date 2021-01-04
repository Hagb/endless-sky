/* Font.h
Copyright (c) 2014-2020 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef ES_TEXT_FONT_H_
#define ES_TEXT_FONT_H_

#include "../Cache.h"
#include "DisplayText.h"
#include "layout.hpp"
#include "../Point.h"
#include "../Shader.h"

#include "../gl_header.h"

#include <cstddef>
#include <string>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <pango/pangocairo.h>
#pragma GCC diagnostic pop

class Color;
class ImageBuffer;



// Class for drawing text in OpenGL.
// The encoding of the text is utf8.
class Font {
public:
	// Font and laying out settings except the pixel size.
	struct DrawingSettings {
		// A font description is a comma separated list of font families.
		std::string description = "Ubuntu";
		// The language for laying out.
		std::string language = "en";
		// The line height is lineHeightScale times larger than the font height.
		double lineHeightScale = 1.20;
		// The paragraph break is paragraphBreakScale times larger than the font height.
		double paragraphBreakScale = 0.40;
	};
	
public:
	Font();
	~Font();
	Font(const Font &a) = delete;
	Font &operator=(const Font &a) = delete;
	
	// Set the font size by pixel size of the text coordinate.
	// It's a rough estimate of the actual font size.
	void SetPixelSize(int size);
	
	// Set the font and laying out settings except the pixel size.
	void SetDrawingSettings(const DrawingSettings &drawingSettings);
	
	// Draw a text string, subject to the given layout and truncation strategy.
	void Draw(const DisplayText &text, const Point &point, const Color &color) const;
	void DrawAliased(const DisplayText &text, double x, double y, const Color &color) const;
	// Draw the given text string, e.g. post-formatting (or without regard to formatting).
	void Draw(const std::string &str, const Point &point, const Color &color) const;
	void DrawAliased(const std::string &str, double x, double y, const Color &color) const;
	
	// Determine the string's width and height, without considering formatting.
	int Width(const std::string &str) const;
	int Height() const;
	// Get the width and height of the text while accounting for the desired layout and truncation strategy.
	int FormattedWidth(const DisplayText &text) const;
	int FormattedHeight(const DisplayText &text) const;
	Point FormattedBounds(const DisplayText &text) const;
	
	// Get the line height and paragraph break.
	int LineHeight(const Layout &layout = {}) const;
	int ParagraphBreak(const Layout &layout = {}) const;
	
	static void ShowUnderlines(bool show) noexcept;
	
	// Escape markup characters if it causes some errors.
	static std::string EscapeMarkupHasError(const std::string &str);
	
private:
	// Text rendered as a sprite.
	struct RenderedText {
		// Texture with rendered text.
		GLuint texture;
		int width;
		int height;
		// Offset from the floored origin to the center of the sprite.
		Point center;
	};
	
	// A key mapping the text and layout parameters, underline status to RenderedText.
	struct CacheKey {
		DisplayText text;
		bool showUnderline;
		
		CacheKey(const DisplayText &t, bool underline);
		bool operator==(const CacheKey &rhs) const noexcept;
	};
	
	// Hash function of CacheKey.
	struct CacheKeyHash {
		typedef CacheKey argument_type;
		typedef std::size_t result_type;
		result_type operator() (argument_type const &s) const noexcept;
	};
	
	// Function to recycle it for RenderedText.
	class AtRecycleForRenderedText {
	public:
		void operator()(RenderedText &v) const;
	};
	
	
private:
	void UpdateSurfaceSize() const;
	void UpdateFont() const;
	
	static std::string ReplaceCharacters(const std::string &str);
	static std::string RemoveAccelerator(const std::string &str);
	
	void DrawCommon(const DisplayText &text, double x, double y, const Color &color, bool alignToDot) const;
	const RenderedText &Render(const DisplayText &text) const;
	void SetUpShader();
	
	int ViewWidth(const DisplayText &text) const;
	
	// Convert Viewport to/from Text coordinates.
	double ViewFromTextX(double x) const;
	double ViewFromTextY(double y) const;
	int ViewFromTextX(int x) const;
	int ViewFromTextY(int y) const;
	int ViewFromTextCeilX(int x) const;
	int ViewFromTextCeilY(int y) const;
	int ViewFromTextFloorX(int x) const;
	int ViewFromTextFloorY(int y) const;
	double TextFromViewX(double x) const;
	double TextFromViewY(double y) const;
	int TextFromViewX(int x) const;
	int TextFromViewY(int y) const;
	int TextFromViewCeilX(int x) const;
	int TextFromViewCeilY(int y) const;
	int TextFromViewFloorX(int x) const;
	int TextFromViewFloorY(int y) const;
	
	
private:
	Shader shader;
	GLuint vao = 0;
	GLuint vbo = 0;
	
	// Shader parameters.
	GLint scaleI = 0;
	GLint centerI = 0;
	GLint sizeI = 0;
	GLint colorI = 0;
	
	// Screen settings.
	mutable int screenWidth = 1;
	mutable int screenHeight = 1;
	mutable int viewWidth = 1;
	mutable int viewHeight = 1;
	mutable int viewFontHeight = 0;
	mutable unsigned int viewDefaultLineHeight = 0;
	mutable unsigned int viewDefaultParagraphBreak = 0;
	
	// Variables related to the font.
	int pixelSize = 0;
	DrawingSettings drawingSettings;
	mutable int space = 0;
	
	// For rendering.
	mutable cairo_t *cr = nullptr;
	mutable PangoContext *context = nullptr;
	mutable PangoLayout *pangoLayout = nullptr;
	mutable int surfaceWidth = 256;
	mutable int surfaceHeight = 64;
	
	// Cache of rendered text.
	mutable Cache<CacheKey, RenderedText, true, CacheKeyHash, AtRecycleForRenderedText> cache;
};



inline Font::CacheKey::CacheKey(const DisplayText &t, bool underline)
	: text(t), showUnderline(underline)
{
}



inline bool Font::CacheKey::operator==(const CacheKey &rhs) const noexcept
{
	return text == rhs.text && showUnderline == rhs.showUnderline;
}



inline Font::CacheKeyHash::result_type Font::CacheKeyHash::operator() (argument_type const &s) const noexcept
{
	const std::string &text = s.text.GetText();
	const Layout &layout = s.text.GetLayout();
	const result_type h1 = std::hash<std::string>()(text);
	const result_type h2 = std::hash<int>()(layout.width);
	const unsigned int pack = s.showUnderline | (static_cast<unsigned int>(layout.align) << 1)
		| (static_cast<unsigned int>(layout.truncate) << 3)
		| (layout.lineHeight << 5) | (layout.paragraphBreak << 13);
	const result_type h3 = std::hash<unsigned int>()(pack);
	return h1 ^ (h2 << 1) ^ (h3 << 2);
}



inline void Font::AtRecycleForRenderedText::operator()(RenderedText &v) const
{
	if(v.texture)
		glDeleteTextures(1, &v.texture);
}



#endif
