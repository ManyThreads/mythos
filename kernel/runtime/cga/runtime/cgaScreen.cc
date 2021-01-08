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

#include "runtime/cgaScreen.hh"

mythos::CgaScreen::CgaScreen(uintptr_t vaddr)
  : index(INDEX_PORT)
  , data(DATA_PORT)
  , screen(reinterpret_cast<CgaChar*>(vaddr))
{
  //mapMemory();
  clear();
  setCursor(0,0);
}

mythos::CgaScreen::CgaScreen()
  : index(INDEX_PORT)
  , data(DATA_PORT)
  , screen(reinterpret_cast<CgaChar*>(VIDEO_RAM))
{
  //mapMemory();
  clear();
  setCursor(0,0);
}

mythos::CgaScreen::CgaScreen(CgaAttr attr)
  : attr(attr)
  , index(INDEX_PORT)
  , data(DATA_PORT)
  , screen(reinterpret_cast<CgaChar*>(VIDEO_RAM))
{
  //mapMemory();
  clear();
  setCursor(0,0);
}

void mythos::CgaScreen::clear()
{
	CgaAttr clearAttr = CgaAttr();
	for (int i = 0;  i < ROWS * COLUMNS; i++) {
		screen[i].setChar(' ');
        screen[i].setAttr(clearAttr);
	}
	setCursor(0,0);
}

void mythos::CgaScreen::setCursor(unsigned row, unsigned column)
{
	unsigned short pos;

	pos = row * COLUMNS + column;

	index.write(HIGH);
	data.write(pos >> 8);

	index.write(LOW);
	data.write(pos & 0xff);

	xpos = column;
	ypos = row;
}

void mythos::CgaScreen::getCursor(unsigned& row, unsigned& column)
{
	unsigned short pos;

	index.write(HIGH);
	pos = data.read() << 8;

	index.write(LOW);
	pos |= data.read() & 0xff;

	row = pos / COLUMNS;
	column = pos % COLUMNS;
}

void mythos::CgaScreen::scroll()
{
	CgaChar* dest = screen;
	CgaChar* src = screen + COLUMNS;

	for(int i=0; i < ((ROWS - 1) * COLUMNS); i++){
		*dest++ = *src++;
	}

	for (int i = 0; i < COLUMNS; i++){
		dest->setChar(' ');
		dest->setAttr(attr);
		dest++;
	}
}

void mythos::CgaScreen::show(char c, const CgaAttr& att)
{

	if (c == '\r')
		xpos = 0;
	else if ( c == '\n' ) {
		ypos++;
		xpos=0;
	} else {
		screen[xpos + ypos * COLUMNS].setChar(c);
		screen[xpos + ypos * COLUMNS].setAttr(att);
		xpos++;
	}

	if (xpos == COLUMNS) {
		xpos = 0;
		ypos++;
	}

	if (ypos == ROWS) {
		scroll();
		ypos--;
	}

	setCursor(ypos, xpos);
}

void mythos::CgaScreen::write(char const* msg, size_t length){
  for(size_t i; i < length; i++){
    if(msg[i] != '\x1b'){
      show(msg[i]);
    }else{
      show('\n');
      return;
    }
  } 
}

