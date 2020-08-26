/*
 * Copyright 2011-2020 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#ifndef ENTRY_H_HEADER_GUARD
#define ENTRY_H_HEADER_GUARD

#include "dbg.h"
#include <bx/bx.h>
#include <bx/filepath.h>
#include <bx/string.h>
#include <bx/spscqueue.h>



namespace bx { struct FileReaderI; struct FileWriterI; struct AllocatorI; }

extern "C" int _main_(int _argc, char** _argv);

#define ENTRY_WINDOW_FLAG_NONE         UINT32_C(0x00000000)
#define ENTRY_WINDOW_FLAG_ASPECT_RATIO UINT32_C(0x00000001)
#define ENTRY_WINDOW_FLAG_FRAME        UINT32_C(0x00000002)

#ifndef ENTRY_CONFIG_IMPLEMENT_MAIN
#	define ENTRY_CONFIG_IMPLEMENT_MAIN 0
#endif // ENTRY_CONFIG_IMPLEMENT_MAIN

#if ENTRY_CONFIG_IMPLEMENT_MAIN
#define ENTRY_IMPLEMENT_MAIN(_app, ...)                 \
	int _main_(int _argc, char** _argv)                 \
	{                                                   \
			_app app(__VA_ARGS__);                      \
			return entry::runApp(&app, _argc, _argv);   \
	}
#else
#define ENTRY_IMPLEMENT_MAIN(_app, ...) \
	_app s_ ## _app ## App(__VA_ARGS__)
#endif // ENTRY_CONFIG_IMPLEMENT_MAIN

namespace entry
{
	struct WindowHandle  { uint16_t idx; };
	inline bool isValid(WindowHandle _handle)  { return UINT16_MAX != _handle.idx; }

	struct GamepadHandle { uint16_t idx; };
	inline bool isValid(GamepadHandle _handle) { return UINT16_MAX != _handle.idx; }


	struct Key
	{
		enum Enum
		{
			None = 0,
			Esc,
			Return,
			Tab,
			Space,
			Backspace,
			Up,
			Down,
			Left,
			Right,
			Insert,
			Delete,
			Home,
			End,
			PageUp,
			PageDown,
			Print,
			Plus,
			Minus,
			LeftBracket,
			RightBracket,
			Semicolon,
			Quote,
			Comma,
			Period,
			Slash,
			Backslash,
			Tilde,
			F1,
			F2,
			F3,
			F4,
			F5,
			F6,
			F7,
			F8,
			F9,
			F10,
			F11,
			F12,
			NumPad0,
			NumPad1,
			NumPad2,
			NumPad3,
			NumPad4,
			NumPad5,
			NumPad6,
			NumPad7,
			NumPad8,
			NumPad9,
			Key0,
			Key1,
			Key2,
			Key3,
			Key4,
			Key5,
			Key6,
			Key7,
			Key8,
			Key9,
			KeyA,
			KeyB,
			KeyC,
			KeyD,
			KeyE,
			KeyF,
			KeyG,
			KeyH,
			KeyI,
			KeyJ,
			KeyK,
			KeyL,
			KeyM,
			KeyN,
			KeyO,
			KeyP,
			KeyQ,
			KeyR,
			KeyS,
			KeyT,
			KeyU,
			KeyV,
			KeyW,
			KeyX,
			KeyY,
			KeyZ,

			GamepadA,
			GamepadB,
			GamepadX,
			GamepadY,
			GamepadThumbL,
			GamepadThumbR,
			GamepadShoulderL,
			GamepadShoulderR,
			GamepadUp,
			GamepadDown,
			GamepadLeft,
			GamepadRight,
			GamepadBack,
			GamepadStart,
			GamepadGuide,

			Count
		};
	};

	bx::FileReaderI* getFileReader();
	bx::FileWriterI* getFileWriter();
	bx::AllocatorI*  getAllocator();

	
} // namespace entry

#endif // ENTRY_H_HEADER_GUARD
