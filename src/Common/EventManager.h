/* openblack - A reimplementation of Lionhead's Black & White.
 *
 * openblack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * openblack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * openblack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openblack. If not, see <http://www.gnu.org/licenses/>.
 */

#include <Common/Queue.h>
#include <SDL.h>
#include <functional>
#include <stdexcept>
#include <unordered_map>

#pragma once

namespace openblack
{

class EventManager;

class EventManager
{
public:
	template <typename Event, typename... Args>
	void Create(Event event)
	{
		Insist<Event>()->Produce(event);
	}

	template <typename Event>
	void AddHandler(EventHandler<Event> callback)
	{
		Insist<Event>()->AddHandler(callback);
	}

private:
	class _EventFamily
	{
	private:
		inline static std::atomic<int> identifier = 0;
	public:
		template <typename... T>
		inline static const int Type = identifier++;
	};
	mutable std::unordered_map<int, std::unique_ptr<IEventQueue>> queues {};
	template <typename Event>
	EventQueue<Event>* Insist()
	{
		const auto eventType = EventManager::_EventFamily::Type<Event>;
		std::unique_ptr<IEventQueue> &p = queues[eventType];

		if (!p)
		{
			p.reset(new EventQueue<Event>());
		}

		return static_cast<EventQueue<Event>*>(p.get());
	}
};
} // namespace openblack
