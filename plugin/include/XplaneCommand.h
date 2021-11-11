/*
 * xPilot: X-Plane pilot client for VATSIM
 * Copyright (C) 2019-2021 Justin Shannon
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
*/

#ifndef XplaneCommand_h
#define XplaneCommand_h

#include <functional>
#include "XPLMUtilities.h"

namespace xpilot
{
	class XplaneCommand
	{
	public:
		XplaneCommand(const char* name, const char* description, std::function<void()> handler) :
			m_handler(handler),
			m_command(XPLMCreateCommand(name, description))
		{
			XPLMRegisterCommandHandler(m_command, callback, false, static_cast<void*>(this));
		}

		~XplaneCommand()
		{
			XPLMUnregisterCommandHandler(m_command, callback, false, static_cast<void*>(this));
		}

		XplaneCommand(const XplaneCommand&) = delete;
		XplaneCommand& operator =(const XplaneCommand&) = delete;
	private:
		static int callback(XPLMCommandRef, XPLMCommandPhase phase, void* refcon)
		{
			if (phase == xplm_CommandBegin) { (static_cast<XplaneCommand*>(refcon)->m_handler)(); }
			return 1;
		}
		std::function<void()> m_handler;
		XPLMCommandRef m_command;
	};
}

#endif // !XplaneCommand_h
