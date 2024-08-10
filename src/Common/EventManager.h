/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <functional>
#include <memory>
#include <unordered_map>

#include "Queue.h"

#pragma once

namespace openblack
{
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
		inline static const int k_Type = identifier++;
	};
	mutable std::unordered_map<int, std::unique_ptr<IEventQueue>> _queues {};
	template <typename Event>
	EventQueue<Event>* Insist()
	{
		const auto eventType = EventManager::_EventFamily::k_Type<Event>;
		std::unique_ptr<IEventQueue>& p = _queues[eventType];

		if (!p)
		{
			p.reset(new EventQueue<Event>());
		}

		return static_cast<EventQueue<Event>*>(p.get());
	}
};
} // namespace openblack
