/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "FormTable.h"

#include <algorithm>
#include <array>

#include "ChlSyntax.h"

namespace openblack::lhvm::chl
{

namespace
{

void CollectWords(const std::vector<PatternItem>& items, std::set<std::string, std::less<>>& words)
{
	const auto addWords = [&words](std::string_view text) {
		size_t start = 0;
		while (start < text.size())
		{
			auto end = text.find(' ', start);
			if (end == std::string_view::npos)
			{
				end = text.size();
			}
			if (end > start)
			{
				words.emplace(text.substr(start, end - start));
			}
			start = end + 1;
		}
	};
	for (const auto& item : items)
	{
		switch (item.kind)
		{
		case PatternItemKind::Word:
		case PatternItemKind::Flag:
			addWords(item.text);
			break;
		case PatternItemKind::Choice:
			for (const auto& [text, value] : item.choices)
			{
				addWords(text);
			}
			break;
		case PatternItemKind::Optional:
			CollectWords(item.items, words);
			break;
		default:
			break;
		}
	}
}

/// Highest parameter index the items give a value to, plus one
size_t CountArguments(const std::vector<PatternItem>& items)
{
	size_t count = 0;
	for (const auto& item : items)
	{
		if (item.argument >= 0)
		{
			count = std::max(count, static_cast<size_t>(item.argument) + 1);
		}
		if (item.kind == PatternItemKind::Optional)
		{
			count = std::max(count, CountArguments(item.items));
		}
	}
	return count;
}

} // namespace

ArgType ParameterType(const NativeSignature& signature, size_t index)
{
	return index < signature.params.size() ? signature.params[index].type : ArgType::Any;
}

ArgType ResultType(const NativeSignature& signature)
{
	return signature.returnType;
}

FormTable::FormTable(std::span<const NativeSignature> natives, const std::multimap<std::string, uint32_t, std::less<>>& index)
{
	const auto forms = StatementForms();
	_forms.reserve(forms.size());
	for (const auto& form : forms)
	{
		// Positions are written [Thing] by the expression grammar itself
		if (form.native == "GET_POSITION")
		{
			continue;
		}
		CompiledForm compiled;
		compiled.source = &form;
		compiled.items = ParsePattern(form.pattern);
		if (compiled.items.empty())
		{
			_rejected.emplace_back(form.pattern);
			continue;
		}
		compiled.argumentCount = CountArguments(compiled.items);
		// The native the form calls: the form says which of those sharing its name
		const auto [first, last] = index.equal_range(std::string(form.native));
		const NativeSignature* chosen = nullptr;
		size_t nth = 0;
		for (auto it = first; it != last; ++it, ++nth)
		{
			if (nth == form.overload)
			{
				chosen = &natives[it->second];
				compiled.native = it->second;
			}
		}
		if (chosen == nullptr || chosen->params.size() < compiled.argumentCount)
		{
			_rejected.emplace_back(form.pattern);
			continue;
		}
		compiled.signature = chosen;
		compiled.postfix = compiled.items.front().kind == PatternItemKind::Argument;
		compiled.negated = form.negated;
		_forms.push_back(std::move(compiled));
	}

	for (const auto& form : _forms)
	{
		CollectWords(form.items, _keywords);
		if (form.postfix)
		{
			_postfix.push_back(&form);
		}
		else
		{
			const auto& first = form.items.front();
			std::set<std::string, std::less<>> starts;
			if (first.kind == PatternItemKind::Word)
			{
				starts.insert(first.text);
			}
			else
			{
				// A choice, flag or optional group: every word that can open it
				CollectWords({first}, starts);
				if (first.kind == PatternItemKind::Optional || first.kind == PatternItemKind::Flag)
				{
					// The group may be absent: the form can also start with what follows. Index it under every
					// keyword and let the matcher decide.
					CollectWords(form.items, starts);
				}
			}
			for (const auto& word : starts)
			{
				_prefix[word].push_back(&form);
			}
		}
	}
	for (const auto keyword : k_Keywords)
	{
		_keywords.emplace(keyword);
	}
}

std::span<const CompiledForm* const> FormTable::Prefix(std::string_view word) const
{
	const auto it = _prefix.find(word);
	if (it == _prefix.end())
	{
		return {};
	}
	return it->second;
}

bool FormTable::IsKeyword(std::string_view word) const
{
	return _keywords.contains(word);
}

std::vector<const CompiledForm*> FormTable::FormsOf(std::string_view native) const
{
	std::vector<const CompiledForm*> result;
	for (const auto& form : _forms)
	{
		if (form.source->native == native)
		{
			result.push_back(&form);
		}
	}
	return result;
}

} // namespace openblack::lhvm::chl
