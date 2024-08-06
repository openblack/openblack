/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <list>
#include <unordered_map>
#include <unordered_set>

namespace openblack
{

template <typename T>
class ReferenceManager
{
public:
	uint32_t GetReference(const T target)
	{
		if (_target2id.contains(target))
		{
			return _target2id[target];
		}
		else
		{
			uint32_t id;
			if (_freeList.size() > 0)
			{
				id = _freeList.front();
				_freeList.pop_front();
			}
			else
			{
			    id = ++_maxId;
			}
			_id2ref.emplace(id, Reference {.id = id, .count = 0, .target = target});
			_target2id[target] = id;
			return id;
		}
	}

	void AddReference(const uint32_t id)
	{
		if (_id2ref.contains(id))
		{
			if (_id2ref[id].count++ == 0)
			{
				_garbage.erase(id);
			}
		}
	}

	void RemoveReference(const uint32_t id)
	{
		if (_id2ref.contains(id))
		{
			auto& ref = _id2ref[id];
			if (--ref.count <= 0)
			{
				_garbage.insert(id);
			}
		}
	}

	T* GetTarget(const uint32_t id)
	{
		if (_id2ref.contains(id))
		{
			auto& ref = _id2ref[id];
			ref.count++;
			return &ref.target;
		}
		else
		{
			return NULL;
		}
	}

	void ForceReference(const T target, const uint32_t id)
	{
		if (_target2id.contains(target))
		{
			if (_target2id[target] != id)
			{
				throw std::runtime_error("Inconsistent reference ID");
			}
		}
		else
		{
			_id2ref.emplace(id, Reference {.id = id, .count = 0, .target = target});
			_target2id[target] = id;
		}
		_id2ref[id].count++;
		_maxId = std::max(_maxId, id);
	}

	void Clear()
	{
		_maxId = 0;
		_freeList.clear();
		_id2ref.clear();
		_target2id.clear();
		_garbage.clear();
	}

	/// Run Garbage Collector
	void GC()
	{
		for (const auto id : _garbage)
		{
			auto ref = _id2ref[id];
			if (ref.count <= 0)
			{
				_target2id.erase(ref.target);
				_id2ref.erase(id);
			}
		}
		_garbage.clear();
		if (_id2ref.size() == 0)
		{
			_maxId = 0;
		}
	}

private:
	struct Reference
	{
		uint32_t id;
		int count;
		T target;
	};

	uint32_t _maxId {0};
	std::list<uint32_t> _freeList;
	std::unordered_map<uint32_t, Reference> _id2ref;
	std::unordered_map<T, uint32_t> _target2id;
	std::unordered_set<uint32_t> _garbage;
};

} // namespace openblack::LHVM
