/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <functional>
#include <unordered_map>

#include "Graphics/RendererInterface.h"
#include "Technique.h"

namespace openblack::graphics
{

template <Technique technique>
void renderTechnique(RendererInterface::DrawSceneDesc desc);

template <>
void renderTechnique<Technique::Sky>(RendererInterface::DrawSceneDesc desc);
template <>
void renderTechnique<Technique::Water>(RendererInterface::DrawSceneDesc desc);
template <>
void renderTechnique<Technique::Island>(RendererInterface::DrawSceneDesc desc);
template <>
void renderTechnique<Technique::Entities>(RendererInterface::DrawSceneDesc desc);
template <>
void renderTechnique<Technique::Sprites>(RendererInterface::DrawSceneDesc desc);
template <>
void renderTechnique<Technique::TestModel>(RendererInterface::DrawSceneDesc desc);
template <>
void renderTechnique<Technique::DebugCross>(RendererInterface::DrawSceneDesc desc);
template <>
void renderTechnique<Technique::Footprint>(RendererInterface::DrawSceneDesc desc);

static const std::unordered_map<Technique, std::function<void(RendererInterface::DrawSceneDesc)>> k_TechniqueMap {
    {Technique::Sky, renderTechnique<Technique::Sky>},
    {Technique::Water, renderTechnique<Technique::Water>},
    {Technique::Island, renderTechnique<Technique::Island>},
    {Technique::Entities, renderTechnique<Technique::Entities>},
    {Technique::Sprites, renderTechnique<Technique::Sprites>},
    {Technique::TestModel, renderTechnique<Technique::TestModel>},
    {Technique::DebugCross, renderTechnique<Technique::DebugCross>},
    {Technique::Footprint, renderTechnique<Technique::Footprint>}};

} // namespace openblack::graphics
