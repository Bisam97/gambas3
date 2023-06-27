/***************************************************************************

  c_key.c

  (c) 2000-2017 Benoît Minisini <benoit.minisini@gambas-basic.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
  MA 02110-1301, USA.

***************************************************************************/

#define __C_KEY_C

#include "c_window.h"
#include "c_key.h"

static SDL_Event *_current = NULL;
static bool _text_event = FALSE;

//-------------------------------------------------------------------------

static void update_event()
{
	if (!_current)
		return;

	_text_event = _current->type == SDL_TEXTINPUT;
}

SDL_Event *KEY_enter_event(SDL_Event *event)
{
	SDL_Event *old = _current;
	_current = event;
	update_event();
	return old;
}

void KEY_leave_event(SDL_Event *event)
{
	_current = event;
	update_event();
}

static bool check_event(void)
{
	if (!_current)
	{
		GB.Error("No keyboard event");
		return TRUE;
	}
	else
		return FALSE;
}

#define CHECK_EVENT() if (check_event()) return

//-------------------------------------------------------------------------

BEGIN_METHOD(Key_get, GB_STRING key)

	char *key = GB.ToZeroString(ARG(key));
	int code = 0;

	if (*key)
	{
		if (!key[1] && ((uchar)key[0] < 127))
		{
			GB.ReturnInteger(key[0]);
			return;
		}
		else
		{
			for (code = 127; code <= 255; code++)
			{
				if (!strcasecmp(SDL_GetKeyName((SDL_Keycode)code), key))
				{
					GB.ReturnInteger(code);
					return;
				}
			}
		}
	}

	GB.ReturnInteger(0);

END_METHOD

BEGIN_PROPERTY(Key_Code)

	CHECK_EVENT();
	GB.ReturnInteger(_text_event ? 0 : _current->key.keysym.sym);

END_PROPERTY

BEGIN_PROPERTY(Key_Shift)

	CHECK_EVENT();
	GB.ReturnBoolean((_text_event ? SDL_GetModState() : _current->key.keysym.mod) & KMOD_SHIFT);

END_PROPERTY

BEGIN_PROPERTY(Key_Control)

	CHECK_EVENT();
	GB.ReturnBoolean((_text_event ? SDL_GetModState() : _current->key.keysym.mod) & KMOD_CTRL);

END_PROPERTY

BEGIN_PROPERTY(Key_Alt)

	CHECK_EVENT();
	GB.ReturnBoolean((_text_event ? SDL_GetModState() : _current->key.keysym.mod) & KMOD_ALT);

END_PROPERTY

BEGIN_PROPERTY(Key_AltGr)

	CHECK_EVENT();
	GB.ReturnBoolean((_text_event ? SDL_GetModState() : _current->key.keysym.mod) & KMOD_MODE);

END_PROPERTY

BEGIN_PROPERTY(Key_Meta)

	CHECK_EVENT();
	GB.ReturnBoolean((_text_event ? SDL_GetModState() : _current->key.keysym.mod) & KMOD_GUI);

END_PROPERTY

BEGIN_PROPERTY(Key_Normal)

	CHECK_EVENT();
	GB.ReturnBoolean(((_text_event ? SDL_GetModState() : _current->key.keysym.mod) & (KMOD_CTRL | KMOD_ALT | KMOD_MODE | KMOD_GUI)) == 0);

END_PROPERTY

BEGIN_PROPERTY(Key_Text)

	CHECK_EVENT();
	if (!_text_event)
		GB.ReturnNull();
	else
		GB.ReturnNewZeroString(_current->text.text);

END_PROPERTY

BEGIN_PROPERTY(Key_Repeat)

	CHECK_EVENT();
	GB.ReturnBoolean(_text_event ? FALSE : _current->key.repeat);

END_PROPERTY


static int convert_keycode_to_scancode(int code)
{
	if (code & SDLK_SCANCODE_MASK)
		return code & ~SDLK_SCANCODE_MASK;

	switch(code)
	{
		case SDLK_RETURN: return SDL_SCANCODE_RETURN;
		case SDLK_ESCAPE: return SDL_SCANCODE_ESCAPE;
		case SDLK_BACKSPACE: return SDL_SCANCODE_BACKSPACE;
		case SDLK_TAB: return SDL_SCANCODE_TAB;
		case SDLK_SPACE: return SDL_SCANCODE_SPACE;
		case SDLK_COMMA: return SDL_SCANCODE_COMMA;
		case SDLK_MINUS: return SDL_SCANCODE_MINUS;
		case SDLK_PERIOD: return SDL_SCANCODE_PERIOD;
		case SDLK_SLASH: return SDL_SCANCODE_SLASH;
		case SDLK_0: return SDL_SCANCODE_0;
		case SDLK_1: return SDL_SCANCODE_1;
		case SDLK_2: return SDL_SCANCODE_2;
		case SDLK_3: return SDL_SCANCODE_3;
		case SDLK_4: return SDL_SCANCODE_4;
		case SDLK_5: return SDL_SCANCODE_5;
		case SDLK_6: return SDL_SCANCODE_6;
		case SDLK_7: return SDL_SCANCODE_7;
		case SDLK_8: return SDL_SCANCODE_8;
		case SDLK_9: return SDL_SCANCODE_9;
		case SDLK_SEMICOLON: return SDL_SCANCODE_SEMICOLON;
		case SDLK_EQUALS: return SDL_SCANCODE_EQUALS;
		case SDLK_LEFTBRACKET: return SDL_SCANCODE_LEFTBRACKET;
		case SDLK_BACKSLASH: return SDL_SCANCODE_BACKSLASH;
		case SDLK_RIGHTBRACKET: return SDL_SCANCODE_RIGHTBRACKET;
		case SDLK_a: return SDL_SCANCODE_A;
		case SDLK_b: return SDL_SCANCODE_B;
		case SDLK_c: return SDL_SCANCODE_C;
		case SDLK_d: return SDL_SCANCODE_D;
		case SDLK_e: return SDL_SCANCODE_E;
		case SDLK_f: return SDL_SCANCODE_F;
		case SDLK_g: return SDL_SCANCODE_G;
		case SDLK_h: return SDL_SCANCODE_H;
		case SDLK_i: return SDL_SCANCODE_I;
		case SDLK_j: return SDL_SCANCODE_J;
		case SDLK_k: return SDL_SCANCODE_K;
		case SDLK_l: return SDL_SCANCODE_L;
		case SDLK_m: return SDL_SCANCODE_M;
		case SDLK_n: return SDL_SCANCODE_N;
		case SDLK_o: return SDL_SCANCODE_O;
		case SDLK_p: return SDL_SCANCODE_P;
		case SDLK_q: return SDL_SCANCODE_Q;
		case SDLK_r: return SDL_SCANCODE_R;
		case SDLK_s: return SDL_SCANCODE_S;
		case SDLK_t: return SDL_SCANCODE_T;
		case SDLK_u: return SDL_SCANCODE_U;
		case SDLK_v: return SDL_SCANCODE_V;
		case SDLK_w: return SDL_SCANCODE_W;
		case SDLK_x: return SDL_SCANCODE_X;
		case SDLK_y: return SDL_SCANCODE_Y;
		case SDLK_z: return SDL_SCANCODE_Z;

		default: return SDL_SCANCODE_UNKNOWN;
	}
}


BEGIN_METHOD(Key_IsPressed, GB_INTEGER key; GB_BOOLEAN ignore)

	int code = VARG(key);
	bool pressed = FALSE;

	if (VARGOPT(ignore, FALSE))
		code = convert_keycode_to_scancode(code);
	else
		code = SDL_GetScancodeFromKey(code);

	if (code != SDL_SCANCODE_UNKNOWN)
	{
		int count;
		const Uint8 *keystate = SDL_GetKeyboardState(&count);
		if (code >= 0 && code < count)
			pressed = keystate[code];
	}

	GB.ReturnBoolean(pressed);

END_METHOD

//-------------------------------------------------------------------------

GB_DESC KeyDesc[] =
{
	GB_DECLARE_STATIC("Key"),
	
	GB_STATIC_METHOD("_get", "i", Key_get, "(Key)s"),

  GB_STATIC_PROPERTY_READ("Code", "i", Key_Code),
  GB_STATIC_PROPERTY_READ("Shift", "b", Key_Shift),
  GB_STATIC_PROPERTY_READ("Control", "b", Key_Control),
  GB_STATIC_PROPERTY_READ("Alt", "b", Key_Alt),
  GB_STATIC_PROPERTY_READ("AltGr", "b", Key_AltGr),
  GB_STATIC_PROPERTY_READ("Meta", "b", Key_Meta),
  GB_STATIC_PROPERTY_READ("Normal", "b", Key_Normal),
  GB_STATIC_PROPERTY_READ("Text", "s", Key_Text),
	GB_STATIC_PROPERTY("Repeat", "b", Key_Repeat),

	GB_STATIC_METHOD("IsPressed", "b", Key_IsPressed, "(Key)i[(IgnoreLayout)b]"),

	GB_CONSTANT("Backspace", "i", SDLK_BACKSPACE),
	GB_CONSTANT("Tab", "i", SDLK_TAB),
	GB_CONSTANT("Return", "i", SDLK_RETURN),
	GB_CONSTANT("Pause", "i", SDLK_PAUSE),
	GB_CONSTANT("Escape", "i", SDLK_ESCAPE),
	GB_CONSTANT("Esc", "i", SDLK_ESCAPE),
	GB_CONSTANT("Space", "i", SDLK_SPACE),
	GB_CONSTANT("Delete", "i", SDLK_DELETE),
	GB_CONSTANT("Num0", "i", SDLK_KP_0),
	GB_CONSTANT("Num1", "i", SDLK_KP_1),
	GB_CONSTANT("Num2", "i", SDLK_KP_2),
	GB_CONSTANT("Num3", "i", SDLK_KP_3),
	GB_CONSTANT("Num4", "i", SDLK_KP_4),
	GB_CONSTANT("Num5", "i", SDLK_KP_5),
	GB_CONSTANT("Num6", "i", SDLK_KP_6),
	GB_CONSTANT("Num7", "i", SDLK_KP_7),
	GB_CONSTANT("Num8", "i", SDLK_KP_8),
	GB_CONSTANT("Num9", "i", SDLK_KP_9),
	GB_CONSTANT("NumPeriod", "i", SDLK_KP_PERIOD),
	GB_CONSTANT("NumDivide", "i", SDLK_KP_DIVIDE),
	GB_CONSTANT("NumMultiply", "i", SDLK_KP_MULTIPLY),
	GB_CONSTANT("NumMinus", "i", SDLK_KP_MINUS),
	GB_CONSTANT("NumPlus", "i", SDLK_KP_PLUS),
	GB_CONSTANT("NumEnter", "i", SDLK_KP_ENTER),
	//GB_CONSTANT("NumEquals", "i", SDLK_KP_EQUALS),
	GB_CONSTANT("Up", "i", SDLK_UP),
	GB_CONSTANT("Down", "i", SDLK_DOWN),
	GB_CONSTANT("Right", "i", SDLK_RIGHT),
	GB_CONSTANT("Left", "i", SDLK_LEFT),
	GB_CONSTANT("Insert", "i", SDLK_INSERT),
	GB_CONSTANT("Home", "i", SDLK_HOME),
	GB_CONSTANT("End", "i", SDLK_END),
	GB_CONSTANT("PageUp", "i", SDLK_PAGEUP),
	GB_CONSTANT("PageDown", "i", SDLK_PAGEDOWN),
	GB_CONSTANT("F1", "i", SDLK_F1),
	GB_CONSTANT("F2", "i", SDLK_F2),
	GB_CONSTANT("F3", "i", SDLK_F3),
	GB_CONSTANT("F4", "i", SDLK_F4),
	GB_CONSTANT("F5", "i", SDLK_F5),
	GB_CONSTANT("F6", "i", SDLK_F6),
	GB_CONSTANT("F7", "i", SDLK_F7),
	GB_CONSTANT("F8", "i", SDLK_F8),
	GB_CONSTANT("F9", "i", SDLK_F9),
	GB_CONSTANT("F10", "i", SDLK_F10),
	GB_CONSTANT("F11", "i", SDLK_F11),
	GB_CONSTANT("F12", "i", SDLK_F12),
	GB_CONSTANT("F13", "i", SDLK_F13),
	GB_CONSTANT("F14", "i", SDLK_F14),
	GB_CONSTANT("F15", "i", SDLK_F15),
	GB_CONSTANT("NumLock", "i", SDLK_NUMLOCKCLEAR),
	GB_CONSTANT("CapsLock", "i", SDLK_CAPSLOCK),
	GB_CONSTANT("ScrollLock", "i", SDLK_SCROLLLOCK),
	GB_CONSTANT("RightShift", "i", SDLK_RSHIFT),
	GB_CONSTANT("LeftShift", "i", SDLK_LSHIFT),
	GB_CONSTANT("RightControl", "i", SDLK_RCTRL),
	GB_CONSTANT("LeftControl", "i", SDLK_LCTRL),
	GB_CONSTANT("RightAlt", "i", SDLK_RALT),
	GB_CONSTANT("LeftAlt", "i", SDLK_LALT),
	GB_CONSTANT("RightMeta", "i", SDLK_RGUI),
	GB_CONSTANT("LeftMeta", "i", SDLK_LGUI),
	GB_CONSTANT("AltGrKey", "i", SDLK_MODE),
	//GB_CONSTANT("Compose", "i", SDLK_COMPOSE),
	//GB_CONSTANT("Help", "i", SDLK_HELP),
	//GB_CONSTANT("Print", "i", SDLK_PRINT),
	GB_CONSTANT("SysReq", "i", SDLK_SYSREQ),
	//GB_CONSTANT("Break", "i", SDLK_BREAK),
	GB_CONSTANT("Menu", "i", SDLK_MENU),
	//GB_CONSTANT("Power", "i", SDLK_POWER),
	//GB_CONSTANT("Euro", "i", SDLK_EURO),
	//GB_CONSTANT("Undo", "i", SDLK_UNDO),

	GB_END_DECLARE
};
