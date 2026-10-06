/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <imgui.h>

/// The editor's colours, shared by its panels so code, lists and the world's overlays read alike
namespace openblack::editor::style
{

constexpr ImVec4 k_Accent {0.36f, 0.62f, 0.98f, 1.0f};
constexpr ImVec4 k_Muted {0.55f, 0.57f, 0.60f, 1.0f};
constexpr ImVec4 k_Good {0.45f, 0.82f, 0.45f, 1.0f};
constexpr ImVec4 k_Warning {0.98f, 0.76f, 0.30f, 1.0f};
constexpr ImVec4 k_Error {0.97f, 0.40f, 0.38f, 1.0f};

// Code
constexpr ImVec4 k_CodeBackground {0.11f, 0.12f, 0.13f, 1.0f};
constexpr ImVec4 k_Address {0.45f, 0.47f, 0.50f, 1.0f};
constexpr ImVec4 k_Opcode {0.80f, 0.55f, 0.95f, 1.0f};
constexpr ImVec4 k_Number {0.70f, 0.85f, 0.55f, 1.0f};
constexpr ImVec4 k_Global {0.95f, 0.75f, 0.45f, 1.0f};
constexpr ImVec4 k_Local {0.90f, 0.90f, 0.80f, 1.0f};
constexpr ImVec4 k_Native {0.45f, 0.78f, 0.98f, 1.0f};
constexpr ImVec4 k_Script {0.40f, 0.90f, 0.75f, 1.0f};
constexpr ImVec4 k_Jump {0.98f, 0.60f, 0.45f, 1.0f};
constexpr ImVec4 k_String {0.85f, 0.80f, 0.45f, 1.0f};
constexpr ImVec4 k_Comment {0.45f, 0.50f, 0.45f, 1.0f};
constexpr ImU32 k_CurrentLine = IM_COL32(60, 110, 200, 90);
constexpr ImU32 k_TaskLine = IM_COL32(230, 180, 60, 70);
constexpr ImU32 k_Breakpoint = IM_COL32(230, 70, 70, 255);

// The world's overlays
constexpr ImU32 k_SelectionBox = IM_COL32(90, 170, 255, 255);
constexpr ImU32 k_HoverBox = IM_COL32(255, 255, 255, 120);
constexpr ImU32 k_GhostBox = IM_COL32(120, 255, 140, 230);

} // namespace openblack::editor::style
