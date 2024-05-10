/*******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <algorithm>

#include <glm/ext/vector_int2.hpp>

#include "Mock.h"

class DragUpDownMockAction final: public MockAction
{
	static constexpr uint32_t k_Start1 = k_StabilizeFrames + 2;
	static constexpr uint32_t k_End1 = k_Start1 - 1 + k_InteractionFrames;
	static constexpr uint32_t k_Start2 = k_End1 + 1;
	static constexpr uint32_t k_End2 = k_Start2 - 1 + k_StabilizeFrames;
	static constexpr uint32_t k_Start3 = k_End2 + 1;
	static constexpr uint32_t k_End3 = k_Start3 - 1 + k_InteractionFrames;
	static constexpr uint32_t k_LiftMouse =
	    k_StabilizeFrames + k_InteractionFrames + k_StabilizeFrames + k_InteractionFrames + k_StabilizeFrames;

public:
	~DragUpDownMockAction() final = default;

	[[nodiscard]] bool GetBindable(openblack::input::BindableActionMap action) const final
	{
		using openblack::input::BindableActionMap;
		if (frameNumber >= k_Start1 - 1 && frameNumber < k_End3)
		{
			return (static_cast<uint32_t>(action) & static_cast<uint32_t>(BindableActionMap::MOVE)) != 0;
		}

		return false;
	}

	[[nodiscard]] glm::uvec2 GetMousePosition() const final
	{
		if (frameNumber < k_Start1)
		{
			return {k_Width / 2, 450};
		}
		if (frameNumber < k_Start2)
		{
			auto t = std::max((frameNumber - 1.0f - k_StabilizeFrames) / k_InteractionFrames, 0.0f);
			return {k_Width / 2, (k_Height - 150.0f - t * 100.0f)};
		}
		if (frameNumber < k_Start3)
		{
			return {k_Width / 2, k_Height - 250};
		}
		if (frameNumber <= k_End3)
		{
			auto t = std::max((frameNumber - 1.0f - (k_StabilizeFrames + k_InteractionFrames + k_StabilizeFrames)) /
			                      k_InteractionFrames,
			                  0.0f);
			return {k_Width / 2, (k_Height - 150.0f - (1.0f - t) * 100.0f)};
		}

		return {k_Width / 2, 450};
	}

	[[nodiscard]] std::array<std::optional<glm::vec3>, 2> GetHandPositions() const final
	{
		return {{{}, {{1000.0f, 0.0f, 1000.0f}}}};
	}
};

// NOLINTBEGIN(bugprone-branch-clone, google-readability-function-size, readability-function-size)
class DragUpDownMockDynamicsSystem final: public MockDynamicsSystem
{
public:
	~DragUpDownMockDynamicsSystem() final = default;

	[[nodiscard]] std::optional<glm::vec2> RayCastClosestHitScreenCoord(glm::u16vec2 screenCoord) const override
	{
		if (screenCoord == k_ScreenCentreLine[0])
		{
			switch (frameNumber)
			{
			case 0:
				return std::nullopt;
			case 202:
				return std::nullopt;
			case 602:
				return std::nullopt;
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == k_ScreenCentreLine[1])
		{
			switch (frameNumber)
			{
			case 0:
				return std::nullopt;
			case 202:
				return std::nullopt;
			case 602:
				return std::nullopt;
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == k_ScreenCentreLine[2])
		{
			switch (frameNumber)
			{
			case 0:
				return {{0.0000000f, 563.2467041f}};
			case 202:
				return std::nullopt;
			case 602:
				return std::nullopt;
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == k_ScreenCentreLine[3])
		{
			switch (frameNumber)
			{
			case 0:
				return {{0.0000000f, 189.2355652f}};
			case 202:
				return std::nullopt;
			case 602:
				return std::nullopt;
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == k_ScreenCentreLine[4])
		{
			switch (frameNumber)
			{
			case 0:
				return {{0.0000000f, 100.4535294f}};
			case 202:
				return std::nullopt;
			case 602:
				return std::nullopt;
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == k_ScreenCentreLine[5])
		{
			switch (frameNumber)
			{
			case 0:
				return {{0.0000000f, 62.1956444f}};
			case 202:
				return std::nullopt;
			case 602:
				return std::nullopt;
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == k_ScreenCentreLine[6])
		{
			switch (frameNumber)
			{
			case 0:
				return {{0.0000000f, 39.9825211f}};
			case 202:
				return std::nullopt;
			case 602:
				return std::nullopt;
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == k_ScreenCentreLine[7])
		{
			switch (frameNumber)
			{
			case 0:
				return {{0.0000000f, 26.0952835f}};
			case 202:
				return std::nullopt;
			case 602:
				return std::nullopt;
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == k_ScreenCentreLine[8])
		{
			switch (frameNumber)
			{
			case 0:
				return {{0.0000000f, 16.1538353f}};
			case 1:
				return {{1000.0000000f, -770.0559082f}};
			case 2:
				return {{1000.0000000f, -743.0767822f}};
			case 3:
				return {{1000.0000000f, -5.7276611f}};
			case 4:
				return {{1000.0000000f, 105.8948517f}};
			case 5:
				return {{1000.0000000f, 112.1076202f}};
			case 6:
				return {{1000.0000000f, 118.2447052f}};
			case 7:
				return {{1000.0000000f, 200.3244019f}};
			case 8:
				return {{1000.0000000f, 204.8554993f}};
			case 9:
				return {{1000.0000000f, 264.9228516f}};
			case 10:
				return {{1000.0000000f, 268.0856323f}};
			case 11:
				return {{1000.0000000f, 271.2250366f}};
			case 12:
				return {{1000.0000000f, 313.6578369f}};
			case 13:
				return {{1000.0000000f, 316.1343384f}};
			case 14:
				return {{1000.0000000f, 318.7072144f}};
			case 15:
				return {{1000.0000000f, 321.1528931f}};
			case 16:
				return {{1000.0000000f, 355.7194519f}};
			case 17:
				return {{1000.0000000f, 357.7429199f}};
			case 18:
				return {{1000.0000000f, 384.0479126f}};
			case 19:
				return {{1000.0000000f, 407.4533691f}};
			case 20:
				return {{1000.0000000f, 427.1724854f}};
			case 21:
				return {{1000.0000000f, 428.2545471f}};
			case 22:
				return {{1000.0000000f, 443.9299011f}};
			case 23:
				return {{1000.0000000f, 444.8811340f}};
			case 24:
				return {{1000.0000000f, 459.2140198f}};
			case 25:
				return {{1000.0000000f, 460.0471497f}};
			case 26:
				return {{1000.0000000f, 472.5143433f}};
			case 27:
				return {{1000.0000000f, 473.2416382f}};
			case 28:
				return {{1000.0000000f, 483.4656067f}};
			case 29:
				return {{1000.0000000f, 484.1685791f}};
			case 30:
				return {{1000.0000000f, 493.8484192f}};
			case 31:
				return {{1000.0000000f, 494.3926697f}};
			case 32:
				return {{1000.0000000f, 502.4442139f}};
			case 33:
				return {{1000.0000000f, 502.9736938f}};
			case 34:
				return {{1000.0000000f, 510.7427979f}};
			case 35:
				return {{1000.0000000f, 511.1937256f}};
			case 36:
				return {{1000.0000000f, 518.1301270f}};
			case 37:
				return {{1000.0000000f, 518.5700684f}};
			case 38:
				return {{1000.0000000f, 519.0095215f}};
			case 39:
				return {{1000.0000000f, 500.6634216f}};
			case 40:
				return {{1000.0000000f, 498.8476868f}};
			case 41:
				return {{1000.0000000f, 471.6894531f}};
			case 42:
				return {{1000.0000000f, 469.9794006f}};
			case 43:
				return {{1000.0000000f, 442.9660645f}};
			case 44:
				return {{1000.0000000f, 417.5871277f}};
			case 45:
				return {{1000.0000000f, 391.1619568f}};
			case 46:
				return {{1000.0000000f, 389.5305786f}};
			case 47:
				return {{1000.0000000f, 364.7833862f}};
			case 48:
				return {{1000.0000000f, 363.1747437f}};
			case 49:
				return {{1000.0000000f, 337.2171631f}};
			case 50:
				return {{1000.0000000f, 335.5335999f}};
			case 51:
				return {{1000.0000000f, 309.9683228f}};
			case 52:
				return {{1000.0000000f, 308.3560791f}};
			case 53:
				return {{1000.0000000f, 285.0237427f}};
			case 54:
				return {{1000.0000000f, 283.0931091f}};
			case 55:
				return {{1000.0000000f, 258.5480347f}};
			case 56:
				return {{1000.0000000f, 256.9194946f}};
			case 57:
				return {{1000.0000000f, 232.4271393f}};
			case 58:
				return {{1000.0000000f, 230.9935760f}};
			case 59:
				return {{1000.0000000f, 208.8545990f}};
			case 60:
				return {{1000.0000000f, 207.3721313f}};
			case 61:
				return {{1000.0000000f, 184.0393066f}};
			case 62:
				return {{1000.0000000f, 182.6068115f}};
			case 63:
				return {{1000.0000000f, 161.3394165f}};
			case 64:
				return {{1000.0000000f, 159.7380066f}};
			case 65:
				return {{1000.0000000f, 137.5003815f}};
			case 66:
				return {{1000.0000000f, 115.8879089f}};
			case 67:
				return {{1000.0000000f, 95.8544159f}};
			case 68:
				return {{1000.0000000f, 94.5362091f}};
			case 69:
				return {{1000.0000000f, 73.7548065f}};
			case 70:
				return {{1000.0000000f, 72.4786377f}};
			case 71:
				return {{1000.0000000f, 52.5038910f}};
			case 72:
				return {{1000.0000000f, 51.1960602f}};
			case 73:
				return {{1000.0000000f, 32.9644012f}};
			case 74:
				return {{1000.0000000f, 31.5235901f}};
			case 75:
				return {{1000.0000000f, 30.2701569f}};
			case 76:
				return {{1000.0000000f, 11.1421204f}};
			case 77:
				return {{1000.0000000f, 10.1613617f}};
			case 78:
				return {{1000.0000000f, -7.2264404f}};
			case 79:
				return {{1000.0000000f, -8.4018555f}};
			case 80:
				return {{1000.0000000f, -26.1702881f}};
			case 81:
				return {{1000.0000000f, -27.1201172f}};
			case 82:
				return {{1000.0000000f, -44.8266602f}};
			case 83:
				return {{1000.0000000f, -45.5584717f}};
			case 84:
				return {{1000.0000000f, -61.5065918f}};
			case 85:
				return {{1000.0000000f, -62.6777344f}};
			case 86:
				return {{1000.0000000f, -78.9976807f}};
			case 87:
				return {{1000.0000000f, -79.9364014f}};
			case 88:
				return {{1000.0000000f, -95.5395508f}};
			case 89:
				return {{1000.0000000f, -96.6873779f}};
			case 90:
				return {{1000.0000000f, -110.8873291f}};
			case 91:
				return {{1000.0000000f, -125.5928955f}};
			case 92:
				return {{1000.0000000f, -139.0494385f}};
			case 93:
				return {{1000.0000000f, -139.7036133f}};
			case 94:
				return {{1000.0000000f, -153.5233154f}};
			case 95:
				return {{1000.0000000f, -154.3769531f}};
			case 96:
				return {{1000.0000000f, -167.5012207f}};
			case 97:
				return {{1000.0000000f, -168.5506592f}};
			case 98:
				return {{1000.0000000f, -180.2821045f}};
			case 99:
				return {{1000.0000000f, -181.0770264f}};
			case 100:
				return {{1000.0000000f, -193.0888672f}};
			case 101:
				return {{1000.0000000f, -194.0817871f}};
			case 102:
				return {{1000.0000000f, -205.6433105f}};
			case 103:
				return {{1000.0000000f, -216.1108398f}};
			case 104:
				return {{1000.0000000f, -216.8082275f}};
			case 105:
				return {{1000.0000000f, -227.3505859f}};
			case 106:
				return {{1000.0000000f, -228.0187988f}};
			case 107:
				return {{1000.0000000f, -228.5657959f}};
			case 108:
				return {{1000.0000000f, -238.1468506f}};
			case 109:
				return {{1000.0000000f, -238.7722168f}};
			case 110:
				return {{1000.0000000f, -248.4827881f}};
			case 111:
				return {{1000.0000000f, -249.0815430f}};
			case 112:
				return {{1000.0000000f, -258.1691895f}};
			case 113:
				return {{1000.0000000f, -258.7281494f}};
			case 114:
				return {{1000.0000000f, -275.2501221f}};
			case 115:
				return {{1000.0000000f, -283.1724854f}};
			case 116:
				return {{1000.0000000f, -283.3957520f}};
			case 117:
				return {{1000.0000000f, -290.4317627f}};
			case 118:
				return {{1000.0000000f, -290.8327637f}};
			case 119:
				return {{1000.0000000f, -297.8603516f}};
			case 120:
				return {{1000.0000000f, -298.2362061f}};
			case 121:
				return {{1000.0000000f, -304.2756348f}};
			case 122:
				return {{1000.0000000f, -304.6058350f}};
			case 123:
				return {{1000.0000000f, -305.0563965f}};
			case 124:
				return {{1000.0000000f, -311.0228271f}};
			case 125:
				return {{1000.0000000f, -316.6230469f}};
			case 126:
				return {{1000.0000000f, -317.1135254f}};
			case 127:
				return {{1000.0000000f, -321.9949951f}};
			case 128:
				return {{1000.0000000f, -322.4465332f}};
			case 129:
				return {{1000.0000000f, -322.8072510f}};
			case 130:
				return {{1000.0000000f, -327.5804443f}};
			case 131:
				return {{1000.0000000f, -332.0192871f}};
			case 132:
				return {{1000.0000000f, -332.1802979f}};
			case 133:
				return {{1000.0000000f, -336.0573730f}};
			case 134:
				return {{1000.0000000f, -336.1817627f}};
			case 135:
				return {{1000.0000000f, -340.1765137f}};
			case 136:
				return {{1000.0000000f, -340.2766113f}};
			case 137:
				return {{1000.0000000f, -343.6551514f}};
			case 138:
				return {{1000.0000000f, -346.5415039f}};
			case 139:
				return {{1000.0000000f, -349.7819824f}};
			case 140:
				return {{1000.0000000f, -349.7995605f}};
			case 141:
				return {{1000.0000000f, -352.2539062f}};
			case 142:
				return {{1000.0000000f, -352.4711914f}};
			case 143:
				return {{1000.0000000f, -354.5693359f}};
			case 144:
				return {{1000.0000000f, -354.7683105f}};
			case 145:
				return {{1000.0000000f, -356.8011475f}};
			case 146:
				return {{1000.0000000f, -356.9757080f}};
			case 147:
				return {{1000.0000000f, -358.6204834f}};
			case 148:
				return {{1000.0000000f, -358.7729492f}};
			case 149:
				return {{1000.0000000f, -360.5307617f}};
			case 150:
				return {{1000.0000000f, -360.4355469f}};
			case 151:
				return {{1000.0000000f, -361.8718262f}};
			case 152:
				return {{1000.0000000f, -361.7562256f}};
			case 153:
				return {{1000.0000000f, -363.0778809f}};
			case 154:
				return {{1000.0000000f, -363.1750488f}};
			case 155:
				return {{1000.0000000f, -364.0666504f}};
			case 156:
				return {{1000.0000000f, -364.1520996f}};
			case 157:
				return {{1000.0000000f, -364.8118896f}};
			case 158:
				return {{1000.0000000f, -365.1131592f}};
			case 159:
				return {{1000.0000000f, -365.6785889f}};
			case 160:
				return {{1000.0000000f, -365.7379150f}};
			case 161:
				return {{1000.0000000f, -366.8975830f}};
			case 162:
				return {{1000.0000000f, -367.1383057f}};
			case 163:
				return {{1000.0000000f, -367.1704102f}};
			case 164:
				return {{1000.0000000f, -367.5058594f}};
			case 165:
				return {{1000.0000000f, -367.5330811f}};
			case 166:
				return {{1000.0000000f, -367.8172607f}};
			case 167:
				return {{1000.0000000f, -367.8402100f}};
			case 168:
				return {{1000.0000000f, -368.0611572f}};
			case 169:
				return {{1000.0000000f, -368.0804443f}};
			case 170:
				return {{1000.0000000f, -368.2398682f}};
			case 171:
				return {{1000.0000000f, -368.2490234f}};
			case 172:
				return {{1000.0000000f, -368.3774414f}};
			case 173:
				return {{1000.0000000f, -368.1584473f}};
			case 174:
				return {{1000.0000000f, -368.4852295f}};
			case 175:
				return {{1000.0000000f, -368.4898682f}};
			case 176:
				return {{1000.0000000f, -368.5522461f}};
			case 177:
				return {{1000.0000000f, -368.5556641f}};
			case 178:
				return {{1000.0000000f, -368.6008301f}};
			case 179:
				return {{1000.0000000f, -368.6030273f}};
			case 180:
				return {{1000.0000000f, -368.6293945f}};
			case 181:
				return {{1000.0000000f, -368.6307373f}};
			case 182:
				return {{1000.0000000f, -368.6463623f}};
			case 183:
				return {{1000.0000000f, -368.6523438f}};
			case 184:
				return {{1000.0000000f, -368.6516113f}};
			case 185:
				return {{1000.0000000f, -368.6513672f}};
			case 186:
				return {{1000.0000000f, -368.6455078f}};
			case 187:
				return {{1000.0000000f, -368.6450195f}};
			case 188:
				return {{1000.0000000f, -368.6356201f}};
			case 189:
				return {{1000.0000000f, -368.6350098f}};
			case 190:
				return {{1000.0000000f, -368.6240234f}};
			case 191:
				return {{1000.0000000f, -368.6234131f}};
			case 192:
				return {{1000.0000000f, -368.6105957f}};
			case 193:
				return {{1000.0000000f, -368.6097412f}};
			case 194:
				return {{1000.0000000f, -368.5974121f}};
			case 195:
				return {{1000.0000000f, -368.5965576f}};
			case 196:
				return {{1000.0000000f, -368.5834961f}};
			case 197:
				return {{1000.0000000f, -368.5827637f}};
			case 198:
				return {{1000.0000000f, -368.5701904f}};
			case 199:
				return {{1000.0000000f, -368.5695801f}};
			case 200:
				return {{1000.0000000f, -368.5584717f}};
			case 201:
				return {{1000.0000000f, -368.5578613f}};
			case 202:
				return {{1000.0000000f, -368.5469971f}};
			case 203:
				return {{1000.0000000f, -368.5463867f}};
			case 204:
				return {{1000.0000000f, -368.5355225f}};
			case 205:
				return {{1000.0000000f, -368.5347900f}};
			case 206:
				return {{1000.0000000f, -368.5025635f}};
			case 207:
				return {{1000.0000000f, -368.4810791f}};
			case 208:
				return {{1000.0000000f, -368.4796143f}};
			case 209:
				return {{1000.0000000f, -368.4780273f}};
			case 210:
				return {{1000.0000000f, -368.4475098f}};
			case 211:
				return {{1000.0000000f, -368.4453125f}};
			case 212:
				return {{1000.0000000f, -368.4040527f}};
			case 213:
				return {{1000.0000000f, -368.4011230f}};
			case 214:
				return {{1000.0000000f, -368.3516846f}};
			case 215:
				return {{1000.0000000f, -368.3480225f}};
			case 216:
				return {{1000.0000000f, -368.2833252f}};
			case 217:
				return {{1000.0000000f, -368.2788086f}};
			case 218:
				return {{1000.0000000f, -368.2016602f}};
			case 219:
				return {{1000.0000000f, -368.1965332f}};
			case 220:
				return {{1000.0000000f, -368.1130371f}};
			case 221:
				return {{1000.0000000f, -368.1072998f}};
			case 222:
				return {{1000.0000000f, -368.0146484f}};
			case 223:
				return {{1000.0000000f, -368.0089111f}};
			case 224:
				return {{1000.0000000f, -367.9161377f}};
			case 225:
				return {{1000.0000000f, -367.9093018f}};
			case 226:
				return {{1000.0000000f, -367.7950439f}};
			case 227:
				return {{1000.0000000f, -367.7873535f}};
			case 228:
				return {{1000.0000000f, -367.6586914f}};
			case 229:
				return {{1000.0000000f, -367.6502686f}};
			case 230:
				return {{1000.0000000f, -367.3686523f}};
			case 231:
				return {{1000.0000000f, -367.2152100f}};
			case 232:
				return {{1000.0000000f, -367.2055664f}};
			case 233:
				return {{1000.0000000f, -367.1960449f}};
			case 234:
				return {{1000.0000000f, -367.0494385f}};
			case 235:
				return {{1000.0000000f, -366.8879395f}};
			case 236:
				return {{1000.0000000f, -366.8778076f}};
			case 237:
				return {{1000.0000000f, -366.7222900f}};
			case 238:
				return {{1000.0000000f, -366.7117920f}};
			case 239:
				return {{1000.0000000f, -366.5399170f}};
			case 240:
				return {{1000.0000000f, -366.5289307f}};
			case 241:
				return {{1000.0000000f, -366.3500977f}};
			case 242:
				return {{1000.0000000f, -366.3387451f}};
			case 243:
				return {{1000.0000000f, -366.1640625f}};
			case 244:
				return {{1000.0000000f, -366.1522217f}};
			case 245:
				return {{1000.0000000f, -365.9576416f}};
			case 246:
				return {{1000.0000000f, -365.9451904f}};
			case 247:
				return {{1000.0000000f, -365.7416992f}};
			case 248:
				return {{1000.0000000f, -365.7287598f}};
			case 249:
				return {{1000.0000000f, -365.5297852f}};
			case 250:
				return {{1000.0000000f, -365.5162354f}};
			case 251:
				return {{1000.0000000f, -365.2945557f}};
			case 252:
				return {{1000.0000000f, -365.2803955f}};
			case 253:
				return {{1000.0000000f, -365.2662354f}};
			case 254:
				return {{1000.0000000f, -364.8010254f}};
			case 255:
				return {{1000.0000000f, -364.5526123f}};
			case 256:
				return {{1000.0000000f, -364.5371094f}};
			case 257:
				return {{1000.0000000f, -364.3033447f}};
			case 258:
				return {{1000.0000000f, -364.2877197f}};
			case 259:
				return {{1000.0000000f, -364.2720947f}};
			case 260:
				return {{1000.0000000f, -364.0151367f}};
			case 261:
				return {{1000.0000000f, -363.9987793f}};
			case 262:
				return {{1000.0000000f, -363.7299805f}};
			case 263:
				return {{1000.0000000f, -363.7128906f}};
			case 264:
				return {{1000.0000000f, -363.4495850f}};
			case 265:
				return {{1000.0000000f, -363.1593018f}};
			case 266:
				return {{1000.0000000f, -363.1409912f}};
			case 267:
				return {{1000.0000000f, -362.8608398f}};
			case 268:
				return {{1000.0000000f, -362.8419189f}};
			case 269:
				return {{1000.0000000f, -362.5319824f}};
			case 270:
				return {{1000.0000000f, -362.5122070f}};
			case 271:
				return {{1000.0000000f, -362.1889648f}};
			case 272:
				return {{1000.0000000f, -362.1683350f}};
			case 273:
				return {{1000.0000000f, -361.8519287f}};
			case 274:
				return {{1000.0000000f, -361.8304443f}};
			case 275:
				return {{1000.0000000f, -361.8088379f}};
			case 276:
				return {{1000.0000000f, -361.4498291f}};
			case 277:
				return {{1000.0000000f, -361.4266357f}};
			case 278:
				return {{1000.0000000f, -361.4033203f}};
			case 279:
				return {{1000.0000000f, -361.0133057f}};
			case 280:
				return {{1000.0000000f, -360.6243896f}};
			case 281:
				return {{1000.0000000f, -360.1939697f}};
			case 282:
				return {{1000.0000000f, -360.1667480f}};
			case 283:
				return {{1000.0000000f, -360.1394043f}};
			case 284:
				return {{1000.0000000f, -359.7172852f}};
			case 285:
				return {{1000.0000000f, -359.6884766f}};
			case 286:
				return {{1000.0000000f, -359.6595459f}};
			case 287:
				return {{1000.0000000f, -359.1754150f}};
			case 288:
				return {{1000.0000000f, -359.1440430f}};
			case 289:
				return {{1000.0000000f, -359.1125488f}};
			case 290:
				return {{1000.0000000f, -358.5827637f}};
			case 291:
				return {{1000.0000000f, -358.5484619f}};
			case 292:
				return {{1000.0000000f, -358.5140381f}};
			case 293:
				return {{1000.0000000f, -357.9708252f}};
			case 294:
				return {{1000.0000000f, -357.9329834f}};
			case 295:
				return {{1000.0000000f, -357.2990723f}};
			case 296:
				return {{1000.0000000f, -357.2579346f}};
			case 297:
				return {{1000.0000000f, -357.2165527f}};
			case 298:
				return {{1000.0000000f, -356.5202637f}};
			case 299:
				return {{1000.0000000f, -356.4748535f}};
			case 300:
				return {{1000.0000000f, -356.4291992f}};
			case 301:
				return {{1000.0000000f, -355.7089844f}};
			case 302:
				return {{1000.0000000f, -355.6590576f}};
			case 303:
				return {{1000.0000000f, -354.8859863f}};
			case 304:
				return {{1000.0000000f, -354.8392334f}};
			case 305:
				return {{1000.0000000f, -354.7927246f}};
			case 306:
				return {{1000.0000000f, -354.1160889f}};
			case 307:
				return {{1000.0000000f, -354.0723877f}};
			case 308:
				return {{1000.0000000f, -353.3978271f}};
			case 309:
				return {{1000.0000000f, -352.7675781f}};
			case 310:
				return {{1000.0000000f, -352.7296143f}};
			case 311:
				return {{1000.0000000f, -352.1800537f}};
			case 312:
				return {{1000.0000000f, -352.1447754f}};
			case 313:
				return {{1000.0000000f, -352.1096191f}};
			case 314:
				return {{1000.0000000f, -351.5682373f}};
			case 315:
				return {{1000.0000000f, -351.5357666f}};
			case 316:
				return {{1000.0000000f, -351.5034180f}};
			case 317:
				return {{1000.0000000f, -351.0059814f}};
			case 318:
				return {{1000.0000000f, -350.9761963f}};
			case 319:
				return {{1000.0000000f, -350.5456543f}};
			case 320:
				return {{1000.0000000f, -350.5180664f}};
			case 321:
				return {{1000.0000000f, -350.4906006f}};
			case 322:
				return {{1000.0000000f, -350.0698242f}};
			case 323:
				return {{1000.0000000f, -350.0446777f}};
			case 324:
				return {{1000.0000000f, -350.0196533f}};
			case 325:
				return {{1000.0000000f, -349.6591797f}};
			case 326:
				return {{1000.0000000f, -349.6361084f}};
			case 327:
				return {{1000.0000000f, -349.6131592f}};
			case 328:
				return {{1000.0000000f, -349.2626953f}};
			case 329:
				return {{1000.0000000f, -349.2418213f}};
			case 330:
				return {{1000.0000000f, -349.2210693f}};
			case 331:
				return {{1000.0000000f, -348.9033203f}};
			case 332:
				return {{1000.0000000f, -348.8843994f}};
			case 333:
				return {{1000.0000000f, -348.8656006f}};
			case 334:
				return {{1000.0000000f, -348.5954590f}};
			case 335:
				return {{1000.0000000f, -348.5782471f}};
			case 336:
				return {{1000.0000000f, -348.5611572f}};
			case 337:
				return {{1000.0000000f, -348.3006592f}};
			case 338:
				return {{1000.0000000f, -348.2851562f}};
			case 339:
				return {{1000.0000000f, -348.2697754f}};
			case 340:
				return {{1000.0000000f, -348.0356445f}};
			case 341:
				return {{1000.0000000f, -348.0217285f}};
			case 342:
				return {{1000.0000000f, -347.8232422f}};
			case 343:
				return {{1000.0000000f, -347.6309814f}};
			case 344:
				return {{1000.0000000f, -347.4677734f}};
			case 345:
				return {{1000.0000000f, -347.4573975f}};
			case 346:
				return {{1000.0000000f, -347.4471436f}};
			case 347:
				return {{1000.0000000f, -347.2919922f}};
			case 348:
				return {{1000.0000000f, -347.2828369f}};
			case 349:
				return {{1000.0000000f, -347.1445312f}};
			case 350:
				return {{1000.0000000f, -347.1363525f}};
			case 351:
				return {{1000.0000000f, -347.1282959f}};
			case 352:
				return {{1000.0000000f, -347.0135498f}};
			case 353:
				return {{1000.0000000f, -347.0063477f}};
			case 354:
				return {{1000.0000000f, -346.8975830f}};
			case 355:
				return {{1000.0000000f, -346.8912354f}};
			case 356:
				return {{1000.0000000f, -346.8848877f}};
			case 357:
				return {{1000.0000000f, -346.7896729f}};
			case 358:
				return {{1000.0000000f, -346.7840576f}};
			case 359:
				return {{1000.0000000f, -346.7054443f}};
			case 360:
				return {{1000.0000000f, -346.7005615f}};
			case 361:
				return {{1000.0000000f, -346.6956787f}};
			case 362:
				return {{1000.0000000f, -346.6226807f}};
			case 363:
				return {{1000.0000000f, -346.6184082f}};
			case 364:
				return {{1000.0000000f, -346.6141357f}};
			case 365:
				return {{1000.0000000f, -346.6099854f}};
			case 366:
				return {{1000.0000000f, -346.5512695f}};
			case 367:
				return {{1000.0000000f, -346.5476074f}};
			case 368:
				return {{1000.0000000f, -346.4932861f}};
			case 369:
				return {{1000.0000000f, -346.4901123f}};
			case 370:
				return {{1000.0000000f, -346.4434814f}};
			case 371:
				return {{1000.0000000f, -346.4407959f}};
			case 372:
				return {{1000.0000000f, -346.4381104f}};
			case 373:
				return {{1000.0000000f, -346.4010010f}};
			case 374:
				return {{1000.0000000f, -346.3670654f}};
			case 375:
				return {{1000.0000000f, -346.3383789f}};
			case 376:
				return {{1000.0000000f, -346.3367920f}};
			case 377:
				return {{1000.0000000f, -346.3352051f}};
			case 378:
				return {{1000.0000000f, -346.3131104f}};
			case 379:
				return {{1000.0000000f, -346.3117676f}};
			case 380:
				return {{1000.0000000f, -346.3104248f}};
			case 381:
				return {{1000.0000000f, -346.2913818f}};
			case 382:
				return {{1000.0000000f, -346.2902832f}};
			case 383:
				return {{1000.0000000f, -346.2891846f}};
			case 384:
				return {{1000.0000000f, -346.2750244f}};
			case 385:
				return {{1000.0000000f, -346.2741699f}};
			case 386:
				return {{1000.0000000f, -346.2733154f}};
			case 387:
				return {{1000.0000000f, -346.2615967f}};
			case 388:
				return {{1000.0000000f, -346.2609863f}};
			case 389:
				return {{1000.0000000f, -346.2521973f}};
			case 390:
				return {{1000.0000000f, -346.2517090f}};
			case 391:
				return {{1000.0000000f, -346.2512207f}};
			case 392:
				return {{1000.0000000f, -346.2454834f}};
			case 393:
				return {{1000.0000000f, -346.2451172f}};
			case 394:
				return {{1000.0000000f, -346.2447510f}};
			case 395:
				return {{1000.0000000f, -346.2408447f}};
			case 396:
				return {{1000.0000000f, -346.2406006f}};
			case 397:
				return {{1000.0000000f, -346.2404785f}};
			case 398:
				return {{1000.0000000f, -346.2385254f}};
			case 399:
				return {{1000.0000000f, -346.2384033f}};
			case 400:
				return {{1000.0000000f, -346.2380371f}};
			case 401:
				return {{1000.0000000f, -346.2380371f}};
			case 402:
				return {{1000.0000000f, -346.2380371f}};
			case 403:
				return {{1000.0000000f, -346.2390137f}};
			case 404:
				return {{1000.0000000f, -346.2391357f}};
			case 405:
				return {{1000.0000000f, -346.2392578f}};
			case 406:
				return {{1000.0000000f, -346.2412109f}};
			case 407:
				return {{1000.0000000f, -346.2441406f}};
			case 408:
				return {{1000.0000000f, -346.2478027f}};
			case 409:
				return {{1000.0000000f, -346.2480469f}};
			case 410:
				return {{1000.0000000f, -346.2482910f}};
			case 411:
				return {{1000.0000000f, -346.2523193f}};
			case 412:
				return {{1000.0000000f, -346.2525635f}};
			case 413:
				return {{1000.0000000f, -346.2528076f}};
			case 414:
				return {{1000.0000000f, -346.2575684f}};
			case 415:
				return {{1000.0000000f, -346.2579346f}};
			case 416:
				return {{1000.0000000f, -346.2583008f}};
			case 417:
				return {{1000.0000000f, -346.2634277f}};
			case 418:
				return {{1000.0000000f, -346.2637939f}};
			case 419:
				return {{1000.0000000f, -346.2641602f}};
			case 420:
				return {{1000.0000000f, -346.2691650f}};
			case 421:
				return {{1000.0000000f, -346.2695312f}};
			case 422:
				return {{1000.0000000f, -346.2750244f}};
			case 423:
				return {{1000.0000000f, -346.2753906f}};
			case 424:
				return {{1000.0000000f, -346.2757568f}};
			case 425:
				return {{1000.0000000f, -346.2810059f}};
			case 426:
				return {{1000.0000000f, -346.2813721f}};
			case 427:
				return {{1000.0000000f, -346.2817383f}};
			case 428:
				return {{1000.0000000f, -346.2873535f}};
			case 429:
				return {{1000.0000000f, -346.2877197f}};
			case 430:
				return {{1000.0000000f, -346.2880859f}};
			case 431:
				return {{1000.0000000f, -346.2937012f}};
			case 432:
				return {{1000.0000000f, -346.2940674f}};
			case 433:
				return {{1000.0000000f, -346.2944336f}};
			case 434:
				return {{1000.0000000f, -346.2995605f}};
			case 435:
				return {{1000.0000000f, -346.2999268f}};
			case 436:
				return {{1000.0000000f, -346.3002930f}};
			case 437:
				return {{1000.0000000f, -346.3056641f}};
			case 438:
				return {{1000.0000000f, -346.3060303f}};
			case 439:
				return {{1000.0000000f, -346.3112793f}};
			case 440:
				return {{1000.0000000f, -346.3160400f}};
			case 441:
				return {{1000.0000000f, -346.3209229f}};
			case 442:
				return {{1000.0000000f, -346.3211670f}};
			case 443:
				return {{1000.0000000f, -346.3214111f}};
			case 444:
				return {{1000.0000000f, -346.3216553f}};
			case 445:
				return {{1000.0000000f, -346.3260498f}};
			case 446:
				return {{1000.0000000f, -346.3262939f}};
			case 447:
				return {{1000.0000000f, -346.3306885f}};
			case 448:
				return {{1000.0000000f, -346.3309326f}};
			case 449:
				return {{1000.0000000f, -346.3311768f}};
			case 450:
				return {{1000.0000000f, -346.3353271f}};
			case 451:
				return {{1000.0000000f, -346.3355713f}};
			case 452:
				return {{1000.0000000f, -346.3393555f}};
			case 453:
				return {{1000.0000000f, -346.3395996f}};
			case 454:
				return {{1000.0000000f, -346.3398438f}};
			case 455:
				return {{1000.0000000f, -346.3436279f}};
			case 456:
				return {{1000.0000000f, -346.3438721f}};
			case 457:
				return {{1000.0000000f, -346.3474121f}};
			case 458:
				return {{1000.0000000f, -346.3476562f}};
			case 459:
				return {{1000.0000000f, -346.3479004f}};
			case 460:
				return {{1000.0000000f, -346.3509521f}};
			case 461:
				return {{1000.0000000f, -346.3511963f}};
			case 462:
				return {{1000.0000000f, -346.3514404f}};
			case 463:
				return {{1000.0000000f, -346.3516846f}};
			case 464:
				return {{1000.0000000f, -346.3547363f}};
			case 465:
				return {{1000.0000000f, -346.3549805f}};
			case 466:
				return {{1000.0000000f, -346.3552246f}};
			case 467:
				return {{1000.0000000f, -346.3579102f}};
			case 468:
				return {{1000.0000000f, -346.3580322f}};
			case 469:
				return {{1000.0000000f, -346.3607178f}};
			case 470:
				return {{1000.0000000f, -346.3631592f}};
			case 471:
				return {{1000.0000000f, -346.3653564f}};
			case 472:
				return {{1000.0000000f, -346.3654785f}};
			case 473:
				return {{1000.0000000f, -346.3656006f}};
			case 474:
				return {{1000.0000000f, -346.3657227f}};
			case 475:
				return {{1000.0000000f, -346.3677979f}};
			case 476:
				return {{1000.0000000f, -346.3679199f}};
			case 477:
				return {{1000.0000000f, -346.3698730f}};
			case 478:
				return {{1000.0000000f, -346.3699951f}};
			case 479:
				return {{1000.0000000f, -346.3701172f}};
			case 480:
				return {{1000.0000000f, -346.3718262f}};
			case 481:
				return {{1000.0000000f, -346.3719482f}};
			case 482:
				return {{1000.0000000f, -346.3720703f}};
			case 483:
				return {{1000.0000000f, -346.3736572f}};
			case 484:
				return {{1000.0000000f, -346.3737793f}};
			case 485:
				return {{1000.0000000f, -346.3739014f}};
			case 486:
				return {{1000.0000000f, -346.3752441f}};
			case 487:
				return {{1000.0000000f, -346.3753662f}};
			case 488:
				return {{1000.0000000f, -346.3767090f}};
			case 489:
				return {{1000.0000000f, -346.3768311f}};
			case 490:
				return {{1000.0000000f, -346.3769531f}};
			case 491:
				return {{1000.0000000f, -346.3770752f}};
			case 492:
				return {{1000.0000000f, -346.3782959f}};
			case 493:
				return {{1000.0000000f, -346.3784180f}};
			case 494:
				return {{1000.0000000f, -346.3793945f}};
			case 495:
				return {{1000.0000000f, -346.3795166f}};
			case 496:
				return {{1000.0000000f, -346.3796387f}};
			case 497:
				return {{1000.0000000f, -346.3797607f}};
			case 498:
				return {{1000.0000000f, -346.3807373f}};
			case 499:
				return {{1000.0000000f, -346.3807373f}};
			case 500:
				return {{1000.0000000f, -346.3815918f}};
			case 501:
				return {{1000.0000000f, -346.3815918f}};
			case 502:
				return {{1000.0000000f, -346.3815918f}};
			case 503:
				return {{1000.0000000f, -346.3913574f}};
			case 504:
				return {{1000.0000000f, -346.4881592f}};
			case 505:
				return {{1000.0000000f, -346.4929199f}};
			case 506:
				return {{1000.0000000f, -346.4979248f}};
			case 507:
				return {{1000.0000000f, -346.5031738f}};
			case 508:
				return {{1000.0000000f, -346.6301270f}};
			case 509:
				return {{1000.0000000f, -346.6406250f}};
			case 510:
				return {{1000.0000000f, -346.8524170f}};
			case 511:
				return {{1000.0000000f, -346.8681641f}};
			case 512:
				return {{1000.0000000f, -347.1433105f}};
			case 513:
				return {{1000.0000000f, -347.1638184f}};
			case 514:
				return {{1000.0000000f, -347.1844482f}};
			case 515:
				return {{1000.0000000f, -347.2054443f}};
			case 516:
				return {{1000.0000000f, -347.5948486f}};
			case 517:
				return {{1000.0000000f, -347.6221924f}};
			case 518:
				return {{1000.0000000f, -348.1022949f}};
			case 519:
				return {{1000.0000000f, -348.1346436f}};
			case 520:
				return {{1000.0000000f, -348.1672363f}};
			case 521:
				return {{1000.0000000f, -348.6911621f}};
			case 522:
				return {{1000.0000000f, -348.7281494f}};
			case 523:
				return {{1000.0000000f, -348.7653809f}};
			case 524:
				return {{1000.0000000f, -349.3939209f}};
			case 525:
				return {{1000.0000000f, -349.4350586f}};
			case 526:
				return {{1000.0000000f, -349.4763184f}};
			case 527:
				return {{1000.0000000f, -350.1192627f}};
			case 528:
				return {{1000.0000000f, -350.1634521f}};
			case 529:
				return {{1000.0000000f, -350.2077637f}};
			case 530:
				return {{1000.0000000f, -350.9383545f}};
			case 531:
				return {{1000.0000000f, -350.9851074f}};
			case 532:
				return {{1000.0000000f, -351.0318604f}};
			case 533:
				return {{1000.0000000f, -351.7949219f}};
			case 534:
				return {{1000.0000000f, -351.8432617f}};
			case 535:
				return {{1000.0000000f, -351.8916016f}};
			case 536:
				return {{1000.0000000f, -353.4113770f}};
			case 537:
				return {{1000.0000000f, -353.4603271f}};
			case 538:
				return {{1000.0000000f, -354.2315674f}};
			case 539:
				return {{1000.0000000f, -354.2789307f}};
			case 540:
				return {{1000.0000000f, -354.3261719f}};
			case 541:
				return {{1000.0000000f, -355.0251465f}};
			case 542:
				return {{1000.0000000f, -355.0710449f}};
			case 543:
				return {{1000.0000000f, -355.1168213f}};
			case 544:
				return {{1000.0000000f, -355.8386230f}};
			case 545:
				return {{1000.0000000f, -355.8829346f}};
			case 546:
				return {{1000.0000000f, -355.9272461f}};
			case 547:
				return {{1000.0000000f, -356.5821533f}};
			case 548:
				return {{1000.0000000f, -356.6251221f}};
			case 549:
				return {{1000.0000000f, -356.6679688f}};
			case 550:
				return {{1000.0000000f, -357.3436279f}};
			case 551:
				return {{1000.0000000f, -357.3851318f}};
			case 552:
				return {{1000.0000000f, -358.0360107f}};
			case 553:
				return {{1000.0000000f, -358.0758057f}};
			case 554:
				return {{1000.0000000f, -358.6597900f}};
			case 555:
				return {{1000.0000000f, -358.6977539f}};
			case 556:
				return {{1000.0000000f, -358.7357178f}};
			case 557:
				return {{1000.0000000f, -359.3291016f}};
			case 558:
				return {{1000.0000000f, -359.3653564f}};
			case 559:
				return {{1000.0000000f, -359.4014893f}};
			case 560:
				return {{1000.0000000f, -359.9670410f}};
			case 561:
				return {{1000.0000000f, -360.0015869f}};
			case 562:
				return {{1000.0000000f, -360.0360107f}};
			case 563:
				return {{1000.0000000f, -360.5422363f}};
			case 564:
				return {{1000.0000000f, -360.5753174f}};
			case 565:
				return {{1000.0000000f, -360.6082764f}};
			case 566:
				return {{1000.0000000f, -361.1246338f}};
			case 567:
				return {{1000.0000000f, -361.1562500f}};
			case 568:
				return {{1000.0000000f, -361.6190186f}};
			case 569:
				return {{1000.0000000f, -362.0883789f}};
			case 570:
				return {{1000.0000000f, -362.5291748f}};
			case 571:
				return {{1000.0000000f, -362.5556641f}};
			case 572:
				return {{1000.0000000f, -362.5820312f}};
			case 573:
				return {{1000.0000000f, -362.9669189f}};
			case 574:
				return {{1000.0000000f, -362.9918213f}};
			case 575:
				return {{1000.0000000f, -363.0166016f}};
			case 576:
				return {{1000.0000000f, -363.3950195f}};
			case 577:
				return {{1000.0000000f, -363.4176025f}};
			case 578:
				return {{1000.0000000f, -363.4400635f}};
			case 579:
				return {{1000.0000000f, -363.7840576f}};
			case 580:
				return {{1000.0000000f, -363.8045654f}};
			case 581:
				return {{1000.0000000f, -363.8249512f}};
			case 582:
				return {{1000.0000000f, -364.1297607f}};
			case 583:
				return {{1000.0000000f, -364.1499023f}};
			case 584:
				return {{1000.0000000f, -364.4687500f}};
			case 585:
				return {{1000.0000000f, -364.4884033f}};
			case 586:
				return {{1000.0000000f, -364.5080566f}};
			case 587:
				return {{1000.0000000f, -364.7995605f}};
			case 588:
				return {{1000.0000000f, -364.8187256f}};
			case 589:
				return {{1000.0000000f, -364.8378906f}};
			case 590:
				return {{1000.0000000f, -365.1412354f}};
			case 591:
				return {{1000.0000000f, -365.1599121f}};
			case 592:
				return {{1000.0000000f, -365.1785889f}};
			case 593:
				return {{1000.0000000f, -365.4741211f}};
			case 594:
				return {{1000.0000000f, -365.4923096f}};
			case 595:
				return {{1000.0000000f, -365.5104980f}};
			case 596:
				return {{1000.0000000f, -365.7802734f}};
			case 597:
				return {{1000.0000000f, -365.7979736f}};
			case 598:
				return {{1000.0000000f, -365.8156738f}};
			case 599:
				return {{1000.0000000f, -366.0958252f}};
			case 600:
				return {{1000.0000000f, -366.1130371f}};
			case 601:
				return {{1000.0000000f, -366.6300049f}};
			case 602:
				return {{1000.0000000f, -366.6459961f}};
			case 603:
				return {{1000.0000000f, -366.8927002f}};
			case 604:
				return {{1000.0000000f, -366.9075928f}};
			case 605:
				return {{1000.0000000f, -367.1212158f}};
			case 606:
				return {{1000.0000000f, -367.1348877f}};
			case 607:
				return {{1000.0000000f, -367.1484375f}};
			case 608:
				return {{1000.0000000f, -367.3541260f}};
			case 609:
				return {{1000.0000000f, -367.3663330f}};
			case 610:
				return {{1000.0000000f, -367.5498047f}};
			case 611:
				return {{1000.0000000f, -367.5605469f}};
			case 612:
				return {{1000.0000000f, -367.7126465f}};
			case 613:
				return {{1000.0000000f, -367.7221680f}};
			case 614:
				return {{1000.0000000f, -367.8640137f}};
			case 615:
				return {{1000.0000000f, -367.8721924f}};
			case 616:
				return {{1000.0000000f, -367.8803711f}};
			case 617:
				return {{1000.0000000f, -368.0013428f}};
			case 618:
				return {{1000.0000000f, -368.0083008f}};
			case 619:
				return {{1000.0000000f, -368.0152588f}};
			case 620:
				return {{1000.0000000f, -368.1114502f}};
			case 621:
				return {{1000.0000000f, -368.1173096f}};
			case 622:
				return {{1000.0000000f, -368.2042236f}};
			case 623:
				return {{1000.0000000f, -368.2092285f}};
			case 624:
				return {{1000.0000000f, -368.2141113f}};
			case 625:
				return {{1000.0000000f, -368.2817383f}};
			case 626:
				return {{1000.0000000f, -368.2858887f}};
			case 627:
				return {{1000.0000000f, -368.2899170f}};
			case 628:
				return {{1000.0000000f, -368.3979492f}};
			case 629:
				return {{1000.0000000f, -368.4006348f}};
			case 630:
				return {{1000.0000000f, -368.4378662f}};
			case 631:
				return {{1000.0000000f, -368.4400635f}};
			case 632:
				return {{1000.0000000f, -368.4422607f}};
			case 633:
				return {{1000.0000000f, -368.4737549f}};
			case 634:
				return {{1000.0000000f, -368.4754639f}};
			case 635:
				return {{1000.0000000f, -368.5002441f}};
			case 636:
				return {{1000.0000000f, -368.5015869f}};
			case 637:
				return {{1000.0000000f, -368.5029297f}};
			case 638:
				return {{1000.0000000f, -368.5208740f}};
			case 639:
				return {{1000.0000000f, -368.5219727f}};
			case 640:
				return {{1000.0000000f, -368.5229492f}};
			case 641:
				return {{1000.0000000f, -368.5374756f}};
			case 642:
				return {{1000.0000000f, -368.5382080f}};
			case 643:
				return {{1000.0000000f, -368.5484619f}};
			case 644:
				return {{1000.0000000f, -368.5490723f}};
			case 645:
				return {{1000.0000000f, -368.5496826f}};
			case 646:
				return {{1000.0000000f, -368.5574951f}};
			case 647:
				return {{1000.0000000f, -368.5578613f}};
			case 648:
				return {{1000.0000000f, -368.5582275f}};
			case 649:
				return {{1000.0000000f, -368.5635986f}};
			case 650:
				return {{1000.0000000f, -368.5638428f}};
			case 651:
				return {{1000.0000000f, -368.5671387f}};
			case 652:
				return {{1000.0000000f, -368.5672607f}};
			case 653:
				return {{1000.0000000f, -368.5673828f}};
			case 654:
				return {{1000.0000000f, -368.5694580f}};
			case 655:
				return {{1000.0000000f, -368.5695801f}};
			case 656:
				return {{1000.0000000f, -368.5705566f}};
			case 657:
				return {{1000.0000000f, -368.5705566f}};
			case 658:
				return {{1000.0000000f, -368.5708008f}};
			case 659:
				return {{1000.0000000f, -368.5705566f}};
			case 660:
				return {{1000.0000000f, -368.5699463f}};
			case 661:
				return {{1000.0000000f, -368.5699463f}};
			case 662:
				return {{1000.0000000f, -368.5699463f}};
			case 663:
				return {{1000.0000000f, -368.5690918f}};
			case 664:
				return {{1000.0000000f, -368.5689697f}};
			case 665:
				return {{1000.0000000f, -368.5678711f}};
			case 666:
				return {{1000.0000000f, -368.5677490f}};
			case 667:
				return {{1000.0000000f, -368.5676270f}};
			case 668:
				return {{1000.0000000f, -368.5665283f}};
			case 669:
				return {{1000.0000000f, -368.5664062f}};
			case 670:
				return {{1000.0000000f, -368.5662842f}};
			case 671:
				return {{1000.0000000f, -368.5651855f}};
			case 672:
				return {{1000.0000000f, -368.5650635f}};
			case 673:
				return {{1000.0000000f, -368.5639648f}};
			case 674:
				return {{1000.0000000f, -368.5638428f}};
			case 675:
				return {{1000.0000000f, -368.5637207f}};
			case 676:
				return {{1000.0000000f, -368.5627441f}};
			case 677:
				return {{1000.0000000f, -368.5626221f}};
			case 678:
				return {{1000.0000000f, -368.5625000f}};
			case 679:
				return {{1000.0000000f, -368.5615234f}};
			case 680:
				return {{1000.0000000f, -368.5615234f}};
			case 681:
				return {{1000.0000000f, -368.5606689f}};
			case 682:
				return {{1000.0000000f, -368.5606689f}};
			case 683:
				return {{1000.0000000f, -368.5606689f}};
			case 684:
				return {{1000.0000000f, -368.5599365f}};
			case 685:
				return {{1000.0000000f, -368.5599365f}};
			case 686:
				return {{1000.0000000f, -368.5592041f}};
			case 687:
				return {{1000.0000000f, -368.5592041f}};
			case 688:
				return {{1000.0000000f, -368.5585938f}};
			case 689:
				return {{1000.0000000f, -368.5581055f}};
			case 690:
				return {{1000.0000000f, -368.5576172f}};
			case 691:
				return {{1000.0000000f, -368.5576172f}};
			case 692:
				return {{1000.0000000f, -368.5576172f}};
			case 693:
				return {{1000.0000000f, -368.5572510f}};
			case 694:
				return {{1000.0000000f, -368.5572510f}};
			case 695:
				return {{1000.0000000f, -368.5568848f}};
			case 696:
				return {{1000.0000000f, -368.5568848f}};
			case 697:
				return {{1000.0000000f, -368.5568848f}};
			case 698:
				return {{1000.0000000f, -368.5566406f}};
			case 699:
				return {{1000.0000000f, -368.5566406f}};
			case 700:
				return {{1000.0000000f, -368.5563965f}};
			case 701:
				return {{1000.0000000f, -368.5563965f}};
			case 702:
				return {{1000.0000000f, -368.5562744f}};
			case 703:
				return {{1000.0000000f, -368.5562744f}};
			case 704:
				return {{1000.0000000f, -368.5562744f}};
			case 705:
				return {{1000.0000000f, -368.5561523f}};
			case 706:
				return {{1000.0000000f, -368.5561523f}};
			case 707:
				return {{1000.0000000f, -368.5561523f}};
			case 708:
				return {{1000.0000000f, -368.5560303f}};
			case 709:
				return {{1000.0000000f, -368.5560303f}};
			case 710:
				return {{1000.0000000f, -368.5559082f}};
			case 711:
				return {{1000.0000000f, -368.5559082f}};
			case 712:
				return {{1000.0000000f, -368.5559082f}};
			case 713:
				return {{1000.0000000f, -368.5557861f}};
			case 714:
				return {{1000.0000000f, -368.5557861f}};
			case 715:
				return {{1000.0000000f, -368.5557861f}};
			case 716:
				return {{1000.0000000f, -368.5557861f}};
			case 717:
				return {{1000.0000000f, -368.5557861f}};
			case 718:
				return {{1000.0000000f, -368.5557861f}};
			case 719:
				return {{1000.0000000f, -368.5557861f}};
			case 720:
				return {{1000.0000000f, -368.5557861f}};
			case 721:
				return {{1000.0000000f, -368.5557861f}};
			case 722:
				return {{1000.0000000f, -368.5557861f}};
			case 723:
				return {{1000.0000000f, -368.5557861f}};
			case 724:
				return {{1000.0000000f, -368.5557861f}};
			case 725:
				return {{1000.0000000f, -368.5557861f}};
			case 726:
				return {{1000.0000000f, -368.5557861f}};
			case 727:
				return {{1000.0000000f, -368.5557861f}};
			case 728:
				return {{1000.0000000f, -368.5557861f}};
			case 729:
				return {{1000.0000000f, -368.5557861f}};
			case 730:
				return {{1000.0000000f, -368.5557861f}};
			case 731:
				return {{1000.0000000f, -368.5557861f}};
			case 732:
				return {{1000.0000000f, -368.5557861f}};
			case 733:
				return {{1000.0000000f, -368.5557861f}};
			case 734:
				return {{1000.0000000f, -368.5557861f}};
			case 735:
				return {{1000.0000000f, -368.5557861f}};
			case 736:
				return {{1000.0000000f, -368.5557861f}};
			case 737:
				return {{1000.0000000f, -368.5557861f}};
			case 738:
				return {{1000.0000000f, -368.5557861f}};
			case 739:
				return {{1000.0000000f, -368.5557861f}};
			case 740:
				return {{1000.0000000f, -368.5557861f}};
			case 741:
				return {{1000.0000000f, -368.5557861f}};
			case 742:
				return {{1000.0000000f, -368.5557861f}};
			case 743:
				return {{1000.0000000f, -368.5557861f}};
			case 744:
				return {{1000.0000000f, -368.5557861f}};
			case 745:
				return {{1000.0000000f, -368.5557861f}};
			case 746:
				return {{1000.0000000f, -368.5557861f}};
			case 747:
				return {{1000.0000000f, -368.5557861f}};
			case 748:
				return {{1000.0000000f, -368.5557861f}};
			case 749:
				return {{1000.0000000f, -368.5557861f}};
			case 750:
				return {{1000.0000000f, -368.5557861f}};
			case 751:
				return {{1000.0000000f, -368.5557861f}};
			case 752:
				return {{1000.0000000f, -368.5557861f}};
			case 753:
				return {{1000.0000000f, -368.5557861f}};
			case 754:
				return {{1000.0000000f, -368.5557861f}};
			case 755:
				return {{1000.0000000f, -368.5557861f}};
			case 756:
				return {{1000.0000000f, -368.5557861f}};
			case 757:
				return {{1000.0000000f, -368.5557861f}};
			case 758:
				return {{1000.0000000f, -368.5557861f}};
			case 759:
				return {{1000.0000000f, -368.5557861f}};
			case 760:
				return {{1000.0000000f, -368.5557861f}};
			case 761:
				return {{1000.0000000f, -368.5557861f}};
			case 762:
				return {{1000.0000000f, -368.5557861f}};
			case 763:
				return {{1000.0000000f, -368.5557861f}};
			case 764:
				return {{1000.0000000f, -368.5557861f}};
			case 765:
				return {{1000.0000000f, -368.5557861f}};
			case 766:
				return {{1000.0000000f, -368.5557861f}};
			case 767:
				return {{1000.0000000f, -368.5557861f}};
			case 768:
				return {{1000.0000000f, -368.5557861f}};
			case 769:
				return {{1000.0000000f, -368.5557861f}};
			case 770:
				return {{1000.0000000f, -368.5557861f}};
			case 771:
				return {{1000.0000000f, -368.5557861f}};
			case 772:
				return {{1000.0000000f, -368.5557861f}};
			case 773:
				return {{1000.0000000f, -368.5557861f}};
			case 774:
				return {{1000.0000000f, -368.5557861f}};
			case 775:
				return {{1000.0000000f, -368.5557861f}};
			case 776:
				return {{1000.0000000f, -368.5557861f}};
			case 777:
				return {{1000.0000000f, -368.5557861f}};
			case 778:
				return {{1000.0000000f, -368.5557861f}};
			case 779:
				return {{1000.0000000f, -368.5557861f}};
			case 780:
				return {{1000.0000000f, -368.5557861f}};
			case 781:
				return {{1000.0000000f, -368.5557861f}};
			case 782:
				return {{1000.0000000f, -368.5557861f}};
			case 783:
				return {{1000.0000000f, -368.5557861f}};
			case 784:
				return {{1000.0000000f, -368.5557861f}};
			case 785:
				return {{1000.0000000f, -368.5557861f}};
			case 786:
				return {{1000.0000000f, -368.5557861f}};
			case 787:
				return {{1000.0000000f, -368.5557861f}};
			case 788:
				return {{1000.0000000f, -368.5557861f}};
			case 789:
				return {{1000.0000000f, -368.5557861f}};
			case 790:
				return {{1000.0000000f, -368.5557861f}};
			case 791:
				return {{1000.0000000f, -368.5557861f}};
			case 792:
				return {{1000.0000000f, -368.5557861f}};
			case 793:
				return {{1000.0000000f, -368.5557861f}};
			case 794:
				return {{1000.0000000f, -368.5557861f}};
			case 795:
				return {{1000.0000000f, -368.5557861f}};
			case 796:
				return {{1000.0000000f, -368.5557861f}};
			case 797:
				return {{1000.0000000f, -368.5557861f}};
			case 798:
				return {{1000.0000000f, -368.5557861f}};
			case 799:
				return {{1000.0000000f, -368.5557861f}};
			case 800:
				return {{1000.0000000f, -368.5557861f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == k_ScreenCentreLine[9])
		{
			switch (frameNumber)
			{
			case 0:
				return {{0.0000000f, 9.0254517f}};
			case 202:
				return {{1000.0000000f, 1075.0816650f}};
			case 602:
				return {{1000.0000000f, 1076.9206543f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 350))
		{
			switch (frameNumber)
			{
			case 301:
				return {{1000.0000000f, 1099.2722168f}};
			case 302:
				return {{1000.0000000f, 1099.3189697f}};
			case 303:
				return {{1000.0000000f, 1100.0920410f}};
			case 304:
				return {{1000.0000000f, 1100.1419678f}};
			case 305:
				return {{1000.0000000f, 1100.1884766f}};
			case 306:
				return {{1000.0000000f, 1100.8619385f}};
			case 307:
				return {{1000.0000000f, 1100.9056396f}};
			case 308:
				return {{1000.0000000f, 1101.5833740f}};
			case 309:
				return {{1000.0000000f, 1102.2136230f}};
			case 310:
				return {{1000.0000000f, 1102.2484131f}};
			case 311:
				return {{1000.0000000f, 1102.7979736f}};
			case 312:
				return {{1000.0000000f, 1102.8364258f}};
			case 313:
				return {{1000.0000000f, 1102.8715820f}};
			case 314:
				return {{1000.0000000f, 1103.4129639f}};
			case 315:
				return {{1000.0000000f, 1103.4422607f}};
			case 316:
				return {{1000.0000000f, 1103.4746094f}};
			case 317:
				return {{1000.0000000f, 1103.9720459f}};
			case 318:
				return {{1000.0000000f, 1104.0050049f}};
			case 319:
				return {{1000.0000000f, 1104.4355469f}};
			case 320:
				return {{1000.0000000f, 1104.4599609f}};
			case 321:
				return {{1000.0000000f, 1104.4874268f}};
			case 322:
				return {{1000.0000000f, 1104.9113770f}};
			case 323:
				return {{1000.0000000f, 1104.9333496f}};
			case 324:
				return {{1000.0000000f, 1104.9583740f}};
			case 325:
				return {{1000.0000000f, 1105.3220215f}};
			case 326:
				return {{1000.0000000f, 1105.3419189f}};
			case 327:
				return {{1000.0000000f, 1105.3648682f}};
			case 328:
				return {{1000.0000000f, 1105.7185059f}};
			case 329:
				return {{1000.0000000f, 1105.7362061f}};
			case 330:
				return {{1000.0000000f, 1105.7569580f}};
			case 331:
				return {{1000.0000000f, 1106.0747070f}};
			case 332:
				return {{1000.0000000f, 1106.0936279f}};
			case 333:
				return {{1000.0000000f, 1106.1156006f}};
			case 334:
				return {{1000.0000000f, 1106.3825684f}};
			case 335:
				return {{1000.0000000f, 1106.4029541f}};
			case 336:
				return {{1000.0000000f, 1106.4200439f}};
			case 337:
				return {{1000.0000000f, 1106.6773682f}};
			case 338:
				return {{1000.0000000f, 1106.6960449f}};
			case 339:
				return {{1000.0000000f, 1106.7082520f}};
			case 340:
				return {{1000.0000000f, 1106.9455566f}};
			case 341:
				return {{1000.0000000f, 1106.9562988f}};
			case 342:
				return {{1000.0000000f, 1107.1579590f}};
			case 343:
				return {{1000.0000000f, 1107.3502197f}};
			case 344:
				return {{1000.0000000f, 1107.5102539f}};
			case 345:
				return {{1000.0000000f, 1107.5206299f}};
			case 346:
				return {{1000.0000000f, 1107.5340576f}};
			case 347:
				return {{1000.0000000f, 1107.6860352f}};
			case 348:
				return {{1000.0000000f, 1107.6951904f}};
			case 349:
				return {{1000.0000000f, 1107.8366699f}};
			case 350:
				return {{1000.0000000f, 1107.8416748f}};
			case 351:
				return {{1000.0000000f, 1107.8529053f}};
			case 352:
				return {{1000.0000000f, 1107.9676514f}};
			case 353:
				return {{1000.0000000f, 1107.9716797f}};
			case 354:
				return {{1000.0000000f, 1108.0836182f}};
			case 355:
				return {{1000.0000000f, 1108.0899658f}};
			case 356:
				return {{1000.0000000f, 1108.0931396f}};
			case 357:
				return {{1000.0000000f, 1108.1883545f}};
			case 358:
				return {{1000.0000000f, 1108.1971436f}};
			case 359:
				return {{1000.0000000f, 1108.2725830f}};
			case 360:
				return {{1000.0000000f, 1108.2774658f}};
			case 361:
				return {{1000.0000000f, 1108.2823486f}};
			case 362:
				return {{1000.0000000f, 1108.3585205f}};
			case 363:
				return {{1000.0000000f, 1108.3627930f}};
			case 364:
				return {{1000.0000000f, 1108.3670654f}};
			case 365:
				return {{1000.0000000f, 1108.3680420f}};
			case 366:
				return {{1000.0000000f, 1108.4299316f}};
			case 367:
				return {{1000.0000000f, 1108.4335938f}};
			case 368:
				return {{1000.0000000f, 1108.4879150f}};
			case 369:
				return {{1000.0000000f, 1108.4910889f}};
			case 370:
				return {{1000.0000000f, 1108.5377197f}};
			case 371:
				return {{1000.0000000f, 1108.5404053f}};
			case 372:
				return {{1000.0000000f, 1108.5399170f}};
			case 373:
				return {{1000.0000000f, 1108.5802002f}};
			case 374:
				return {{1000.0000000f, 1108.6109619f}};
			case 375:
				return {{1000.0000000f, 1108.6396484f}};
			case 376:
				return {{1000.0000000f, 1108.6412354f}};
			case 377:
				return {{1000.0000000f, 1108.6459961f}};
			case 378:
				return {{1000.0000000f, 1108.6680908f}};
			case 379:
				return {{1000.0000000f, 1108.6662598f}};
			case 380:
				return {{1000.0000000f, 1108.6707764f}};
			case 381:
				return {{1000.0000000f, 1108.6866455f}};
			case 382:
				return {{1000.0000000f, 1108.6909180f}};
			case 383:
				return {{1000.0000000f, 1108.6888428f}};
			case 384:
				return {{1000.0000000f, 1108.7061768f}};
			case 385:
				return {{1000.0000000f, 1108.7070312f}};
			case 386:
				return {{1000.0000000f, 1108.7047119f}};
			case 387:
				return {{1000.0000000f, 1108.7196045f}};
			case 388:
				return {{1000.0000000f, 1108.7202148f}};
			case 389:
				return {{1000.0000000f, 1108.7290039f}};
			case 390:
				return {{1000.0000000f, 1108.7263184f}};
			case 391:
				return {{1000.0000000f, 1108.7299805f}};
			case 392:
				return {{1000.0000000f, 1108.7357178f}};
			case 393:
				return {{1000.0000000f, 1108.7360840f}};
			case 394:
				return {{1000.0000000f, 1108.7364502f}};
			case 395:
				return {{1000.0000000f, 1108.7403564f}};
			case 396:
				return {{1000.0000000f, 1108.7406006f}};
			case 397:
				return {{1000.0000000f, 1108.7407227f}};
			case 398:
				return {{1000.0000000f, 1108.7395020f}};
			case 399:
				return {{1000.0000000f, 1108.7427979f}};
			case 400:
				return {{1000.0000000f, 1108.7431641f}};
			case 401:
				return {{1000.0000000f, 1108.7431641f}};
			case 402:
				return {{1000.0000000f, 1108.7431641f}};
			case 403:
				return {{1000.0000000f, 1108.7421875f}};
			case 404:
				return {{1000.0000000f, 1108.7388916f}};
			case 405:
				return {{1000.0000000f, 1108.7419434f}};
			case 406:
				return {{1000.0000000f, 1108.7399902f}};
			case 407:
				return {{1000.0000000f, 1108.7370605f}};
			case 408:
				return {{1000.0000000f, 1108.7333984f}};
			case 409:
				return {{1000.0000000f, 1108.7299805f}};
			case 410:
				return {{1000.0000000f, 1108.7297363f}};
			case 411:
				return {{1000.0000000f, 1108.7257080f}};
			case 412:
				return {{1000.0000000f, 1108.7254639f}};
			case 413:
				return {{1000.0000000f, 1108.7283936f}};
			case 414:
				return {{1000.0000000f, 1108.7236328f}};
			case 415:
				return {{1000.0000000f, 1108.7232666f}};
			case 416:
				return {{1000.0000000f, 1108.7229004f}};
			case 417:
				return {{1000.0000000f, 1108.7177734f}};
			case 418:
				return {{1000.0000000f, 1108.7174072f}};
			case 419:
				return {{1000.0000000f, 1108.7138672f}};
			case 420:
				return {{1000.0000000f, 1108.7120361f}};
			case 421:
				return {{1000.0000000f, 1108.7116699f}};
			case 422:
				return {{1000.0000000f, 1108.7061768f}};
			case 423:
				return {{1000.0000000f, 1108.7058105f}};
			case 424:
				return {{1000.0000000f, 1108.7022705f}};
			case 425:
				return {{1000.0000000f, 1108.6970215f}};
			case 426:
				return {{1000.0000000f, 1108.6998291f}};
			case 427:
				return {{1000.0000000f, 1108.6994629f}};
			case 428:
				return {{1000.0000000f, 1108.6906738f}};
			case 429:
				return {{1000.0000000f, 1108.6903076f}};
			case 430:
				return {{1000.0000000f, 1108.6931152f}};
			case 431:
				return {{1000.0000000f, 1108.6875000f}};
			case 432:
				return {{1000.0000000f, 1108.6839600f}};
			case 433:
				return {{1000.0000000f, 1108.6835938f}};
			case 434:
				return {{1000.0000000f, 1108.6784668f}};
			case 435:
				return {{1000.0000000f, 1108.6781006f}};
			case 436:
				return {{1000.0000000f, 1108.6809082f}};
			case 437:
				return {{1000.0000000f, 1108.6723633f}};
			case 438:
				return {{1000.0000000f, 1108.6719971f}};
			case 439:
				return {{1000.0000000f, 1108.6699219f}};
			case 440:
				return {{1000.0000000f, 1108.6619873f}};
			case 441:
				return {{1000.0000000f, 1108.6571045f}};
			case 442:
				return {{1000.0000000f, 1108.6600342f}};
			case 443:
				return {{1000.0000000f, 1108.6597900f}};
			case 444:
				return {{1000.0000000f, 1108.6595459f}};
			case 445:
				return {{1000.0000000f, 1108.6551514f}};
			case 446:
				return {{1000.0000000f, 1108.6549072f}};
			case 447:
				return {{1000.0000000f, 1108.6473389f}};
			case 448:
				return {{1000.0000000f, 1108.6502686f}};
			case 449:
				return {{1000.0000000f, 1108.6500244f}};
			case 450:
				return {{1000.0000000f, 1108.6427002f}};
			case 451:
				return {{1000.0000000f, 1108.6424561f}};
			case 452:
				return {{1000.0000000f, 1108.6418457f}};
			case 453:
				return {{1000.0000000f, 1108.6384277f}};
			case 454:
				return {{1000.0000000f, 1108.6381836f}};
			case 455:
				return {{1000.0000000f, 1108.6375732f}};
			case 456:
				return {{1000.0000000f, 1108.6341553f}};
			case 457:
				return {{1000.0000000f, 1108.6337891f}};
			case 458:
				return {{1000.0000000f, 1108.6335449f}};
			case 459:
				return {{1000.0000000f, 1108.6333008f}};
			case 460:
				return {{1000.0000000f, 1108.6302490f}};
			case 461:
				return {{1000.0000000f, 1108.6268311f}};
			case 462:
				return {{1000.0000000f, 1108.6265869f}};
			case 463:
				return {{1000.0000000f, 1108.6295166f}};
			case 464:
				return {{1000.0000000f, 1108.6264648f}};
			case 465:
				return {{1000.0000000f, 1108.6262207f}};
			case 466:
				return {{1000.0000000f, 1108.6259766f}};
			case 467:
				return {{1000.0000000f, 1108.6201172f}};
			case 468:
				return {{1000.0000000f, 1108.6231689f}};
			case 469:
				return {{1000.0000000f, 1108.6204834f}};
			case 470:
				return {{1000.0000000f, 1108.6180420f}};
			case 471:
				return {{1000.0000000f, 1108.6158447f}};
			case 472:
				return {{1000.0000000f, 1108.6125488f}};
			case 473:
				return {{1000.0000000f, 1108.6156006f}};
			case 474:
				return {{1000.0000000f, 1108.6154785f}};
			case 475:
				return {{1000.0000000f, 1108.6134033f}};
			case 476:
				return {{1000.0000000f, 1108.6101074f}};
			case 477:
				return {{1000.0000000f, 1108.6113281f}};
			case 478:
				return {{1000.0000000f, 1108.6112061f}};
			case 479:
				return {{1000.0000000f, 1108.6079102f}};
			case 480:
				return {{1000.0000000f, 1108.6093750f}};
			case 481:
				return {{1000.0000000f, 1108.6060791f}};
			case 482:
				return {{1000.0000000f, 1108.6091309f}};
			case 483:
				return {{1000.0000000f, 1108.6075439f}};
			case 484:
				return {{1000.0000000f, 1108.6042480f}};
			case 485:
				return {{1000.0000000f, 1108.6072998f}};
			case 486:
				return {{1000.0000000f, 1108.6027832f}};
			case 487:
				return {{1000.0000000f, 1108.6058350f}};
			case 488:
				return {{1000.0000000f, 1108.6044922f}};
			case 489:
				return {{1000.0000000f, 1108.6011963f}};
			case 490:
				return {{1000.0000000f, 1108.6042480f}};
			case 491:
				return {{1000.0000000f, 1108.6009521f}};
			case 492:
				return {{1000.0000000f, 1108.5997314f}};
			case 493:
				return {{1000.0000000f, 1108.6027832f}};
			case 494:
				return {{1000.0000000f, 1108.6018066f}};
			case 495:
				return {{1000.0000000f, 1108.5985107f}};
			case 496:
				return {{1000.0000000f, 1108.6015625f}};
			case 497:
				return {{1000.0000000f, 1108.6014404f}};
			case 498:
				return {{1000.0000000f, 1108.5972900f}};
			case 499:
				return {{1000.0000000f, 1108.5972900f}};
			case 500:
				return {{1000.0000000f, 1108.5996094f}};
			case 501:
				return {{1000.0000000f, 1108.5996094f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 351))
		{
			switch (frameNumber)
			{
			case 300:
				return {{1000.0000000f, 1099.1944580f}};
			case 502:
				return {{1000.0000000f, 1109.2420654f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 352))
		{
			switch (frameNumber)
			{
			case 299:
				return {{1000.0000000f, 1099.7669678f}};
			case 503:
				return {{1000.0000000f, 1109.8474121f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 353))
		{
			switch (frameNumber)
			{
			case 298:
				return {{1000.0000000f, 1100.3138428f}};
			case 504:
				return {{1000.0000000f, 1110.3489990f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 354))
		{
			switch (frameNumber)
			{
			case 297:
				return {{1000.0000000f, 1100.1914062f}};
			case 505:
				return {{1000.0000000f, 1110.9150391f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 355))
		{
			switch (frameNumber)
			{
			case 296:
				return {{1000.0000000f, 1100.7062988f}};
			case 506:
				return {{1000.0000000f, 1111.4635010f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 356))
		{
			switch (frameNumber)
			{
			case 295:
				return {{1000.0000000f, 1101.2049561f}};
			case 507:
				return {{1000.0000000f, 1112.0008545f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 357))
		{
			switch (frameNumber)
			{
			case 294:
				return {{1000.0000000f, 1101.0836182f}};
			case 508:
				return {{1000.0000000f, 1112.3864746f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 358))
		{
			switch (frameNumber)
			{
			case 293:
				return {{1000.0000000f, 1101.5439453f}};
			case 509:
				return {{1000.0000000f, 1112.8740234f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 359))
		{
			switch (frameNumber)
			{
			case 292:
				return {{1000.0000000f, 1101.4848633f}};
			case 510:
				return {{1000.0000000f, 1113.1437988f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 360))
		{
			switch (frameNumber)
			{
			case 291:
				return {{1000.0000000f, 1101.9162598f}};
			case 511:
				return {{1000.0000000f, 1113.5965576f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 361))
		{
			switch (frameNumber)
			{
			case 290:
				return {{1000.0000000f, 1102.3326416f}};
			case 512:
				return {{1000.0000000f, 1113.7696533f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 362))
		{
			switch (frameNumber)
			{
			case 289:
				return {{1000.0000000f, 1102.2368164f}};
			case 513:
				return {{1000.0000000f, 1114.1881104f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 363))
		{
			switch (frameNumber)
			{
			case 288:
				return {{1000.0000000f, 1102.6282959f}};
			case 514:
				return {{1000.0000000f, 1114.5878906f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 364))
		{
			switch (frameNumber)
			{
			case 287:
				return {{1000.0000000f, 1103.0068359f}};
			case 515:
				return {{1000.0000000f, 1114.9768066f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 365))
		{
			switch (frameNumber)
			{
			case 286:
				return {{1000.0000000f, 1102.9201660f}};
			case 516:
				return {{1000.0000000f, 1114.9848633f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 366))
		{
			switch (frameNumber)
			{
			case 285:
				return {{1000.0000000f, 1103.2770996f}};
			case 517:
				return {{1000.0000000f, 1115.3457031f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 367))
		{
			switch (frameNumber)
			{
			case 284:
				return {{1000.0000000f, 1103.6225586f}};
			case 518:
				return {{1000.0000000f, 1115.2375488f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 368))
		{
			switch (frameNumber)
			{
			case 283:
				return {{1000.0000000f, 1103.5642090f}};
			case 519:
				return {{1000.0000000f, 1115.5711670f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 369))
		{
			switch (frameNumber)
			{
			case 282:
				return {{1000.0000000f, 1103.8900146f}};
			case 520:
				return {{1000.0000000f, 1115.8895264f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 370))
		{
			switch (frameNumber)
			{
			case 281:
				return {{1000.0000000f, 1104.2058105f}};
			case 521:
				return {{1000.0000000f, 1115.7086182f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 371))
		{
			switch (frameNumber)
			{
			case 280:
				return {{1000.0000000f, 1104.1092529f}};
			case 522:
				return {{1000.0000000f, 1116.0056152f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 372))
		{
			switch (frameNumber)
			{
			case 279:
				return {{1000.0000000f, 1104.0449219f}};
			case 523:
				return {{1000.0000000f, 1116.2928467f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 373))
		{
			switch (frameNumber)
			{
			case 278:
				return {{1000.0000000f, 1103.9707031f}};
			case 524:
				return {{1000.0000000f, 1115.9822998f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 374))
		{
			switch (frameNumber)
			{
			case 277:
				return {{1000.0000000f, 1104.2570801f}};
			case 525:
				return {{1000.0000000f, 1116.2486572f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == k_ScreenCentreLine[10])
		{
			switch (frameNumber)
			{
			case 0:
				return {{0.0000000f, 3.4066925f}};
			case 202:
				return {{1000.0000000f, 1097.5006104f}};
			case 276:
				return {{1000.0000000f, 1104.5311279f}};
			case 526:
				return {{1000.0000000f, 1116.5046387f}};
			case 602:
				return {{1000.0000000f, 1099.3371582f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 376))
		{
			switch (frameNumber)
			{
			case 275:
				return {{1000.0000000f, 1104.4638672f}};
			case 527:
				return {{1000.0000000f, 1116.1533203f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 377))
		{
			switch (frameNumber)
			{
			case 274:
				return {{1000.0000000f, 1104.7263184f}};
			case 528:
				return {{1000.0000000f, 1116.3933105f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 378))
		{
			switch (frameNumber)
			{
			case 273:
				return {{1000.0000000f, 1104.9818115f}};
			case 529:
				return {{1000.0000000f, 1116.6259766f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 379))
		{
			switch (frameNumber)
			{
			case 272:
				return {{1000.0000000f, 1104.9354248f}};
			case 530:
				return {{1000.0000000f, 1116.1655273f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 380))
		{
			switch (frameNumber)
			{
			case 271:
				return {{1000.0000000f, 1105.1783447f}};
			case 531:
				return {{1000.0000000f, 1116.3820801f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 381))
		{
			switch (frameNumber)
			{
			case 270:
				return {{1000.0000000f, 1105.1120605f}};
			case 532:
				return {{1000.0000000f, 1116.5922852f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 382))
		{
			switch (frameNumber)
			{
			case 269:
				return {{1000.0000000f, 1105.3431396f}};
			case 533:
				return {{1000.0000000f, 1116.0820312f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 383))
		{
			switch (frameNumber)
			{
			case 268:
				return {{1000.0000000f, 1105.2779541f}};
			case 534:
				return {{1000.0000000f, 1116.2784424f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 384))
		{
			switch (frameNumber)
			{
			case 267:
				return {{1000.0000000f, 1105.4981689f}};
			case 535:
				return {{1000.0000000f, 1116.4674072f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 385))
		{
			switch (frameNumber)
			{
			case 266:
				return {{1000.0000000f, 1105.4515381f}};
			case 536:
				return {{1000.0000000f, 1115.1813965f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 386))
		{
			switch (frameNumber)
			{
			case 265:
				return {{1000.0000000f, 1105.6613770f}};
			case 537:
				return {{1000.0000000f, 1115.3604736f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 387))
		{
			switch (frameNumber)
			{
			case 264:
				return {{1000.0000000f, 1105.5942383f}};
			case 538:
				return {{1000.0000000f, 1114.8122559f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 388))
		{
			switch (frameNumber)
			{
			case 263:
				return {{1000.0000000f, 1105.5489502f}};
			case 539:
				return {{1000.0000000f, 1114.9830322f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 389))
		{
			switch (frameNumber)
			{
			case 262:
				return {{1000.0000000f, 1105.7449951f}};
			case 540:
				return {{1000.0000000f, 1115.1488037f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 390))
		{
			switch (frameNumber)
			{
			case 261:
				return {{1000.0000000f, 1105.6846924f}};
			case 541:
				return {{1000.0000000f, 1114.6584473f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 391))
		{
			switch (frameNumber)
			{
			case 260:
				return {{1000.0000000f, 1105.8723145f}};
			case 542:
				return {{1000.0000000f, 1114.8165283f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 392))
		{
			switch (frameNumber)
			{
			case 259:
				return {{1000.0000000f, 1105.8150635f}};
			case 543:
				return {{1000.0000000f, 1114.9720459f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 393))
		{
			switch (frameNumber)
			{
			case 258:
				return {{1000.0000000f, 1105.9946289f}};
			case 544:
				return {{1000.0000000f, 1114.4438477f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 394))
		{
			switch (frameNumber)
			{
			case 257:
				return {{1000.0000000f, 1106.1702881f}};
			case 545:
				return {{1000.0000000f, 1114.5908203f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 395))
		{
			switch (frameNumber)
			{
			case 256:
				return {{1000.0000000f, 1106.1237793f}};
			case 546:
				return {{1000.0000000f, 1114.7337646f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 396))
		{
			switch (frameNumber)
			{
			case 255:
				return {{1000.0000000f, 1106.2917480f}};
			case 547:
				return {{1000.0000000f, 1114.2623291f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 397))
		{
			switch (frameNumber)
			{
			case 254:
				return {{1000.0000000f, 1106.2229004f}};
			case 548:
				return {{1000.0000000f, 1114.3990479f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 398))
		{
			switch (frameNumber)
			{
			case 253:
				return {{1000.0000000f, 1105.9335938f}};
			case 549:
				return {{1000.0000000f, 1114.5322266f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 399))
		{
			switch (frameNumber)
			{
			case 252:
				return {{1000.0000000f, 1106.0920410f}};
			case 550:
				return {{1000.0000000f, 1114.0306396f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 400))
		{
			switch (frameNumber)
			{
			case 251:
				return {{1000.0000000f, 1106.2501221f}};
			case 551:
				return {{1000.0000000f, 1114.1584473f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 401))
		{
			switch (frameNumber)
			{
			case 250:
				return {{1000.0000000f, 1106.1940918f}};
			case 552:
				return {{1000.0000000f, 1113.6733398f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 402))
		{
			switch (frameNumber)
			{
			case 249:
				return {{1000.0000000f, 1106.3432617f}};
			case 553:
				return {{1000.0000000f, 1113.7977295f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 403))
		{
			switch (frameNumber)
			{
			case 248:
				return {{1000.0000000f, 1106.3020020f}};
			case 554:
				return {{1000.0000000f, 1113.3731689f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 404))
		{
			switch (frameNumber)
			{
			case 247:
				return {{1000.0000000f, 1106.4470215f}};
			case 555:
				return {{1000.0000000f, 1113.4901123f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 405))
		{
			switch (frameNumber)
			{
			case 246:
				return {{1000.0000000f, 1106.3969727f}};
			case 556:
				return {{1000.0000000f, 1113.6070557f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 406))
		{
			switch (frameNumber)
			{
			case 245:
				return {{1000.0000000f, 1106.5351562f}};
			case 557:
				return {{1000.0000000f, 1113.1643066f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 407))
		{
			switch (frameNumber)
			{
			case 244:
				return {{1000.0000000f, 1106.4884033f}};
			case 558:
				return {{1000.0000000f, 1113.2758789f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 408))
		{
			switch (frameNumber)
			{
			case 243:
				return {{1000.0000000f, 1106.6217041f}};
			case 559:
				return {{1000.0000000f, 1113.3848877f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 409))
		{
			switch (frameNumber)
			{
			case 242:
				return {{1000.0000000f, 1106.5880127f}};
			case 560:
				return {{1000.0000000f, 1112.9617920f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 410))
		{
			switch (frameNumber)
			{
			case 241:
				return {{1000.0000000f, 1106.7180176f}};
			case 561:
				return {{1000.0000000f, 1113.0671387f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 411))
		{
			switch (frameNumber)
			{
			case 240:
				return {{1000.0000000f, 1106.6766357f}};
			case 562:
				return {{1000.0000000f, 1113.1701660f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 412))
		{
			switch (frameNumber)
			{
			case 239:
				return {{1000.0000000f, 1106.8006592f}};
			case 563:
				return {{1000.0000000f, 1112.7989502f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == k_ScreenCentreLine[11])
		{
			switch (frameNumber)
			{
			case 0:
				return {{0.0000000f, -0.9229774f}};
			case 202:
				return {{1000.0000000f, 1104.8607178f}};
			case 238:
				return {{1000.0000000f, 1106.7613525f}};
			case 564:
				return {{1000.0000000f, 1112.8984375f}};
			case 602:
				return {{1000.0000000f, 1106.6951904f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 414))
		{
			switch (frameNumber)
			{
			case 237:
				return {{1000.0000000f, 1106.8811035f}};
			case 565:
				return {{1000.0000000f, 1112.9957275f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 415))
		{
			switch (frameNumber)
			{
			case 236:
				return {{1000.0000000f, 1106.8537598f}};
			case 566:
				return {{1000.0000000f, 1112.6074219f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 416))
		{
			switch (frameNumber)
			{
			case 235:
				return {{1000.0000000f, 1106.9697266f}};
			case 567:
				return {{1000.0000000f, 1112.7004395f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 417))
		{
			switch (frameNumber)
			{
			case 234:
				return {{1000.0000000f, 1106.9321289f}};
			case 568:
				return {{1000.0000000f, 1112.3627930f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 418))
		{
			switch (frameNumber)
			{
			case 233:
				return {{1000.0000000f, 1106.9074707f}};
			case 569:
				return {{1000.0000000f, 1112.0151367f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 419))
		{
			switch (frameNumber)
			{
			case 232:
				return {{1000.0000000f, 1107.0163574f}};
			case 570:
				return {{1000.0000000f, 1111.6939697f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 420))
		{
			switch (frameNumber)
			{
			case 231:
				return {{1000.0000000f, 1107.1254883f}};
			case 571:
				return {{1000.0000000f, 1111.7851562f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 421))
		{
			switch (frameNumber)
			{
			case 230:
				return {{1000.0000000f, 1107.0885010f}};
			case 572:
				return {{1000.0000000f, 1111.8746338f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 422))
		{
			switch (frameNumber)
			{
			case 229:
				return {{1000.0000000f, 1106.9218750f}};
			case 573:
				return {{1000.0000000f, 1111.6022949f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 423))
		{
			switch (frameNumber)
			{
			case 228:
				return {{1000.0000000f, 1107.0253906f}};
			case 574:
				return {{1000.0000000f, 1111.6906738f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 424))
		{
			switch (frameNumber)
			{
			case 227:
				return {{1000.0000000f, 1107.0081787f}};
			case 575:
				return {{1000.0000000f, 1111.7761230f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 425))
		{
			switch (frameNumber)
			{
			case 226:
				return {{1000.0000000f, 1107.1091309f}};
			case 576:
				return {{1000.0000000f, 1111.5050049f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 426))
		{
			switch (frameNumber)
			{
			case 225:
				return {{1000.0000000f, 1107.1016846f}};
			case 577:
				return {{1000.0000000f, 1111.5904541f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 427))
		{
			switch (frameNumber)
			{
			case 224:
				return {{1000.0000000f, 1107.2014160f}};
			case 578:
				return {{1000.0000000f, 1111.6718750f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 428))
		{
			switch (frameNumber)
			{
			case 223:
				return {{1000.0000000f, 1107.2136230f}};
			case 579:
				return {{1000.0000000f, 1111.4313965f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 429))
		{
			switch (frameNumber)
			{
			case 222:
				return {{1000.0000000f, 1107.3098145f}};
			case 580:
				return {{1000.0000000f, 1111.5140381f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 430))
		{
			switch (frameNumber)
			{
			case 221:
				return {{1000.0000000f, 1107.3195801f}};
			case 581:
				return {{1000.0000000f, 1111.5939941f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 431))
		{
			switch (frameNumber)
			{
			case 220:
				return {{1000.0000000f, 1107.4116211f}};
			case 582:
				return {{1000.0000000f, 1111.3868408f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 432))
		{
			switch (frameNumber)
			{
			case 219:
				return {{1000.0000000f, 1107.4289551f}};
			case 583:
				return {{1000.0000000f, 1111.4652100f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 433))
		{
			switch (frameNumber)
			{
			case 218:
				return {{1000.0000000f, 1107.5198975f}};
			case 584:
				return {{1000.0000000f, 1111.2423096f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 434))
		{
			switch (frameNumber)
			{
			case 217:
				return {{1000.0000000f, 1107.5400391f}};
			case 585:
				return {{1000.0000000f, 1111.3170166f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 435))
		{
			switch (frameNumber)
			{
			case 216:
				return {{1000.0000000f, 1107.6276855f}};
			case 586:
				return {{1000.0000000f, 1111.3892822f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 436))
		{
			switch (frameNumber)
			{
			case 215:
				return {{1000.0000000f, 1107.6594238f}};
			case 587:
				return {{1000.0000000f, 1111.1896973f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 437))
		{
			switch (frameNumber)
			{
			case 214:
				return {{1000.0000000f, 1107.7464600f}};
			case 588:
				return {{1000.0000000f, 1111.2608643f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 438))
		{
			switch (frameNumber)
			{
			case 213:
				return {{1000.0000000f, 1107.7901611f}};
			case 589:
				return {{1000.0000000f, 1111.3319092f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 439))
		{
			switch (frameNumber)
			{
			case 212:
				return {{1000.0000000f, 1107.8752441f}};
			case 590:
				return {{1000.0000000f, 1111.1163330f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 440))
		{
			switch (frameNumber)
			{
			case 211:
				return {{1000.0000000f, 1107.9255371f}};
			case 591:
				return {{1000.0000000f, 1111.1843262f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 441))
		{
			switch (frameNumber)
			{
			case 210:
				return {{1000.0000000f, 1108.0080566f}};
			case 592:
				return {{1000.0000000f, 1111.2509766f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 442))
		{
			switch (frameNumber)
			{
			case 209:
				return {{1000.0000000f, 1108.0686035f}};
			case 593:
				return {{1000.0000000f, 1111.0396729f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 443))
		{
			switch (frameNumber)
			{
			case 208:
				return {{1000.0000000f, 1108.1505127f}};
			case 594:
				return {{1000.0000000f, 1111.1044922f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 444))
		{
			switch (frameNumber)
			{
			case 207:
				return {{1000.0000000f, 1108.2313232f}};
			case 595:
				return {{1000.0000000f, 1111.1704102f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 445))
		{
			switch (frameNumber)
			{
			case 206:
				return {{1000.0000000f, 1108.2969971f}};
			case 596:
				return {{1000.0000000f, 1110.9803467f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 446))
		{
			switch (frameNumber)
			{
			case 205:
				return {{1000.0000000f, 1108.3601074f}};
			case 597:
				return {{1000.0000000f, 1111.0433350f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 447))
		{
			switch (frameNumber)
			{
			case 204:
				return {{1000.0000000f, 1108.4385986f}};
			case 598:
				return {{1000.0000000f, 1111.1032715f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 448))
		{
			switch (frameNumber)
			{
			case 203:
				return {{1000.0000000f, 1108.5148926f}};
			case 599:
				return {{1000.0000000f, 1110.9006348f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == glm::u16vec2(400, 449))
		{
			switch (frameNumber)
			{
			case 202:
				return {{1000.0000000f, 1108.5913086f}};
			case 600:
				return {{1000.0000000f, 1110.9608154f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == k_ScreenCentreLine[12])
		{
			switch (frameNumber)
			{
			case 0:
				return {{0.0000000f, -4.5307636f}};
			case 1:
				return {{1000.0000000f, 1116.1998291f}};
			case 2:
				return {{1000.0000000f, 1116.1997070f}};
			case 3:
				return {{1000.0000000f, 1116.2059326f}};
			case 4:
				return {{1000.0000000f, 1116.2072754f}};
			case 5:
				return {{1000.0000000f, 1116.2073975f}};
			case 6:
				return {{1000.0000000f, 1116.2075195f}};
			case 7:
				return {{1000.0000000f, 1116.2087402f}};
			case 8:
				return {{1000.0000000f, 1116.2087402f}};
			case 9:
				return {{1000.0000000f, 1116.2099609f}};
			case 10:
				return {{1000.0000000f, 1116.2099609f}};
			case 11:
				return {{1000.0000000f, 1116.2099609f}};
			case 12:
				return {{1000.0000000f, 1116.2109375f}};
			case 13:
				return {{1000.0000000f, 1116.2109375f}};
			case 14:
				return {{1000.0000000f, 1116.2110596f}};
			case 15:
				return {{1000.0000000f, 1116.2110596f}};
			case 16:
				return {{1000.0000000f, 1116.2119141f}};
			case 17:
				return {{1000.0000000f, 1116.2119141f}};
			case 18:
				return {{1000.0000000f, 1116.2126465f}};
			case 19:
				return {{1000.0000000f, 1116.2132568f}};
			case 20:
				return {{1000.0000000f, 1116.2138672f}};
			case 21:
				return {{1000.0000000f, 1116.2138672f}};
			case 22:
				return {{1000.0000000f, 1116.2143555f}};
			case 23:
				return {{1000.0000000f, 1116.2144775f}};
			case 24:
				return {{1000.0000000f, 1116.2148438f}};
			case 25:
				return {{1000.0000000f, 1116.2148438f}};
			case 26:
				return {{1000.0000000f, 1116.2153320f}};
			case 27:
				return {{1000.0000000f, 1116.2153320f}};
			case 28:
				return {{1000.0000000f, 1116.2156982f}};
			case 29:
				return {{1000.0000000f, 1116.2156982f}};
			case 30:
				return {{1000.0000000f, 1116.2160645f}};
			case 31:
				return {{1000.0000000f, 1116.2160645f}};
			case 32:
				return {{1000.0000000f, 1116.2163086f}};
			case 33:
				return {{1000.0000000f, 1116.2164307f}};
			case 34:
				return {{1000.0000000f, 1116.2167969f}};
			case 35:
				return {{1000.0000000f, 1116.2167969f}};
			case 36:
				return {{1000.0000000f, 1116.2169189f}};
			case 37:
				return {{1000.0000000f, 1116.2169189f}};
			case 38:
				return {{1000.0000000f, 1116.2170410f}};
			case 39:
				return {{1000.0000000f, 1116.0631104f}};
			case 40:
				return {{1000.0000000f, 1116.0500488f}};
			case 41:
				return {{1000.0000000f, 1115.8371582f}};
			case 42:
				return {{1000.0000000f, 1115.8244629f}};
			case 43:
				return {{1000.0000000f, 1115.6113281f}};
			case 44:
				return {{1000.0000000f, 1115.4114990f}};
			case 45:
				return {{1000.0000000f, 1115.2003174f}};
			case 46:
				return {{1000.0000000f, 1115.1870117f}};
			case 47:
				return {{1000.0000000f, 1114.9885254f}};
			case 48:
				return {{1000.0000000f, 1114.9752197f}};
			case 49:
				return {{1000.0000000f, 1114.7661133f}};
			case 50:
				return {{1000.0000000f, 1114.7526855f}};
			case 51:
				return {{1000.0000000f, 1114.5437012f}};
			case 52:
				return {{1000.0000000f, 1114.5313721f}};
			case 53:
				return {{1000.0000000f, 1114.3376465f}};
			case 54:
				return {{1000.0000000f, 1114.3237305f}};
			case 55:
				return {{1000.0000000f, 1114.1207275f}};
			case 56:
				return {{1000.0000000f, 1114.1076660f}};
			case 57:
				return {{1000.0000000f, 1113.9056396f}};
			case 58:
				return {{1000.0000000f, 1113.8916016f}};
			case 59:
				return {{1000.0000000f, 1113.7060547f}};
			case 60:
				return {{1000.0000000f, 1113.6927490f}};
			case 61:
				return {{1000.0000000f, 1113.4967041f}};
			case 62:
				return {{1000.0000000f, 1113.4848633f}};
			case 63:
				return {{1000.0000000f, 1113.3021240f}};
			case 64:
				return {{1000.0000000f, 1113.2899170f}};
			case 65:
				return {{1000.0000000f, 1113.0999756f}};
			case 66:
				return {{1000.0000000f, 1112.9127197f}};
			case 67:
				return {{1000.0000000f, 1112.7402344f}};
			case 68:
				return {{1000.0000000f, 1112.7285156f}};
			case 69:
				return {{1000.0000000f, 1112.5487061f}};
			case 70:
				return {{1000.0000000f, 1112.5377197f}};
			case 71:
				return {{1000.0000000f, 1112.3625488f}};
			case 72:
				return {{1000.0000000f, 1112.3515625f}};
			case 73:
				return {{1000.0000000f, 1112.1894531f}};
			case 74:
				return {{1000.0000000f, 1112.1791992f}};
			case 75:
				return {{1000.0000000f, 1112.1695557f}};
			case 76:
				return {{1000.0000000f, 1112.0009766f}};
			case 77:
				return {{1000.0000000f, 1111.9915771f}};
			case 78:
				return {{1000.0000000f, 1111.8374023f}};
			case 79:
				return {{1000.0000000f, 1111.8281250f}};
			case 80:
				return {{1000.0000000f, 1111.6679688f}};
			case 81:
				return {{1000.0000000f, 1111.6583252f}};
			case 82:
				return {{1000.0000000f, 1111.5053711f}};
			case 83:
				return {{1000.0000000f, 1111.4946289f}};
			case 84:
				return {{1000.0000000f, 1111.3544922f}};
			case 85:
				return {{1000.0000000f, 1111.3453369f}};
			case 86:
				return {{1000.0000000f, 1111.2001953f}};
			case 87:
				return {{1000.0000000f, 1111.1918945f}};
			case 88:
				return {{1000.0000000f, 1111.0515137f}};
			case 89:
				return {{1000.0000000f, 1111.0428467f}};
			case 90:
				return {{1000.0000000f, 1110.9154053f}};
			case 91:
				return {{1000.0000000f, 1110.7847900f}};
			case 92:
				return {{1000.0000000f, 1110.6647949f}};
			case 93:
				return {{1000.0000000f, 1110.6569824f}};
			case 94:
				return {{1000.0000000f, 1110.5344238f}};
			case 95:
				return {{1000.0000000f, 1110.5277100f}};
			case 96:
				return {{1000.0000000f, 1110.4105225f}};
			case 97:
				return {{1000.0000000f, 1110.4025879f}};
			case 98:
				return {{1000.0000000f, 1110.2971191f}};
			case 99:
				return {{1000.0000000f, 1110.2906494f}};
			case 100:
				return {{1000.0000000f, 1110.1828613f}};
			case 101:
				return {{1000.0000000f, 1110.1771240f}};
			case 102:
				return {{1000.0000000f, 1110.0727539f}};
			case 103:
				return {{1000.0000000f, 1109.9815674f}};
			case 104:
				return {{1000.0000000f, 1109.9738770f}};
			case 105:
				return {{1000.0000000f, 1109.8812256f}};
			case 106:
				return {{1000.0000000f, 1109.8757324f}};
			case 107:
				return {{1000.0000000f, 1109.8702393f}};
			case 108:
				return {{1000.0000000f, 1109.7858887f}};
			case 109:
				return {{1000.0000000f, 1109.7807617f}};
			case 110:
				return {{1000.0000000f, 1109.6961670f}};
			case 111:
				return {{1000.0000000f, 1109.6900635f}};
			case 112:
				return {{1000.0000000f, 1109.6108398f}};
			case 113:
				return {{1000.0000000f, 1109.6064453f}};
			case 114:
				return {{1000.0000000f, 1109.4621582f}};
			case 115:
				return {{1000.0000000f, 1109.3956299f}};
			case 116:
				return {{1000.0000000f, 1109.3911133f}};
			case 117:
				return {{1000.0000000f, 1109.3317871f}};
			case 118:
				return {{1000.0000000f, 1109.3271484f}};
			case 119:
				return {{1000.0000000f, 1109.2663574f}};
			case 120:
				return {{1000.0000000f, 1109.2640381f}};
			case 121:
				return {{1000.0000000f, 1109.2104492f}};
			case 122:
				return {{1000.0000000f, 1109.2072754f}};
			case 123:
				return {{1000.0000000f, 1109.2038574f}};
			case 124:
				return {{1000.0000000f, 1109.1527100f}};
			case 125:
				return {{1000.0000000f, 1109.1042480f}};
			case 126:
				return {{1000.0000000f, 1109.1013184f}};
			case 127:
				return {{1000.0000000f, 1109.0588379f}};
			case 128:
				return {{1000.0000000f, 1109.0552979f}};
			case 129:
				return {{1000.0000000f, 1109.0543213f}};
			case 130:
				return {{1000.0000000f, 1109.0135498f}};
			case 131:
				return {{1000.0000000f, 1108.9732666f}};
			case 132:
				return {{1000.0000000f, 1108.9721680f}};
			case 133:
				return {{1000.0000000f, 1108.9395752f}};
			case 134:
				return {{1000.0000000f, 1108.9367676f}};
			case 135:
				return {{1000.0000000f, 1108.9049072f}};
			case 136:
				return {{1000.0000000f, 1108.9038086f}};
			case 137:
				return {{1000.0000000f, 1108.8754883f}};
			case 138:
				return {{1000.0000000f, 1108.8500977f}};
			case 139:
				return {{1000.0000000f, 1108.8258057f}};
			case 140:
				return {{1000.0000000f, 1108.8237305f}};
			case 141:
				return {{1000.0000000f, 1108.8041992f}};
			case 142:
				return {{1000.0000000f, 1108.8023682f}};
			case 143:
				return {{1000.0000000f, 1108.7824707f}};
			case 144:
				return {{1000.0000000f, 1108.7832031f}};
			case 145:
				return {{1000.0000000f, 1108.7645264f}};
			case 146:
				return {{1000.0000000f, 1108.7631836f}};
			case 147:
				return {{1000.0000000f, 1108.7495117f}};
			case 148:
				return {{1000.0000000f, 1108.7484131f}};
			case 149:
				return {{1000.0000000f, 1108.7355957f}};
			case 150:
				return {{1000.0000000f, 1108.7343750f}};
			case 151:
				return {{1000.0000000f, 1108.7238770f}};
			case 152:
				return {{1000.0000000f, 1108.7232666f}};
			case 153:
				return {{1000.0000000f, 1108.7136230f}};
			case 154:
				return {{1000.0000000f, 1108.7114258f}};
			case 155:
				return {{1000.0000000f, 1108.7033691f}};
			case 156:
				return {{1000.0000000f, 1108.7036133f}};
			case 157:
				return {{1000.0000000f, 1108.6971436f}};
			case 158:
				return {{1000.0000000f, 1108.6965332f}};
			case 159:
				return {{1000.0000000f, 1108.6907959f}};
			case 160:
				return {{1000.0000000f, 1108.6887207f}};
			case 161:
				return {{1000.0000000f, 1108.6809082f}};
			case 162:
				return {{1000.0000000f, 1108.6774902f}};
			case 163:
				return {{1000.0000000f, 1108.6772461f}};
			case 164:
				return {{1000.0000000f, 1108.6748047f}};
			case 165:
				return {{1000.0000000f, 1108.6750488f}};
			case 166:
				return {{1000.0000000f, 1108.6729736f}};
			case 167:
				return {{1000.0000000f, 1108.6719971f}};
			case 168:
				return {{1000.0000000f, 1108.6704102f}};
			case 169:
				return {{1000.0000000f, 1108.6713867f}};
			case 170:
				return {{1000.0000000f, 1108.6702881f}};
			case 171:
				return {{1000.0000000f, 1108.6687012f}};
			case 172:
				return {{1000.0000000f, 1108.6678467f}};
			case 173:
				return {{1000.0000000f, 1108.6683350f}};
			case 174:
				return {{1000.0000000f, 1108.6676025f}};
			case 175:
				return {{1000.0000000f, 1108.6680908f}};
			case 176:
				return {{1000.0000000f, 1108.6676025f}};
			case 177:
				return {{1000.0000000f, 1108.6667480f}};
			case 178:
				return {{1000.0000000f, 1108.6663818f}};
			case 179:
				return {{1000.0000000f, 1108.6666260f}};
			case 180:
				return {{1000.0000000f, 1108.6663818f}};
			case 181:
				return {{1000.0000000f, 1108.6665039f}};
			case 182:
				return {{1000.0000000f, 1108.6665039f}};
			case 183:
				return {{1000.0000000f, 1108.6665039f}};
			case 184:
				return {{1000.0000000f, 1108.6665039f}};
			case 185:
				return {{1000.0000000f, 1108.6665039f}};
			case 186:
				return {{1000.0000000f, 1108.6665039f}};
			case 187:
				return {{1000.0000000f, 1108.6665039f}};
			case 188:
				return {{1000.0000000f, 1108.6666260f}};
			case 189:
				return {{1000.0000000f, 1108.6665039f}};
			case 190:
				return {{1000.0000000f, 1108.6666260f}};
			case 191:
				return {{1000.0000000f, 1108.6666260f}};
			case 192:
				return {{1000.0000000f, 1108.6667480f}};
			case 193:
				return {{1000.0000000f, 1108.6666260f}};
			case 194:
				return {{1000.0000000f, 1108.6667480f}};
			case 195:
				return {{1000.0000000f, 1108.6666260f}};
			case 196:
				return {{1000.0000000f, 1108.6667480f}};
			case 197:
				return {{1000.0000000f, 1108.6667480f}};
			case 198:
				return {{1000.0000000f, 1108.6667480f}};
			case 199:
				return {{1000.0000000f, 1108.6667480f}};
			case 200:
				return {{1000.0000000f, 1108.6667480f}};
			case 201:
				return {{1000.0000000f, 1108.6667480f}};
			case 202:
				return {{1000.0000000f, 1108.6668701f}};
			case 601:
				return {{1000.0000000f, 1110.5192871f}};
			case 602:
				return {{1000.0000000f, 1110.5023193f}};
			case 603:
				return {{1000.0000000f, 1110.2556152f}};
			case 604:
				return {{1000.0000000f, 1110.2407227f}};
			case 605:
				return {{1000.0000000f, 1110.0270996f}};
			case 606:
				return {{1000.0000000f, 1110.0134277f}};
			case 607:
				return {{1000.0000000f, 1109.9998779f}};
			case 608:
				return {{1000.0000000f, 1109.7941895f}};
			case 609:
				return {{1000.0000000f, 1109.7819824f}};
			case 610:
				return {{1000.0000000f, 1109.5994873f}};
			case 611:
				return {{1000.0000000f, 1109.5887451f}};
			case 612:
				return {{1000.0000000f, 1109.4356689f}};
			case 613:
				return {{1000.0000000f, 1109.4261475f}};
			case 614:
				return {{1000.0000000f, 1109.2843018f}};
			case 615:
				return {{1000.0000000f, 1109.2761230f}};
			case 616:
				return {{1000.0000000f, 1109.2689209f}};
			case 617:
				return {{1000.0000000f, 1109.1469727f}};
			case 618:
				return {{1000.0000000f, 1109.1400146f}};
			case 619:
				return {{1000.0000000f, 1109.1340332f}};
			case 620:
				return {{1000.0000000f, 1109.0368652f}};
			case 621:
				return {{1000.0000000f, 1109.0310059f}};
			case 622:
				return {{1000.0000000f, 1108.9440918f}};
			case 623:
				return {{1000.0000000f, 1108.9390869f}};
			case 624:
				return {{1000.0000000f, 1108.9342041f}};
			case 625:
				return {{1000.0000000f, 1108.8665771f}};
			case 626:
				return {{1000.0000000f, 1108.8634033f}};
			case 627:
				return {{1000.0000000f, 1108.8593750f}};
			case 628:
				return {{1000.0000000f, 1108.7513428f}};
			case 629:
				return {{1000.0000000f, 1108.7486572f}};
			case 630:
				return {{1000.0000000f, 1108.7114258f}};
			case 631:
				return {{1000.0000000f, 1108.7092285f}};
			case 632:
				return {{1000.0000000f, 1108.7060547f}};
			case 633:
				return {{1000.0000000f, 1108.6745605f}};
			case 634:
				return {{1000.0000000f, 1108.6738281f}};
			case 635:
				return {{1000.0000000f, 1108.6480713f}};
			case 636:
				return {{1000.0000000f, 1108.6467285f}};
			case 637:
				return {{1000.0000000f, 1108.6463623f}};
			case 638:
				return {{1000.0000000f, 1108.6284180f}};
			case 639:
				return {{1000.0000000f, 1108.6263428f}};
			case 640:
				return {{1000.0000000f, 1108.6253662f}};
			case 641:
				return {{1000.0000000f, 1108.6108398f}};
			case 642:
				return {{1000.0000000f, 1108.6101074f}};
			case 643:
				return {{1000.0000000f, 1108.5998535f}};
			case 644:
				return {{1000.0000000f, 1108.5992432f}};
			case 645:
				return {{1000.0000000f, 1108.5986328f}};
			case 646:
				return {{1000.0000000f, 1108.5917969f}};
			case 647:
				return {{1000.0000000f, 1108.5914307f}};
			case 648:
				return {{1000.0000000f, 1108.5900879f}};
			case 649:
				return {{1000.0000000f, 1108.5856934f}};
			case 650:
				return {{1000.0000000f, 1108.5844727f}};
			case 651:
				return {{1000.0000000f, 1108.5811768f}};
			case 652:
				return {{1000.0000000f, 1108.5820312f}};
			case 653:
				return {{1000.0000000f, 1108.5809326f}};
			case 654:
				return {{1000.0000000f, 1108.5798340f}};
			case 655:
				return {{1000.0000000f, 1108.5787354f}};
			case 656:
				return {{1000.0000000f, 1108.5777588f}};
			case 657:
				return {{1000.0000000f, 1108.5777588f}};
			case 658:
				return {{1000.0000000f, 1108.5775146f}};
			case 659:
				return {{1000.0000000f, 1108.5777588f}};
			case 660:
				return {{1000.0000000f, 1108.5783691f}};
			case 661:
				return {{1000.0000000f, 1108.5783691f}};
			case 662:
				return {{1000.0000000f, 1108.5783691f}};
			case 663:
				return {{1000.0000000f, 1108.5802002f}};
			case 664:
				return {{1000.0000000f, 1108.5793457f}};
			case 665:
				return {{1000.0000000f, 1108.5814209f}};
			case 666:
				return {{1000.0000000f, 1108.5805664f}};
			case 667:
				return {{1000.0000000f, 1108.5816650f}};
			case 668:
				return {{1000.0000000f, 1108.5817871f}};
			case 669:
				return {{1000.0000000f, 1108.5828857f}};
			case 670:
				return {{1000.0000000f, 1108.5820312f}};
			case 671:
				return {{1000.0000000f, 1108.5841064f}};
			case 672:
				return {{1000.0000000f, 1108.5832520f}};
			case 673:
				return {{1000.0000000f, 1108.5853271f}};
			case 674:
				return {{1000.0000000f, 1108.5844727f}};
			case 675:
				return {{1000.0000000f, 1108.5845947f}};
			case 676:
				return {{1000.0000000f, 1108.5865479f}};
			case 677:
				return {{1000.0000000f, 1108.5856934f}};
			case 678:
				return {{1000.0000000f, 1108.5858154f}};
			case 679:
				return {{1000.0000000f, 1108.5877686f}};
			case 680:
				return {{1000.0000000f, 1108.5877686f}};
			case 681:
				return {{1000.0000000f, 1108.5876465f}};
			case 682:
				return {{1000.0000000f, 1108.5876465f}};
			case 683:
				return {{1000.0000000f, 1108.5876465f}};
			case 684:
				return {{1000.0000000f, 1108.5893555f}};
			case 685:
				return {{1000.0000000f, 1108.5893555f}};
			case 686:
				return {{1000.0000000f, 1108.5891113f}};
			case 687:
				return {{1000.0000000f, 1108.5891113f}};
			case 688:
				return {{1000.0000000f, 1108.5897217f}};
			case 689:
				return {{1000.0000000f, 1108.5911865f}};
			case 690:
				return {{1000.0000000f, 1108.5906982f}};
			case 691:
				return {{1000.0000000f, 1108.5906982f}};
			case 692:
				return {{1000.0000000f, 1108.5906982f}};
			case 693:
				return {{1000.0000000f, 1108.5920410f}};
			case 694:
				return {{1000.0000000f, 1108.5920410f}};
			case 695:
				return {{1000.0000000f, 1108.5924072f}};
			case 696:
				return {{1000.0000000f, 1108.5924072f}};
			case 697:
				return {{1000.0000000f, 1108.5924072f}};
			case 698:
				return {{1000.0000000f, 1108.5926514f}};
			case 699:
				return {{1000.0000000f, 1108.5926514f}};
			case 700:
				return {{1000.0000000f, 1108.5919189f}};
			case 701:
				return {{1000.0000000f, 1108.5919189f}};
			case 702:
				return {{1000.0000000f, 1108.5930176f}};
			case 703:
				return {{1000.0000000f, 1108.5930176f}};
			case 704:
				return {{1000.0000000f, 1108.5930176f}};
			case 705:
				return {{1000.0000000f, 1108.5921631f}};
			case 706:
				return {{1000.0000000f, 1108.5921631f}};
			case 707:
				return {{1000.0000000f, 1108.5921631f}};
			case 708:
				return {{1000.0000000f, 1108.5932617f}};
			case 709:
				return {{1000.0000000f, 1108.5932617f}};
			case 710:
				return {{1000.0000000f, 1108.5924072f}};
			case 711:
				return {{1000.0000000f, 1108.5924072f}};
			case 712:
				return {{1000.0000000f, 1108.5924072f}};
			case 713:
				return {{1000.0000000f, 1108.5925293f}};
			case 714:
				return {{1000.0000000f, 1108.5925293f}};
			case 715:
				return {{1000.0000000f, 1108.5925293f}};
			case 716:
				return {{1000.0000000f, 1108.5925293f}};
			case 717:
				return {{1000.0000000f, 1108.5925293f}};
			case 718:
				return {{1000.0000000f, 1108.5925293f}};
			case 719:
				return {{1000.0000000f, 1108.5925293f}};
			case 720:
				return {{1000.0000000f, 1108.5925293f}};
			case 721:
				return {{1000.0000000f, 1108.5925293f}};
			case 722:
				return {{1000.0000000f, 1108.5925293f}};
			case 723:
				return {{1000.0000000f, 1108.5925293f}};
			case 724:
				return {{1000.0000000f, 1108.5925293f}};
			case 725:
				return {{1000.0000000f, 1108.5925293f}};
			case 726:
				return {{1000.0000000f, 1108.5925293f}};
			case 727:
				return {{1000.0000000f, 1108.5925293f}};
			case 728:
				return {{1000.0000000f, 1108.5925293f}};
			case 729:
				return {{1000.0000000f, 1108.5925293f}};
			case 730:
				return {{1000.0000000f, 1108.5925293f}};
			case 731:
				return {{1000.0000000f, 1108.5925293f}};
			case 732:
				return {{1000.0000000f, 1108.5925293f}};
			case 733:
				return {{1000.0000000f, 1108.5925293f}};
			case 734:
				return {{1000.0000000f, 1108.5925293f}};
			case 735:
				return {{1000.0000000f, 1108.5925293f}};
			case 736:
				return {{1000.0000000f, 1108.5925293f}};
			case 737:
				return {{1000.0000000f, 1108.5925293f}};
			case 738:
				return {{1000.0000000f, 1108.5925293f}};
			case 739:
				return {{1000.0000000f, 1108.5925293f}};
			case 740:
				return {{1000.0000000f, 1108.5925293f}};
			case 741:
				return {{1000.0000000f, 1108.5925293f}};
			case 742:
				return {{1000.0000000f, 1108.5925293f}};
			case 743:
				return {{1000.0000000f, 1108.5925293f}};
			case 744:
				return {{1000.0000000f, 1108.5925293f}};
			case 745:
				return {{1000.0000000f, 1108.5925293f}};
			case 746:
				return {{1000.0000000f, 1108.5925293f}};
			case 747:
				return {{1000.0000000f, 1108.5925293f}};
			case 748:
				return {{1000.0000000f, 1108.5925293f}};
			case 749:
				return {{1000.0000000f, 1108.5925293f}};
			case 750:
				return {{1000.0000000f, 1108.5925293f}};
			case 751:
				return {{1000.0000000f, 1108.5925293f}};
			case 752:
				return {{1000.0000000f, 1108.5925293f}};
			case 753:
				return {{1000.0000000f, 1108.5925293f}};
			case 754:
				return {{1000.0000000f, 1108.5925293f}};
			case 755:
				return {{1000.0000000f, 1108.5925293f}};
			case 756:
				return {{1000.0000000f, 1108.5925293f}};
			case 757:
				return {{1000.0000000f, 1108.5925293f}};
			case 758:
				return {{1000.0000000f, 1108.5925293f}};
			case 759:
				return {{1000.0000000f, 1108.5925293f}};
			case 760:
				return {{1000.0000000f, 1108.5925293f}};
			case 761:
				return {{1000.0000000f, 1108.5925293f}};
			case 762:
				return {{1000.0000000f, 1108.5925293f}};
			case 763:
				return {{1000.0000000f, 1108.5925293f}};
			case 764:
				return {{1000.0000000f, 1108.5925293f}};
			case 765:
				return {{1000.0000000f, 1108.5925293f}};
			case 766:
				return {{1000.0000000f, 1108.5925293f}};
			case 767:
				return {{1000.0000000f, 1108.5925293f}};
			case 768:
				return {{1000.0000000f, 1108.5925293f}};
			case 769:
				return {{1000.0000000f, 1108.5925293f}};
			case 770:
				return {{1000.0000000f, 1108.5925293f}};
			case 771:
				return {{1000.0000000f, 1108.5925293f}};
			case 772:
				return {{1000.0000000f, 1108.5925293f}};
			case 773:
				return {{1000.0000000f, 1108.5925293f}};
			case 774:
				return {{1000.0000000f, 1108.5925293f}};
			case 775:
				return {{1000.0000000f, 1108.5925293f}};
			case 776:
				return {{1000.0000000f, 1108.5925293f}};
			case 777:
				return {{1000.0000000f, 1108.5925293f}};
			case 778:
				return {{1000.0000000f, 1108.5925293f}};
			case 779:
				return {{1000.0000000f, 1108.5925293f}};
			case 780:
				return {{1000.0000000f, 1108.5925293f}};
			case 781:
				return {{1000.0000000f, 1108.5925293f}};
			case 782:
				return {{1000.0000000f, 1108.5925293f}};
			case 783:
				return {{1000.0000000f, 1108.5925293f}};
			case 784:
				return {{1000.0000000f, 1108.5925293f}};
			case 785:
				return {{1000.0000000f, 1108.5925293f}};
			case 786:
				return {{1000.0000000f, 1108.5925293f}};
			case 787:
				return {{1000.0000000f, 1108.5925293f}};
			case 788:
				return {{1000.0000000f, 1108.5925293f}};
			case 789:
				return {{1000.0000000f, 1108.5925293f}};
			case 790:
				return {{1000.0000000f, 1108.5925293f}};
			case 791:
				return {{1000.0000000f, 1108.5925293f}};
			case 792:
				return {{1000.0000000f, 1108.5925293f}};
			case 793:
				return {{1000.0000000f, 1108.5925293f}};
			case 794:
				return {{1000.0000000f, 1108.5925293f}};
			case 795:
				return {{1000.0000000f, 1108.5925293f}};
			case 796:
				return {{1000.0000000f, 1108.5925293f}};
			case 797:
				return {{1000.0000000f, 1108.5925293f}};
			case 798:
				return {{1000.0000000f, 1108.5925293f}};
			case 799:
				return {{1000.0000000f, 1108.5925293f}};
			case 800:
				return {{1000.0000000f, 1108.5925293f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == k_ScreenCentreLine[13])
		{
			switch (frameNumber)
			{
			case 0:
				return {{0.0000000f, -7.4373074f}};
			case 202:
				return {{1000.0000000f, 1110.8963623f}};
			case 602:
				return {{1000.0000000f, 1112.7314453f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == k_ScreenCentreLine[14])
		{
			switch (frameNumber)
			{
			case 0:
				return {{0.0000000f, -9.9486294f}};
			case 202:
				return {{1000.0000000f, 1112.4274902f}};
			case 602:
				return {{1000.0000000f, 1114.2617188f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == k_ScreenCentreLine[15])
		{
			switch (frameNumber)
			{
			case 0:
				return {{0.0000000f, -12.0339508f}};
			case 202:
				return {{1000.0000000f, 1113.4934082f}};
			case 602:
				return {{1000.0000000f, 1115.3277588f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		assert(false); // Shouldn't be any unaccounted raycasts
		return std::nullopt;
	}
};
// NOLINTEND(bugprone-branch-clone, google-readability-function-size, readability-function-size)
