/* -*- mode:C++; indent-tabs-mode:nil; -*- */
/* MIT License -- MyThOS: The Many-Threads Operating System
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Copyright 2020 Philipp Gypser and contributors, BTU Cottbus-Senftenberg
 */
#pragma once

namespace mythos{

class CgaAttr {
private:
  enum AttrMaskAndShifts {
		FG_MASK = 0xf,
		BG_MASK = 0x70,
		BLINK_MASK = 0x80,
		BG_SHIFT = 4,
		BLINK_SHIFT = 7
	};


public:
	enum Color {
		BLACK=0,
		BLUE=1,
		GREEN=2,
		CYAN=3,
		RED=4,
		MAGENTA=5,
		BROWN=6,
		LIGHT_GRAY=7,
		GRAY=8,
		LIGHT_BLUE=9,
		LIGHT_GREEN=10,
		LIGHT_CYAN=11,
		LIGHT_RED=12,
		LIGHT_MAGENTA=13,
		YELLOW=14,
		WHITE=15
	};

	CgaAttr(Color fg=WHITE, Color bg=BLACK, bool blink=false)
	{
		attr = (fg & FG_MASK) | ((bg << BG_SHIFT) & BG_MASK);
		if(blink){
			attr |= BLINK_MASK;
		}
	}

	void setForeground(Color col)
	{
		attr = (attr & ~(FG_MASK)) | (col & FG_MASK);
	}

	void setBackground(Color col)
	{
		attr = (attr & ~(BG_MASK)) | ((col << BG_SHIFT) & BG_MASK);
	}

	void setBlinkState(bool blink)
	{
		if(blink){
			attr |= BLINK_MASK;
		}else{
			attr &= ~BLINK_MASK;
		}
	}

	void setAttr(CgaAttr attr)
	{
		this->attr = attr.attr;
	}

	Color getForeground()
	{
		return (Color)(attr & FG_MASK);
	}

	Color getBackground()
	{
		return (Color)((attr & BG_MASK) >> BG_SHIFT);
	}

	bool getBlinkState()
	{
		return (Color)(attr >> BLINK_SHIFT);
	}

private:

	char attr;

};


} // namespace mythos
