/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

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
