/*******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "Mock.h"

class TwoButtonZoomOutInMockAction final: public MockAction
{
	static constexpr uint32_t k_Start1 = k_StabilizeFrames;
	static constexpr uint32_t k_End1 = k_StabilizeFrames + k_InteractionFrames;
	static constexpr uint32_t k_Start2 = k_StabilizeFrames + k_InteractionFrames + k_StabilizeFrames;

public:
	~TwoButtonZoomOutInMockAction() final = default;

	[[nodiscard]] bool GetUnbindable(openblack::input::UnbindableActionMap action) const final
	{
		using openblack::input::UnbindableActionMap;
		if ((frameNumber > k_Start1 && frameNumber < k_End1) || frameNumber > k_Start2)
		{
			return (static_cast<uint8_t>(action) & (static_cast<uint8_t>(UnbindableActionMap::TWO_BUTTON_CLICK))) != 0;
		}
		return false;
	}

	[[nodiscard]] glm::uvec2 GetMousePosition() const override { return {400, 450}; }

	[[nodiscard]] glm::ivec2 GetMouseDelta() const final
	{
		if (frameNumber > k_Start1 && frameNumber < k_End1)
		{
			return {0, 3};
		}
		if (frameNumber > k_Start2)
		{
			return {0, -3};
		}
		return {0, 0};
	}
};

// NOLINTBEGIN(bugprone-branch-clone, google-readability-function-size)
class TwoButtonZoomOutInMockDynamicsSystem: public MockDynamicsSystem
{
public:
	TwoButtonZoomOutInMockDynamicsSystem() = default;

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
			case 301:
				return std::nullopt;
			case 502:
				return std::nullopt;
			case 600:
				return std::nullopt;
			case 601:
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
			case 301:
				return std::nullopt;
			case 502:
				return std::nullopt;
			case 600:
				return std::nullopt;
			case 601:
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
			case 301:
				return std::nullopt;
			case 502:
				return std::nullopt;
			case 600:
				return std::nullopt;
			case 601:
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
			case 301:
				return std::nullopt;
			case 502:
				return std::nullopt;
			case 600:
				return std::nullopt;
			case 601:
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
			case 301:
				return std::nullopt;
			case 502:
				return std::nullopt;
			case 600:
				return std::nullopt;
			case 601:
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
			case 301:
				return std::nullopt;
			case 502:
				return std::nullopt;
			case 600:
				return std::nullopt;
			case 601:
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
			case 301:
				return std::nullopt;
			case 502:
				return std::nullopt;
			case 600:
				return std::nullopt;
			case 601:
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
			case 301:
				return std::nullopt;
			case 502:
				return std::nullopt;
			case 600:
				return std::nullopt;
			case 601:
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
				return {{1000.0000000f, -368.5367432f}};
			case 205:
				return {{1000.0000000f, -368.5361328f}};
			case 206:
				return {{1000.0000000f, -368.5202637f}};
			case 207:
				return {{1000.0000000f, -368.5139160f}};
			case 208:
				return {{1000.0000000f, -368.5206299f}};
			case 209:
				return {{1000.0000000f, -368.5202637f}};
			case 210:
				return {{1000.0000000f, -368.5144043f}};
			case 211:
				return {{1000.0000000f, -368.5140381f}};
			case 212:
				return {{1000.0000000f, -368.5090332f}};
			case 213:
				return {{1000.0000000f, -368.5159912f}};
			case 214:
				return {{1000.0000000f, -368.5122070f}};
			case 215:
				return {{1000.0000000f, -368.5192871f}};
			case 216:
				return {{1000.0000000f, -368.5156250f}};
			case 217:
				return {{1000.0000000f, -368.5225830f}};
			case 218:
				return {{1000.0000000f, -368.5196533f}};
			case 219:
				return {{1000.0000000f, -368.3007812f}};
			case 220:
				return {{1000.0000000f, -368.2983398f}};
			case 221:
				return {{1000.0000000f, -368.5385742f}};
			case 222:
				return {{1000.0000000f, -368.5366211f}};
			case 223:
				return {{1000.0000000f, -368.5509033f}};
			case 224:
				return {{1000.0000000f, -368.5495605f}};
			case 225:
				return {{1000.0000000f, -368.3308105f}};
			case 226:
				return {{1000.0000000f, -368.3294678f}};
			case 227:
				return {{1000.0000000f, -368.3439941f}};
			case 228:
				return {{1000.0000000f, -368.3427734f}};
			case 229:
				return {{1000.0000000f, -368.5904541f}};
			case 230:
				return {{1000.0000000f, -368.5894775f}};
			case 231:
				return {{1000.0000000f, -368.3917236f}};
			case 232:
				return {{1000.0000000f, -368.4061279f}};
			case 233:
				return {{1000.0000000f, -368.4061279f}};
			case 234:
				return {{1000.0000000f, -368.4058838f}};
			case 235:
				return {{1000.0000000f, -368.4200439f}};
			case 236:
				return {{1000.0000000f, -368.6680908f}};
			case 237:
				return {{1000.0000000f, -368.4416504f}};
			case 238:
				return {{1000.0000000f, -368.4561768f}};
			case 239:
				return {{1000.0000000f, -368.4559326f}};
			case 240:
				return {{1000.0000000f, -368.4703369f}};
			case 241:
				return {{1000.0000000f, -368.4700928f}};
			case 242:
				return {{1000.0000000f, -368.4919434f}};
			case 243:
				return {{1000.0000000f, -368.4916992f}};
			case 244:
				return {{1000.0000000f, -368.5062256f}};
			case 245:
				return {{1000.0000000f, -368.5061035f}};
			case 246:
				return {{1000.0000000f, -368.2939453f}};
			case 247:
				return {{1000.0000000f, -368.2938232f}};
			case 248:
				return {{1000.0000000f, -368.5422363f}};
			case 249:
				return {{1000.0000000f, -368.5422363f}};
			case 250:
				return {{1000.0000000f, -368.3299561f}};
			case 251:
				return {{1000.0000000f, -368.3298340f}};
			case 252:
				return {{1000.0000000f, -368.5783691f}};
			case 253:
				return {{1000.0000000f, -368.5782471f}};
			case 254:
				return {{1000.0000000f, -368.5783691f}};
			case 255:
				return {{1000.0000000f, -368.6145020f}};
			case 256:
				return {{1000.0000000f, -368.4018555f}};
			case 257:
				return {{1000.0000000f, -368.4017334f}};
			case 258:
				return {{1000.0000000f, -368.6503906f}};
			case 259:
				return {{1000.0000000f, -368.4233398f}};
			case 260:
				return {{1000.0000000f, -368.4233398f}};
			case 261:
				return {{1000.0000000f, -368.4377441f}};
			case 262:
				return {{1000.0000000f, -368.4377441f}};
			case 263:
				return {{1000.0000000f, -368.4594727f}};
			case 264:
				return {{1000.0000000f, -368.4593506f}};
			case 265:
				return {{1000.0000000f, -368.4737549f}};
			case 266:
				return {{1000.0000000f, -368.4953613f}};
			case 267:
				return {{1000.0000000f, -368.4952393f}};
			case 268:
				return {{1000.0000000f, -368.5097656f}};
			case 269:
				return {{1000.0000000f, -368.5097656f}};
			case 270:
				return {{1000.0000000f, -368.5314941f}};
			case 271:
				return {{1000.0000000f, -368.5314941f}};
			case 272:
				return {{1000.0000000f, -368.3187256f}};
			case 273:
				return {{1000.0000000f, -368.3184814f}};
			case 274:
				return {{1000.0000000f, -368.5677490f}};
			case 275:
				return {{1000.0000000f, -368.5677490f}};
			case 276:
				return {{1000.0000000f, -368.5676270f}};
			case 277:
				return {{1000.0000000f, -368.3546143f}};
			case 278:
				return {{1000.0000000f, -368.5894775f}};
			case 279:
				return {{1000.0000000f, -368.5893555f}};
			case 280:
				return {{1000.0000000f, -368.3762207f}};
			case 281:
				return {{1000.0000000f, -368.6254883f}};
			case 282:
				return {{1000.0000000f, -368.4124756f}};
			case 283:
				return {{1000.0000000f, -368.4123535f}};
			case 284:
				return {{1000.0000000f, -368.4122314f}};
			case 285:
				return {{1000.0000000f, -368.4339600f}};
			case 286:
				return {{1000.0000000f, -368.4340820f}};
			case 287:
				return {{1000.0000000f, -368.4339600f}};
			case 288:
				return {{1000.0000000f, -368.4556885f}};
			case 289:
				return {{1000.0000000f, -368.4558105f}};
			case 290:
				return {{1000.0000000f, -368.4558105f}};
			case 291:
				return {{1000.0000000f, -368.4776611f}};
			case 292:
				return {{1000.0000000f, -368.4774170f}};
			case 293:
				return {{1000.0000000f, -368.4774170f}};
			case 294:
				return {{1000.0000000f, -368.4992676f}};
			case 295:
				return {{1000.0000000f, -368.4993896f}};
			case 296:
				return {{1000.0000000f, -368.5211182f}};
			case 297:
				return {{1000.0000000f, -368.5211182f}};
			case 298:
				return {{1000.0000000f, -368.5209961f}};
			case 299:
				return {{1000.0000000f, -368.5428467f}};
			case 300:
				return {{1000.0000000f, -368.3149414f}};
			case 301:
				return {{1000.0000000f, -368.3145752f}};
			case 302:
				return {{1000.0000000f, -368.3365479f}};
			case 303:
				return {{1000.0000000f, -368.3363037f}};
			case 304:
				return {{1000.0000000f, -368.3581543f}};
			case 305:
				return {{1000.0000000f, -368.5936279f}};
			case 306:
				return {{1000.0000000f, -368.5935059f}};
			case 307:
				return {{1000.0000000f, -368.3798828f}};
			case 308:
				return {{1000.0000000f, -368.6153564f}};
			case 309:
				return {{1000.0000000f, -368.4014893f}};
			case 310:
				return {{1000.0000000f, -368.4234619f}};
			case 311:
				return {{1000.0000000f, -368.4233398f}};
			case 312:
				return {{1000.0000000f, -368.4378662f}};
			case 313:
				return {{1000.0000000f, -368.4377441f}};
			case 314:
				return {{1000.0000000f, -368.4376221f}};
			case 315:
				return {{1000.0000000f, -368.4521484f}};
			case 316:
				return {{1000.0000000f, -368.4594727f}};
			case 317:
				return {{1000.0000000f, -368.4594727f}};
			case 318:
				return {{1000.0000000f, -368.4739990f}};
			case 319:
				return {{1000.0000000f, -368.4739990f}};
			case 320:
				return {{1000.0000000f, -368.4813232f}};
			case 321:
				return {{1000.0000000f, -368.4813232f}};
			case 322:
				return {{1000.0000000f, -368.4812012f}};
			case 323:
				return {{1000.0000000f, -368.4958496f}};
			case 324:
				return {{1000.0000000f, -368.4958496f}};
			case 325:
				return {{1000.0000000f, -368.4957275f}};
			case 326:
				return {{1000.0000000f, -368.5030518f}};
			case 327:
				return {{1000.0000000f, -368.5030518f}};
			case 328:
				return {{1000.0000000f, -368.5031738f}};
			case 329:
				return {{1000.0000000f, -368.5103760f}};
			case 330:
				return {{1000.0000000f, -368.5103760f}};
			case 331:
				return {{1000.0000000f, -368.5103760f}};
			case 332:
				return {{1000.0000000f, -368.5177002f}};
			case 333:
				return {{1000.0000000f, -368.5177002f}};
			case 334:
				return {{1000.0000000f, -368.5177002f}};
			case 335:
				return {{1000.0000000f, -368.5250244f}};
			case 336:
				return {{1000.0000000f, -368.5251465f}};
			case 337:
				return {{1000.0000000f, -368.5250244f}};
			case 338:
				return {{1000.0000000f, -368.5249023f}};
			case 339:
				return {{1000.0000000f, -368.2962646f}};
			case 340:
				return {{1000.0000000f, -368.2963867f}};
			case 341:
				return {{1000.0000000f, -368.5323486f}};
			case 342:
				return {{1000.0000000f, -368.5322266f}};
			case 343:
				return {{1000.0000000f, -368.5322266f}};
			case 344:
				return {{1000.0000000f, -368.3035889f}};
			case 345:
				return {{1000.0000000f, -368.3035889f}};
			case 346:
				return {{1000.0000000f, -368.3035889f}};
			case 347:
				return {{1000.0000000f, -368.5396729f}};
			case 348:
				return {{1000.0000000f, -368.5394287f}};
			case 349:
				return {{1000.0000000f, -368.5394287f}};
			case 350:
				return {{1000.0000000f, -368.5395508f}};
			case 351:
				return {{1000.0000000f, -368.5395508f}};
			case 352:
				return {{1000.0000000f, -368.5395508f}};
			case 353:
				return {{1000.0000000f, -368.5394287f}};
			case 354:
				return {{1000.0000000f, -368.5394287f}};
			case 355:
				return {{1000.0000000f, -368.5394287f}};
			case 356:
				return {{1000.0000000f, -368.5394287f}};
			case 357:
				return {{1000.0000000f, -368.5393066f}};
			case 358:
				return {{1000.0000000f, -368.5391846f}};
			case 359:
				return {{1000.0000000f, -368.5393066f}};
			case 360:
				return {{1000.0000000f, -368.5393066f}};
			case 361:
				return {{1000.0000000f, -368.5394287f}};
			case 362:
				return {{1000.0000000f, -368.5395508f}};
			case 363:
				return {{1000.0000000f, -368.5394287f}};
			case 364:
				return {{1000.0000000f, -368.5394287f}};
			case 365:
				return {{1000.0000000f, -368.5394287f}};
			case 366:
				return {{1000.0000000f, -368.5394287f}};
			case 367:
				return {{1000.0000000f, -368.5394287f}};
			case 368:
				return {{1000.0000000f, -368.5394287f}};
			case 369:
				return {{1000.0000000f, -368.5394287f}};
			case 370:
				return {{1000.0000000f, -368.5393066f}};
			case 371:
				return {{1000.0000000f, -368.5394287f}};
			case 372:
				return {{1000.0000000f, -368.5393066f}};
			case 373:
				return {{1000.0000000f, -368.5393066f}};
			case 374:
				return {{1000.0000000f, -368.5393066f}};
			case 375:
				return {{1000.0000000f, -368.5393066f}};
			case 376:
				return {{1000.0000000f, -368.5393066f}};
			case 377:
				return {{1000.0000000f, -368.5393066f}};
			case 378:
				return {{1000.0000000f, -368.5393066f}};
			case 379:
				return {{1000.0000000f, -368.5393066f}};
			case 380:
				return {{1000.0000000f, -368.5390625f}};
			case 381:
				return {{1000.0000000f, -368.5390625f}};
			case 382:
				return {{1000.0000000f, -368.5391846f}};
			case 383:
				return {{1000.0000000f, -368.5390625f}};
			case 384:
				return {{1000.0000000f, -368.5391846f}};
			case 385:
				return {{1000.0000000f, -368.5391846f}};
			case 386:
				return {{1000.0000000f, -368.5391846f}};
			case 387:
				return {{1000.0000000f, -368.5391846f}};
			case 388:
				return {{1000.0000000f, -368.5391846f}};
			case 389:
				return {{1000.0000000f, -368.5391846f}};
			case 390:
				return {{1000.0000000f, -368.5393066f}};
			case 391:
				return {{1000.0000000f, -368.5391846f}};
			case 392:
				return {{1000.0000000f, -368.5393066f}};
			case 393:
				return {{1000.0000000f, -368.5393066f}};
			case 394:
				return {{1000.0000000f, -368.5393066f}};
			case 395:
				return {{1000.0000000f, -368.5393066f}};
			case 396:
				return {{1000.0000000f, -368.5393066f}};
			case 397:
				return {{1000.0000000f, -368.5393066f}};
			case 398:
				return {{1000.0000000f, -368.5393066f}};
			case 399:
				return {{1000.0000000f, -368.5394287f}};
			case 400:
				return {{1000.0000000f, -368.5394287f}};
			case 401:
				return {{1000.0000000f, -368.5394287f}};
			case 402:
				return {{1000.0000000f, -368.5394287f}};
			case 403:
				return {{1000.0000000f, -368.5393066f}};
			case 404:
				return {{1000.0000000f, -368.5394287f}};
			case 405:
				return {{1000.0000000f, -368.5394287f}};
			case 406:
				return {{1000.0000000f, -368.5394287f}};
			case 407:
				return {{1000.0000000f, -368.5395508f}};
			case 408:
				return {{1000.0000000f, -368.5395508f}};
			case 409:
				return {{1000.0000000f, -368.5395508f}};
			case 410:
				return {{1000.0000000f, -368.5395508f}};
			case 411:
				return {{1000.0000000f, -368.5395508f}};
			case 412:
				return {{1000.0000000f, -368.5395508f}};
			case 413:
				return {{1000.0000000f, -368.5395508f}};
			case 414:
				return {{1000.0000000f, -368.5395508f}};
			case 415:
				return {{1000.0000000f, -368.5395508f}};
			case 416:
				return {{1000.0000000f, -368.5395508f}};
			case 417:
				return {{1000.0000000f, -368.5396729f}};
			case 418:
				return {{1000.0000000f, -368.5396729f}};
			case 419:
				return {{1000.0000000f, -368.5396729f}};
			case 420:
				return {{1000.0000000f, -368.5396729f}};
			case 421:
				return {{1000.0000000f, -368.5396729f}};
			case 422:
				return {{1000.0000000f, -368.5396729f}};
			case 423:
				return {{1000.0000000f, -368.5396729f}};
			case 424:
				return {{1000.0000000f, -368.5396729f}};
			case 425:
				return {{1000.0000000f, -368.5396729f}};
			case 426:
				return {{1000.0000000f, -368.5396729f}};
			case 427:
				return {{1000.0000000f, -368.5396729f}};
			case 428:
				return {{1000.0000000f, -368.5396729f}};
			case 429:
				return {{1000.0000000f, -368.5396729f}};
			case 430:
				return {{1000.0000000f, -368.5396729f}};
			case 431:
				return {{1000.0000000f, -368.5396729f}};
			case 432:
				return {{1000.0000000f, -368.5396729f}};
			case 433:
				return {{1000.0000000f, -368.5396729f}};
			case 434:
				return {{1000.0000000f, -368.5396729f}};
			case 435:
				return {{1000.0000000f, -368.5396729f}};
			case 436:
				return {{1000.0000000f, -368.5396729f}};
			case 437:
				return {{1000.0000000f, -368.5396729f}};
			case 438:
				return {{1000.0000000f, -368.5396729f}};
			case 439:
				return {{1000.0000000f, -368.5395508f}};
			case 440:
				return {{1000.0000000f, -368.5395508f}};
			case 441:
				return {{1000.0000000f, -368.5395508f}};
			case 442:
				return {{1000.0000000f, -368.5395508f}};
			case 443:
				return {{1000.0000000f, -368.5395508f}};
			case 444:
				return {{1000.0000000f, -368.5395508f}};
			case 445:
				return {{1000.0000000f, -368.5395508f}};
			case 446:
				return {{1000.0000000f, -368.5395508f}};
			case 447:
				return {{1000.0000000f, -368.5395508f}};
			case 448:
				return {{1000.0000000f, -368.5395508f}};
			case 449:
				return {{1000.0000000f, -368.5395508f}};
			case 450:
				return {{1000.0000000f, -368.5395508f}};
			case 451:
				return {{1000.0000000f, -368.5395508f}};
			case 452:
				return {{1000.0000000f, -368.5395508f}};
			case 453:
				return {{1000.0000000f, -368.5395508f}};
			case 454:
				return {{1000.0000000f, -368.5395508f}};
			case 455:
				return {{1000.0000000f, -368.5395508f}};
			case 456:
				return {{1000.0000000f, -368.5395508f}};
			case 457:
				return {{1000.0000000f, -368.5395508f}};
			case 458:
				return {{1000.0000000f, -368.5395508f}};
			case 459:
				return {{1000.0000000f, -368.5395508f}};
			case 460:
				return {{1000.0000000f, -368.5395508f}};
			case 461:
				return {{1000.0000000f, -368.5395508f}};
			case 462:
				return {{1000.0000000f, -368.5395508f}};
			case 463:
				return {{1000.0000000f, -368.5395508f}};
			case 464:
				return {{1000.0000000f, -368.5395508f}};
			case 465:
				return {{1000.0000000f, -368.5395508f}};
			case 466:
				return {{1000.0000000f, -368.5395508f}};
			case 467:
				return {{1000.0000000f, -368.5395508f}};
			case 468:
				return {{1000.0000000f, -368.5395508f}};
			case 469:
				return {{1000.0000000f, -368.5395508f}};
			case 470:
				return {{1000.0000000f, -368.5395508f}};
			case 471:
				return {{1000.0000000f, -368.5395508f}};
			case 472:
				return {{1000.0000000f, -368.5395508f}};
			case 473:
				return {{1000.0000000f, -368.5395508f}};
			case 474:
				return {{1000.0000000f, -368.5395508f}};
			case 475:
				return {{1000.0000000f, -368.5395508f}};
			case 476:
				return {{1000.0000000f, -368.5395508f}};
			case 477:
				return {{1000.0000000f, -368.5395508f}};
			case 478:
				return {{1000.0000000f, -368.5395508f}};
			case 479:
				return {{1000.0000000f, -368.5395508f}};
			case 480:
				return {{1000.0000000f, -368.5395508f}};
			case 481:
				return {{1000.0000000f, -368.5395508f}};
			case 482:
				return {{1000.0000000f, -368.5395508f}};
			case 483:
				return {{1000.0000000f, -368.5395508f}};
			case 484:
				return {{1000.0000000f, -368.5395508f}};
			case 485:
				return {{1000.0000000f, -368.5395508f}};
			case 486:
				return {{1000.0000000f, -368.5395508f}};
			case 487:
				return {{1000.0000000f, -368.5395508f}};
			case 488:
				return {{1000.0000000f, -368.5395508f}};
			case 489:
				return {{1000.0000000f, -368.5395508f}};
			case 490:
				return {{1000.0000000f, -368.5395508f}};
			case 491:
				return {{1000.0000000f, -368.5395508f}};
			case 492:
				return {{1000.0000000f, -368.5395508f}};
			case 493:
				return {{1000.0000000f, -368.5395508f}};
			case 494:
				return {{1000.0000000f, -368.5395508f}};
			case 495:
				return {{1000.0000000f, -368.5395508f}};
			case 496:
				return {{1000.0000000f, -368.5395508f}};
			case 497:
				return {{1000.0000000f, -368.5395508f}};
			case 498:
				return {{1000.0000000f, -368.5395508f}};
			case 499:
				return {{1000.0000000f, -368.5395508f}};
			case 500:
				return {{1000.0000000f, -368.5395508f}};
			case 501:
				return {{1000.0000000f, -368.5395508f}};
			case 502:
				return {{1000.0000000f, -368.5395508f}};
			case 503:
				return {{1000.0000000f, -368.5395508f}};
			case 504:
				return {{1000.0000000f, -368.5394287f}};
			case 505:
				return {{1000.0000000f, -368.5394287f}};
			case 506:
				return {{1000.0000000f, -368.5394287f}};
			case 507:
				return {{1000.0000000f, -368.5394287f}};
			case 508:
				return {{1000.0000000f, -368.5394287f}};
			case 509:
				return {{1000.0000000f, -368.3034668f}};
			case 510:
				return {{1000.0000000f, -368.3035889f}};
			case 511:
				return {{1000.0000000f, -368.5321045f}};
			case 512:
				return {{1000.0000000f, -368.5321045f}};
			case 513:
				return {{1000.0000000f, -368.5247803f}};
			case 514:
				return {{1000.0000000f, -368.5247803f}};
			case 515:
				return {{1000.0000000f, -368.5247803f}};
			case 516:
				return {{1000.0000000f, -368.5249023f}};
			case 517:
				return {{1000.0000000f, -368.5174561f}};
			case 518:
				return {{1000.0000000f, -368.5175781f}};
			case 519:
				return {{1000.0000000f, -368.5101318f}};
			case 520:
				return {{1000.0000000f, -368.5102539f}};
			case 521:
				return {{1000.0000000f, -368.5101318f}};
			case 522:
				return {{1000.0000000f, -368.4956055f}};
			case 523:
				return {{1000.0000000f, -368.4956055f}};
			case 524:
				return {{1000.0000000f, -368.4954834f}};
			case 525:
				return {{1000.0000000f, -368.4808350f}};
			case 526:
				return {{1000.0000000f, -368.4809570f}};
			case 527:
				return {{1000.0000000f, -368.4808350f}};
			case 528:
				return {{1000.0000000f, -368.4663086f}};
			case 529:
				return {{1000.0000000f, -368.4663086f}};
			case 530:
				return {{1000.0000000f, -368.4661865f}};
			case 531:
				return {{1000.0000000f, -368.4442139f}};
			case 532:
				return {{1000.0000000f, -368.4443359f}};
			case 533:
				return {{1000.0000000f, -368.4442139f}};
			case 534:
				return {{1000.0000000f, -368.6578369f}};
			case 535:
				return {{1000.0000000f, -368.4222412f}};
			case 536:
				return {{1000.0000000f, -368.4221191f}};
			case 537:
				return {{1000.0000000f, -368.3856201f}};
			case 538:
				return {{1000.0000000f, -368.6135254f}};
			case 539:
				return {{1000.0000000f, -368.3636475f}};
			case 540:
				return {{1000.0000000f, -368.5916748f}};
			case 541:
				return {{1000.0000000f, -368.5915527f}};
			case 542:
				return {{1000.0000000f, -368.5697021f}};
			case 543:
				return {{1000.0000000f, -368.5697021f}};
			case 544:
				return {{1000.0000000f, -368.5694580f}};
			case 545:
				return {{1000.0000000f, -368.5477295f}};
			case 546:
				return {{1000.0000000f, -368.3125000f}};
			case 547:
				return {{1000.0000000f, -368.3125000f}};
			case 548:
				return {{1000.0000000f, -368.2906494f}};
			case 549:
				return {{1000.0000000f, -368.2906494f}};
			case 550:
				return {{1000.0000000f, -368.5184326f}};
			case 551:
				return {{1000.0000000f, -368.4964600f}};
			case 552:
				return {{1000.0000000f, -368.4963379f}};
			case 553:
				return {{1000.0000000f, -368.4744873f}};
			case 554:
				return {{1000.0000000f, -368.4744873f}};
			case 555:
				return {{1000.0000000f, -368.4526367f}};
			case 556:
				return {{1000.0000000f, -368.4453125f}};
			case 557:
				return {{1000.0000000f, -368.4451904f}};
			case 558:
				return {{1000.0000000f, -368.4233398f}};
			case 559:
				return {{1000.0000000f, -368.4234619f}};
			case 560:
				return {{1000.0000000f, -368.4233398f}};
			case 561:
				return {{1000.0000000f, -368.6290283f}};
			case 562:
				return {{1000.0000000f, -368.3942871f}};
			case 563:
				return {{1000.0000000f, -368.3942871f}};
			case 564:
				return {{1000.0000000f, -368.3724365f}};
			case 565:
				return {{1000.0000000f, -368.3724365f}};
			case 566:
				return {{1000.0000000f, -368.5996094f}};
			case 567:
				return {{1000.0000000f, -368.3431396f}};
			case 568:
				return {{1000.0000000f, -368.3431396f}};
			case 569:
				return {{1000.0000000f, -368.3212891f}};
			case 570:
				return {{1000.0000000f, -368.5264893f}};
			case 571:
				return {{1000.0000000f, -368.5046387f}};
			case 572:
				return {{1000.0000000f, -368.5046387f}};
			case 573:
				return {{1000.0000000f, -368.2702637f}};
			case 574:
				return {{1000.0000000f, -368.2484131f}};
			case 575:
				return {{1000.0000000f, -368.4754639f}};
			case 576:
				return {{1000.0000000f, -368.4754639f}};
			case 577:
				return {{1000.0000000f, -368.4536133f}};
			case 578:
				return {{1000.0000000f, -368.4536133f}};
			case 579:
				return {{1000.0000000f, -368.4534912f}};
			case 580:
				return {{1000.0000000f, -368.6589355f}};
			case 581:
				return {{1000.0000000f, -368.4245605f}};
			case 582:
				return {{1000.0000000f, -368.4245605f}};
			case 583:
				return {{1000.0000000f, -368.4027100f}};
			case 584:
				return {{1000.0000000f, -368.4027100f}};
			case 585:
				return {{1000.0000000f, -368.3809814f}};
			case 586:
				return {{1000.0000000f, -368.6077881f}};
			case 587:
				return {{1000.0000000f, -368.6076660f}};
			case 588:
				return {{1000.0000000f, -368.5858154f}};
			case 589:
				return {{1000.0000000f, -368.3518066f}};
			case 590:
				return {{1000.0000000f, -368.3516846f}};
			case 591:
				return {{1000.0000000f, -368.5567627f}};
			case 592:
				return {{1000.0000000f, -368.5567627f}};
			case 593:
				return {{1000.0000000f, -368.5566406f}};
			case 594:
				return {{1000.0000000f, -368.3010254f}};
			case 595:
				return {{1000.0000000f, -368.3011475f}};
			case 596:
				return {{1000.0000000f, -368.5274658f}};
			case 597:
				return {{1000.0000000f, -368.5057373f}};
			case 598:
				return {{1000.0000000f, -368.5057373f}};
			case 599:
				return {{1000.0000000f, -368.5056152f}};
			case 600:
				return {{1000.0000000f, -368.4766846f}};
			case 601:
				return {{1000.0000000f, -368.4765625f}};
			case 602:
				return {{1000.0000000f, -368.4332275f}};
			case 603:
				return {{1000.0000000f, -368.4334717f}};
			case 604:
				return {{1000.0000000f, -368.4117432f}};
			case 605:
				return {{1000.0000000f, -368.6381836f}};
			case 606:
				return {{1000.0000000f, -368.3905029f}};
			case 607:
				return {{1000.0000000f, -368.3905029f}};
			case 608:
				return {{1000.0000000f, -368.6170654f}};
			case 609:
				return {{1000.0000000f, -368.3693848f}};
			case 610:
				return {{1000.0000000f, -368.3701172f}};
			case 611:
				return {{1000.0000000f, -368.5819092f}};
			case 612:
				return {{1000.0000000f, -368.3492432f}};
			case 613:
				return {{1000.0000000f, -368.3348389f}};
			case 614:
				return {{1000.0000000f, -368.3355713f}};
			case 615:
				return {{1000.0000000f, -368.3210449f}};
			case 616:
				return {{1000.0000000f, -368.3211670f}};
			case 617:
				return {{1000.0000000f, -368.5477295f}};
			case 618:
				return {{1000.0000000f, -368.3074951f}};
			case 619:
				return {{1000.0000000f, -368.3076172f}};
			case 620:
				return {{1000.0000000f, -368.3083496f}};
			case 621:
				return {{1000.0000000f, -368.5269775f}};
			case 622:
				return {{1000.0000000f, -368.5275879f}};
			case 623:
				return {{1000.0000000f, -368.2873535f}};
			case 624:
				return {{1000.0000000f, -368.2873535f}};
			case 625:
				return {{1000.0000000f, -368.2878418f}};
			case 626:
				return {{1000.0000000f, -368.2806396f}};
			case 627:
				return {{1000.0000000f, -368.5064697f}};
			case 628:
				return {{1000.0000000f, -368.5073242f}};
			case 629:
				return {{1000.0000000f, -368.5001221f}};
			case 630:
				return {{1000.0000000f, -368.5006104f}};
			case 631:
				return {{1000.0000000f, -368.4932861f}};
			case 632:
				return {{1000.0000000f, -368.4934082f}};
			case 633:
				return {{1000.0000000f, -368.4936523f}};
			case 634:
				return {{1000.0000000f, -368.4864502f}};
			case 635:
				return {{1000.0000000f, -368.4865723f}};
			case 636:
				return {{1000.0000000f, -368.4865723f}};
			case 637:
				return {{1000.0000000f, -368.4865723f}};
			case 638:
				return {{1000.0000000f, -368.4868164f}};
			case 639:
				return {{1000.0000000f, -368.4869385f}};
			case 640:
				return {{1000.0000000f, -368.4868164f}};
			case 641:
				return {{1000.0000000f, -368.4871826f}};
			case 642:
				return {{1000.0000000f, -368.4799805f}};
			case 643:
				return {{1000.0000000f, -368.4801025f}};
			case 644:
				return {{1000.0000000f, -368.4801025f}};
			case 645:
				return {{1000.0000000f, -368.4801025f}};
			case 646:
				return {{1000.0000000f, -368.4803467f}};
			case 647:
				return {{1000.0000000f, -368.4803467f}};
			case 648:
				return {{1000.0000000f, -368.4803467f}};
			case 649:
				return {{1000.0000000f, -368.4804688f}};
			case 650:
				return {{1000.0000000f, -368.4804688f}};
			case 651:
				return {{1000.0000000f, -368.4804688f}};
			case 652:
				return {{1000.0000000f, -368.4805908f}};
			case 653:
				return {{1000.0000000f, -368.4805908f}};
			case 654:
				return {{1000.0000000f, -368.4805908f}};
			case 655:
				return {{1000.0000000f, -368.4805908f}};
			case 656:
				return {{1000.0000000f, -368.4805908f}};
			case 657:
				return {{1000.0000000f, -368.4805908f}};
			case 658:
				return {{1000.0000000f, -368.4805908f}};
			case 659:
				return {{1000.0000000f, -368.4805908f}};
			case 660:
				return {{1000.0000000f, -368.4805908f}};
			case 661:
				return {{1000.0000000f, -368.4805908f}};
			case 662:
				return {{1000.0000000f, -368.4805908f}};
			case 663:
				return {{1000.0000000f, -368.4805908f}};
			case 664:
				return {{1000.0000000f, -368.4805908f}};
			case 665:
				return {{1000.0000000f, -368.4805908f}};
			case 666:
				return {{1000.0000000f, -368.4805908f}};
			case 667:
				return {{1000.0000000f, -368.4805908f}};
			case 668:
				return {{1000.0000000f, -368.4805908f}};
			case 669:
				return {{1000.0000000f, -368.4805908f}};
			case 670:
				return {{1000.0000000f, -368.4805908f}};
			case 671:
				return {{1000.0000000f, -368.4805908f}};
			case 672:
				return {{1000.0000000f, -368.4805908f}};
			case 673:
				return {{1000.0000000f, -368.4805908f}};
			case 674:
				return {{1000.0000000f, -368.4807129f}};
			case 675:
				return {{1000.0000000f, -368.4808350f}};
			case 676:
				return {{1000.0000000f, -368.4808350f}};
			case 677:
				return {{1000.0000000f, -368.4809570f}};
			case 678:
				return {{1000.0000000f, -368.4810791f}};
			case 679:
				return {{1000.0000000f, -368.4810791f}};
			case 680:
				return {{1000.0000000f, -368.4809570f}};
			case 681:
				return {{1000.0000000f, -368.4809570f}};
			case 682:
				return {{1000.0000000f, -368.4809570f}};
			case 683:
				return {{1000.0000000f, -368.4809570f}};
			case 684:
				return {{1000.0000000f, -368.4809570f}};
			case 685:
				return {{1000.0000000f, -368.4809570f}};
			case 686:
				return {{1000.0000000f, -368.4809570f}};
			case 687:
				return {{1000.0000000f, -368.4809570f}};
			case 688:
				return {{1000.0000000f, -368.4809570f}};
			case 689:
				return {{1000.0000000f, -368.4809570f}};
			case 690:
				return {{1000.0000000f, -368.4808350f}};
			case 691:
				return {{1000.0000000f, -368.4808350f}};
			case 692:
				return {{1000.0000000f, -368.4808350f}};
			case 693:
				return {{1000.0000000f, -368.4807129f}};
			case 694:
				return {{1000.0000000f, -368.4808350f}};
			case 695:
				return {{1000.0000000f, -368.4808350f}};
			case 696:
				return {{1000.0000000f, -368.4808350f}};
			case 697:
				return {{1000.0000000f, -368.4808350f}};
			case 698:
				return {{1000.0000000f, -368.4807129f}};
			case 699:
				return {{1000.0000000f, -368.4807129f}};
			case 700:
				return {{1000.0000000f, -368.4807129f}};
			case 701:
				return {{1000.0000000f, -368.4807129f}};
			case 702:
				return {{1000.0000000f, -368.4805908f}};
			case 703:
				return {{1000.0000000f, -368.4807129f}};
			case 704:
				return {{1000.0000000f, -368.4807129f}};
			case 705:
				return {{1000.0000000f, -368.4805908f}};
			case 706:
				return {{1000.0000000f, -368.4805908f}};
			case 707:
				return {{1000.0000000f, -368.4805908f}};
			case 708:
				return {{1000.0000000f, -368.4805908f}};
			case 709:
				return {{1000.0000000f, -368.4805908f}};
			case 710:
				return {{1000.0000000f, -368.4805908f}};
			case 711:
				return {{1000.0000000f, -368.4805908f}};
			case 712:
				return {{1000.0000000f, -368.4805908f}};
			case 713:
				return {{1000.0000000f, -368.4805908f}};
			case 714:
				return {{1000.0000000f, -368.4805908f}};
			case 715:
				return {{1000.0000000f, -368.4805908f}};
			case 716:
				return {{1000.0000000f, -368.4805908f}};
			case 717:
				return {{1000.0000000f, -368.4805908f}};
			case 718:
				return {{1000.0000000f, -368.4805908f}};
			case 719:
				return {{1000.0000000f, -368.4805908f}};
			case 720:
				return {{1000.0000000f, -368.4805908f}};
			case 721:
				return {{1000.0000000f, -368.4805908f}};
			case 722:
				return {{1000.0000000f, -368.4805908f}};
			case 723:
				return {{1000.0000000f, -368.4805908f}};
			case 724:
				return {{1000.0000000f, -368.4805908f}};
			case 725:
				return {{1000.0000000f, -368.4804688f}};
			case 726:
				return {{1000.0000000f, -368.4804688f}};
			case 727:
				return {{1000.0000000f, -368.4804688f}};
			case 728:
				return {{1000.0000000f, -368.4804688f}};
			case 729:
				return {{1000.0000000f, -368.4804688f}};
			case 730:
				return {{1000.0000000f, -368.4804688f}};
			case 731:
				return {{1000.0000000f, -368.4804688f}};
			case 732:
				return {{1000.0000000f, -368.4804688f}};
			case 733:
				return {{1000.0000000f, -368.4804688f}};
			case 734:
				return {{1000.0000000f, -368.4804688f}};
			case 735:
				return {{1000.0000000f, -368.4804688f}};
			case 736:
				return {{1000.0000000f, -368.4804688f}};
			case 737:
				return {{1000.0000000f, -368.4804688f}};
			case 738:
				return {{1000.0000000f, -368.4804688f}};
			case 739:
				return {{1000.0000000f, -368.4804688f}};
			case 740:
				return {{1000.0000000f, -368.4804688f}};
			case 741:
				return {{1000.0000000f, -368.4804688f}};
			case 742:
				return {{1000.0000000f, -368.4804688f}};
			case 743:
				return {{1000.0000000f, -368.4804688f}};
			case 744:
				return {{1000.0000000f, -368.4804688f}};
			case 745:
				return {{1000.0000000f, -368.4804688f}};
			case 746:
				return {{1000.0000000f, -368.4804688f}};
			case 747:
				return {{1000.0000000f, -368.4804688f}};
			case 748:
				return {{1000.0000000f, -368.4804688f}};
			case 749:
				return {{1000.0000000f, -368.4804688f}};
			case 750:
				return {{1000.0000000f, -368.4804688f}};
			case 751:
				return {{1000.0000000f, -368.4804688f}};
			case 752:
				return {{1000.0000000f, -368.4804688f}};
			case 753:
				return {{1000.0000000f, -368.4804688f}};
			case 754:
				return {{1000.0000000f, -368.4804688f}};
			case 755:
				return {{1000.0000000f, -368.4804688f}};
			case 756:
				return {{1000.0000000f, -368.4804688f}};
			case 757:
				return {{1000.0000000f, -368.4804688f}};
			case 758:
				return {{1000.0000000f, -368.4804688f}};
			case 759:
				return {{1000.0000000f, -368.4804688f}};
			case 760:
				return {{1000.0000000f, -368.4804688f}};
			case 761:
				return {{1000.0000000f, -368.4804688f}};
			case 762:
				return {{1000.0000000f, -368.4804688f}};
			case 763:
				return {{1000.0000000f, -368.4804688f}};
			case 764:
				return {{1000.0000000f, -368.4804688f}};
			case 765:
				return {{1000.0000000f, -368.4804688f}};
			case 766:
				return {{1000.0000000f, -368.4804688f}};
			case 767:
				return {{1000.0000000f, -368.4804688f}};
			case 768:
				return {{1000.0000000f, -368.4804688f}};
			case 769:
				return {{1000.0000000f, -368.4804688f}};
			case 770:
				return {{1000.0000000f, -368.4804688f}};
			case 771:
				return {{1000.0000000f, -368.4804688f}};
			case 772:
				return {{1000.0000000f, -368.4804688f}};
			case 773:
				return {{1000.0000000f, -368.4804688f}};
			case 774:
				return {{1000.0000000f, -368.4804688f}};
			case 775:
				return {{1000.0000000f, -368.4804688f}};
			case 776:
				return {{1000.0000000f, -368.4804688f}};
			case 777:
				return {{1000.0000000f, -368.4804688f}};
			case 778:
				return {{1000.0000000f, -368.4804688f}};
			case 779:
				return {{1000.0000000f, -368.4804688f}};
			case 780:
				return {{1000.0000000f, -368.4804688f}};
			case 781:
				return {{1000.0000000f, -368.4804688f}};
			case 782:
				return {{1000.0000000f, -368.4804688f}};
			case 783:
				return {{1000.0000000f, -368.4804688f}};
			case 784:
				return {{1000.0000000f, -368.4804688f}};
			case 785:
				return {{1000.0000000f, -368.4804688f}};
			case 786:
				return {{1000.0000000f, -368.4804688f}};
			case 787:
				return {{1000.0000000f, -368.4804688f}};
			case 788:
				return {{1000.0000000f, -368.4804688f}};
			case 789:
				return {{1000.0000000f, -368.4804688f}};
			case 790:
				return {{1000.0000000f, -368.4804688f}};
			case 791:
				return {{1000.0000000f, -368.4804688f}};
			case 792:
				return {{1000.0000000f, -368.4804688f}};
			case 793:
				return {{1000.0000000f, -368.4804688f}};
			case 794:
				return {{1000.0000000f, -368.4804688f}};
			case 795:
				return {{1000.0000000f, -368.4804688f}};
			case 796:
				return {{1000.0000000f, -368.4804688f}};
			case 797:
				return {{1000.0000000f, -368.4804688f}};
			case 798:
				return {{1000.0000000f, -368.4804688f}};
			case 799:
				return {{1000.0000000f, -368.4804688f}};
			case 800:
				return {{1000.0000000f, -368.4804688f}};
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
			case 301:
				return {{1000.0000000f, 1114.1660156f}};
			case 502:
				return {{1000.0000000f, 1124.3378906f}};
			case 600:
				return {{1000.0000000f, 1086.9483643f}};
			case 601:
				return {{1000.0000000f, 1084.7178955f}};
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
			case 301:
				return {{1000.0000000f, 1137.1875000f}};
			case 502:
				return {{1000.0000000f, 1147.5131836f}};
			case 600:
				return {{1000.0000000f, 1109.5463867f}};
			case 601:
				return {{1000.0000000f, 1107.2849121f}};
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
			case 301:
				return {{1000.0000000f, 1144.7453613f}};
			case 502:
				return {{1000.0000000f, 1155.1259766f}};
			case 600:
				return {{1000.0000000f, 1116.9654541f}};
			case 601:
				return {{1000.0000000f, 1114.6922607f}};
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
			case 203:
				return {{1000.0000000f, 1108.6667480f}};
			case 204:
				return {{1000.0000000f, 1108.6829834f}};
			case 205:
				return {{1000.0000000f, 1108.6860352f}};
			case 206:
				return {{1000.0000000f, 1108.8454590f}};
			case 207:
				return {{1000.0000000f, 1108.9794922f}};
			case 208:
				return {{1000.0000000f, 1108.9899902f}};
			case 209:
				return {{1000.0000000f, 1109.0002441f}};
			case 210:
				return {{1000.0000000f, 1109.1987305f}};
			case 211:
				return {{1000.0000000f, 1109.2121582f}};
			case 212:
				return {{1000.0000000f, 1109.4829102f}};
			case 213:
				return {{1000.0000000f, 1109.5009766f}};
			case 214:
				return {{1000.0000000f, 1109.8237305f}};
			case 215:
				return {{1000.0000000f, 1109.8487549f}};
			case 216:
				return {{1000.0000000f, 1110.2625732f}};
			case 217:
				return {{1000.0000000f, 1110.2905273f}};
			case 218:
				return {{1000.0000000f, 1110.7784424f}};
			case 219:
				return {{1000.0000000f, 1110.8120117f}};
			case 220:
				return {{1000.0000000f, 1111.3294678f}};
			case 221:
				return {{1000.0000000f, 1111.3670654f}};
			case 222:
				return {{1000.0000000f, 1111.9794922f}};
			case 223:
				return {{1000.0000000f, 1112.0203857f}};
			case 224:
				return {{1000.0000000f, 1112.6472168f}};
			case 225:
				return {{1000.0000000f, 1112.6901855f}};
			case 226:
				return {{1000.0000000f, 1113.4086914f}};
			case 227:
				return {{1000.0000000f, 1113.4549561f}};
			case 228:
				return {{1000.0000000f, 1114.2186279f}};
			case 229:
				return {{1000.0000000f, 1114.2668457f}};
			case 230:
				return {{1000.0000000f, 1115.8420410f}};
			case 231:
				return {{1000.0000000f, 1116.6669922f}};
			case 232:
				return {{1000.0000000f, 1116.7172852f}};
			case 233:
				return {{1000.0000000f, 1116.7674561f}};
			case 234:
				return {{1000.0000000f, 1117.5385742f}};
			case 235:
				return {{1000.0000000f, 1118.3652344f}};
			case 236:
				return {{1000.0000000f, 1118.4162598f}};
			case 237:
				return {{1000.0000000f, 1119.1933594f}};
			case 238:
				return {{1000.0000000f, 1119.2453613f}};
			case 239:
				return {{1000.0000000f, 1120.0772705f}};
			case 240:
				return {{1000.0000000f, 1120.1302490f}};
			case 241:
				return {{1000.0000000f, 1120.9735107f}};
			case 242:
				return {{1000.0000000f, 1121.0260010f}};
			case 243:
				return {{1000.0000000f, 1121.8271484f}};
			case 244:
				return {{1000.0000000f, 1121.8811035f}};
			case 245:
				return {{1000.0000000f, 1122.7445068f}};
			case 246:
				return {{1000.0000000f, 1122.7996826f}};
			case 247:
				return {{1000.0000000f, 1123.6776123f}};
			case 248:
				return {{1000.0000000f, 1123.7324219f}};
			case 249:
				return {{1000.0000000f, 1124.5660400f}};
			case 250:
				return {{1000.0000000f, 1124.6213379f}};
			case 251:
				return {{1000.0000000f, 1125.5219727f}};
			case 252:
				return {{1000.0000000f, 1125.5795898f}};
			case 253:
				return {{1000.0000000f, 1125.6363525f}};
			case 254:
				return {{1000.0000000f, 1127.4281006f}};
			case 255:
				return {{1000.0000000f, 1128.3507080f}};
			case 256:
				return {{1000.0000000f, 1128.4062500f}};
			case 257:
				return {{1000.0000000f, 1129.2526855f}};
			case 258:
				return {{1000.0000000f, 1129.3081055f}};
			case 259:
				return {{1000.0000000f, 1129.3653564f}};
			case 260:
				return {{1000.0000000f, 1130.2619629f}};
			case 261:
				return {{1000.0000000f, 1130.3168945f}};
			case 262:
				return {{1000.0000000f, 1131.2221680f}};
			case 263:
				return {{1000.0000000f, 1131.2797852f}};
			case 264:
				return {{1000.0000000f, 1132.1304932f}};
			case 265:
				return {{1000.0000000f, 1133.0421143f}};
			case 266:
				return {{1000.0000000f, 1133.0996094f}};
			case 267:
				return {{1000.0000000f, 1133.9494629f}};
			case 268:
				return {{1000.0000000f, 1134.0053711f}};
			case 269:
				return {{1000.0000000f, 1134.9106445f}};
			case 270:
				return {{1000.0000000f, 1134.9681396f}};
			case 271:
				return {{1000.0000000f, 1135.8762207f}};
			case 272:
				return {{1000.0000000f, 1135.9327393f}};
			case 273:
				return {{1000.0000000f, 1136.7869873f}};
			case 274:
				return {{1000.0000000f, 1136.8436279f}};
			case 275:
				return {{1000.0000000f, 1136.9010010f}};
			case 276:
				return {{1000.0000000f, 1137.8260498f}};
			case 277:
				return {{1000.0000000f, 1137.8822021f}};
			case 278:
				return {{1000.0000000f, 1137.9409180f}};
			case 279:
				return {{1000.0000000f, 1138.8922119f}};
			case 280:
				return {{1000.0000000f, 1139.7972412f}};
			case 281:
				return {{1000.0000000f, 1140.7630615f}};
			case 282:
				return {{1000.0000000f, 1140.8229980f}};
			case 283:
				return {{1000.0000000f, 1140.8833008f}};
			case 284:
				return {{1000.0000000f, 1141.7875977f}};
			case 285:
				return {{1000.0000000f, 1141.8500977f}};
			case 286:
				return {{1000.0000000f, 1141.9088135f}};
			case 287:
				return {{1000.0000000f, 1142.8905029f}};
			case 288:
				return {{1000.0000000f, 1142.9537354f}};
			case 289:
				return {{1000.0000000f, 1143.0137939f}};
			case 290:
				return {{1000.0000000f, 1144.0216064f}};
			case 291:
				return {{1000.0000000f, 1144.0867920f}};
			case 292:
				return {{1000.0000000f, 1144.1496582f}};
			case 293:
				return {{1000.0000000f, 1145.1230469f}};
			case 294:
				return {{1000.0000000f, 1145.1889648f}};
			case 295:
				return {{1000.0000000f, 1146.2509766f}};
			case 296:
				return {{1000.0000000f, 1146.3197021f}};
			case 297:
				return {{1000.0000000f, 1146.3859863f}};
			case 298:
				return {{1000.0000000f, 1147.4746094f}};
			case 299:
				return {{1000.0000000f, 1147.5428467f}};
			case 300:
				return {{1000.0000000f, 1147.6119385f}};
			case 301:
				return {{1000.0000000f, 1148.6572266f}};
			case 302:
				return {{1000.0000000f, 1148.7269287f}};
			case 303:
				return {{1000.0000000f, 1149.8464355f}};
			case 304:
				return {{1000.0000000f, 1149.9144287f}};
			case 305:
				return {{1000.0000000f, 1149.9838867f}};
			case 306:
				return {{1000.0000000f, 1150.9987793f}};
			case 307:
				return {{1000.0000000f, 1151.0650635f}};
			case 308:
				return {{1000.0000000f, 1152.0865479f}};
			case 309:
				return {{1000.0000000f, 1153.0361328f}};
			case 310:
				return {{1000.0000000f, 1153.0930176f}};
			case 311:
				return {{1000.0000000f, 1153.9031982f}};
			case 312:
				return {{1000.0000000f, 1153.9541016f}};
			case 313:
				return {{1000.0000000f, 1154.0064697f}};
			case 314:
				return {{1000.0000000f, 1154.7772217f}};
			case 315:
				return {{1000.0000000f, 1154.8239746f}};
			case 316:
				return {{1000.0000000f, 1154.8682861f}};
			case 317:
				return {{1000.0000000f, 1155.5454102f}};
			case 318:
				return {{1000.0000000f, 1155.5845947f}};
			case 319:
				return {{1000.0000000f, 1156.1385498f}};
			case 320:
				return {{1000.0000000f, 1156.1723633f}};
			case 321:
				return {{1000.0000000f, 1156.2067871f}};
			case 322:
				return {{1000.0000000f, 1156.7149658f}};
			case 323:
				return {{1000.0000000f, 1156.7445068f}};
			case 324:
				return {{1000.0000000f, 1156.7736816f}};
			case 325:
				return {{1000.0000000f, 1157.1789551f}};
			case 326:
				return {{1000.0000000f, 1157.2036133f}};
			case 327:
				return {{1000.0000000f, 1157.2282715f}};
			case 328:
				return {{1000.0000000f, 1157.5881348f}};
			case 329:
				return {{1000.0000000f, 1157.6094971f}};
			case 330:
				return {{1000.0000000f, 1157.6300049f}};
			case 331:
				return {{1000.0000000f, 1157.9293213f}};
			case 332:
				return {{1000.0000000f, 1157.9476318f}};
			case 333:
				return {{1000.0000000f, 1157.9636230f}};
			case 334:
				return {{1000.0000000f, 1158.1926270f}};
			case 335:
				return {{1000.0000000f, 1158.2077637f}};
			case 336:
				return {{1000.0000000f, 1158.2216797f}};
			case 337:
				return {{1000.0000000f, 1158.4223633f}};
			case 338:
				return {{1000.0000000f, 1158.4313965f}};
			case 339:
				return {{1000.0000000f, 1158.4434814f}};
			case 340:
				return {{1000.0000000f, 1158.6016846f}};
			case 341:
				return {{1000.0000000f, 1158.6123047f}};
			case 342:
				return {{1000.0000000f, 1158.7312012f}};
			case 343:
				return {{1000.0000000f, 1158.8332520f}};
			case 344:
				return {{1000.0000000f, 1158.9101562f}};
			case 345:
				return {{1000.0000000f, 1158.9150391f}};
			case 346:
				return {{1000.0000000f, 1158.9194336f}};
			case 347:
				return {{1000.0000000f, 1158.9812012f}};
			case 348:
				return {{1000.0000000f, 1158.9848633f}};
			case 349:
				return {{1000.0000000f, 1159.0311279f}};
			case 350:
				return {{1000.0000000f, 1159.0336914f}};
			case 351:
				return {{1000.0000000f, 1159.0361328f}};
			case 352:
				return {{1000.0000000f, 1159.0672607f}};
			case 353:
				return {{1000.0000000f, 1159.0692139f}};
			case 354:
				return {{1000.0000000f, 1159.0927734f}};
			case 355:
				return {{1000.0000000f, 1159.0919189f}};
			case 356:
				return {{1000.0000000f, 1159.0931396f}};
			case 357:
				return {{1000.0000000f, 1159.1093750f}};
			case 358:
				return {{1000.0000000f, 1159.1091309f}};
			case 359:
				return {{1000.0000000f, 1159.1188965f}};
			case 360:
				return {{1000.0000000f, 1159.1184082f}};
			case 361:
				return {{1000.0000000f, 1159.1197510f}};
			case 362:
				return {{1000.0000000f, 1159.1243896f}};
			case 363:
				return {{1000.0000000f, 1159.1235352f}};
			case 364:
				return {{1000.0000000f, 1159.1246338f}};
			case 365:
				return {{1000.0000000f, 1159.1237793f}};
			case 366:
				return {{1000.0000000f, 1159.1247559f}};
			case 367:
				return {{1000.0000000f, 1159.1247559f}};
			case 368:
				return {{1000.0000000f, 1159.1235352f}};
			case 369:
				return {{1000.0000000f, 1159.1245117f}};
			case 370:
				return {{1000.0000000f, 1159.1204834f}};
			case 371:
				return {{1000.0000000f, 1159.1202393f}};
			case 372:
				return {{1000.0000000f, 1159.1209717f}};
			case 373:
				return {{1000.0000000f, 1159.1163330f}};
			case 374:
				return {{1000.0000000f, 1159.1118164f}};
			case 375:
				return {{1000.0000000f, 1159.1069336f}};
			case 376:
				return {{1000.0000000f, 1159.1065674f}};
			case 377:
				return {{1000.0000000f, 1159.1062012f}};
			case 378:
				return {{1000.0000000f, 1159.1025391f}};
			case 379:
				return {{1000.0000000f, 1159.1013184f}};
			case 380:
				return {{1000.0000000f, 1159.1010742f}};
			case 381:
				return {{1000.0000000f, 1159.0972900f}};
			case 382:
				return {{1000.0000000f, 1159.0970459f}};
			case 383:
				return {{1000.0000000f, 1159.0957031f}};
			case 384:
				return {{1000.0000000f, 1159.0916748f}};
			case 385:
				return {{1000.0000000f, 1159.0924072f}};
			case 386:
				return {{1000.0000000f, 1159.0921631f}};
			case 387:
				return {{1000.0000000f, 1159.0870361f}};
			case 388:
				return {{1000.0000000f, 1159.0867920f}};
			case 389:
				return {{1000.0000000f, 1159.0830078f}};
			case 390:
				return {{1000.0000000f, 1159.0849609f}};
			case 391:
				return {{1000.0000000f, 1159.0847168f}};
			case 392:
				return {{1000.0000000f, 1159.0815430f}};
			case 393:
				return {{1000.0000000f, 1159.0812988f}};
			case 394:
				return {{1000.0000000f, 1159.0810547f}};
			case 395:
				return {{1000.0000000f, 1159.0781250f}};
			case 396:
				return {{1000.0000000f, 1159.0781250f}};
			case 397:
				return {{1000.0000000f, 1159.0778809f}};
			case 398:
				return {{1000.0000000f, 1159.0753174f}};
			case 399:
				return {{1000.0000000f, 1159.0751953f}};
			case 400:
				return {{1000.0000000f, 1159.0732422f}};
			case 401:
				return {{1000.0000000f, 1159.0729980f}};
			case 402:
				return {{1000.0000000f, 1159.0728760f}};
			case 403:
				return {{1000.0000000f, 1159.0710449f}};
			case 404:
				return {{1000.0000000f, 1159.0709229f}};
			case 405:
				return {{1000.0000000f, 1159.0708008f}};
			case 406:
				return {{1000.0000000f, 1159.0693359f}};
			case 407:
				return {{1000.0000000f, 1159.0679932f}};
			case 408:
				return {{1000.0000000f, 1159.0668945f}};
			case 409:
				return {{1000.0000000f, 1159.0667725f}};
			case 410:
				return {{1000.0000000f, 1159.0666504f}};
			case 411:
				return {{1000.0000000f, 1159.0659180f}};
			case 412:
				return {{1000.0000000f, 1159.0659180f}};
			case 413:
				return {{1000.0000000f, 1159.0659180f}};
			case 414:
				return {{1000.0000000f, 1159.0650635f}};
			case 415:
				return {{1000.0000000f, 1159.0650635f}};
			case 416:
				return {{1000.0000000f, 1159.0650635f}};
			case 417:
				return {{1000.0000000f, 1159.0644531f}};
			case 418:
				return {{1000.0000000f, 1159.0644531f}};
			case 419:
				return {{1000.0000000f, 1159.0644531f}};
			case 420:
				return {{1000.0000000f, 1159.0639648f}};
			case 421:
				return {{1000.0000000f, 1159.0639648f}};
			case 422:
				return {{1000.0000000f, 1159.0635986f}};
			case 423:
				return {{1000.0000000f, 1159.0635986f}};
			case 424:
				return {{1000.0000000f, 1159.0635986f}};
			case 425:
				return {{1000.0000000f, 1159.0634766f}};
			case 426:
				return {{1000.0000000f, 1159.0634766f}};
			case 427:
				return {{1000.0000000f, 1159.0634766f}};
			case 428:
				return {{1000.0000000f, 1159.0631104f}};
			case 429:
				return {{1000.0000000f, 1159.0631104f}};
			case 430:
				return {{1000.0000000f, 1159.0631104f}};
			case 431:
				return {{1000.0000000f, 1159.0629883f}};
			case 432:
				return {{1000.0000000f, 1159.0629883f}};
			case 433:
				return {{1000.0000000f, 1159.0629883f}};
			case 434:
				return {{1000.0000000f, 1159.0628662f}};
			case 435:
				return {{1000.0000000f, 1159.0628662f}};
			case 436:
				return {{1000.0000000f, 1159.0628662f}};
			case 437:
				return {{1000.0000000f, 1159.0627441f}};
			case 438:
				return {{1000.0000000f, 1159.0627441f}};
			case 439:
				return {{1000.0000000f, 1159.0627441f}};
			case 440:
				return {{1000.0000000f, 1159.0627441f}};
			case 441:
				return {{1000.0000000f, 1159.0627441f}};
			case 442:
				return {{1000.0000000f, 1159.0627441f}};
			case 443:
				return {{1000.0000000f, 1159.0627441f}};
			case 444:
				return {{1000.0000000f, 1159.0627441f}};
			case 445:
				return {{1000.0000000f, 1159.0627441f}};
			case 446:
				return {{1000.0000000f, 1159.0627441f}};
			case 447:
				return {{1000.0000000f, 1159.0627441f}};
			case 448:
				return {{1000.0000000f, 1159.0627441f}};
			case 449:
				return {{1000.0000000f, 1159.0627441f}};
			case 450:
				return {{1000.0000000f, 1159.0627441f}};
			case 451:
				return {{1000.0000000f, 1159.0627441f}};
			case 452:
				return {{1000.0000000f, 1159.0627441f}};
			case 453:
				return {{1000.0000000f, 1159.0627441f}};
			case 454:
				return {{1000.0000000f, 1159.0627441f}};
			case 455:
				return {{1000.0000000f, 1159.0627441f}};
			case 456:
				return {{1000.0000000f, 1159.0627441f}};
			case 457:
				return {{1000.0000000f, 1159.0627441f}};
			case 458:
				return {{1000.0000000f, 1159.0627441f}};
			case 459:
				return {{1000.0000000f, 1159.0627441f}};
			case 460:
				return {{1000.0000000f, 1159.0627441f}};
			case 461:
				return {{1000.0000000f, 1159.0627441f}};
			case 462:
				return {{1000.0000000f, 1159.0627441f}};
			case 463:
				return {{1000.0000000f, 1159.0627441f}};
			case 464:
				return {{1000.0000000f, 1159.0627441f}};
			case 465:
				return {{1000.0000000f, 1159.0627441f}};
			case 466:
				return {{1000.0000000f, 1159.0627441f}};
			case 467:
				return {{1000.0000000f, 1159.0627441f}};
			case 468:
				return {{1000.0000000f, 1159.0627441f}};
			case 469:
				return {{1000.0000000f, 1159.0627441f}};
			case 470:
				return {{1000.0000000f, 1159.0627441f}};
			case 471:
				return {{1000.0000000f, 1159.0627441f}};
			case 472:
				return {{1000.0000000f, 1159.0627441f}};
			case 473:
				return {{1000.0000000f, 1159.0627441f}};
			case 474:
				return {{1000.0000000f, 1159.0627441f}};
			case 475:
				return {{1000.0000000f, 1159.0627441f}};
			case 476:
				return {{1000.0000000f, 1159.0627441f}};
			case 477:
				return {{1000.0000000f, 1159.0627441f}};
			case 478:
				return {{1000.0000000f, 1159.0627441f}};
			case 479:
				return {{1000.0000000f, 1159.0627441f}};
			case 480:
				return {{1000.0000000f, 1159.0627441f}};
			case 481:
				return {{1000.0000000f, 1159.0627441f}};
			case 482:
				return {{1000.0000000f, 1159.0627441f}};
			case 483:
				return {{1000.0000000f, 1159.0627441f}};
			case 484:
				return {{1000.0000000f, 1159.0627441f}};
			case 485:
				return {{1000.0000000f, 1159.0627441f}};
			case 486:
				return {{1000.0000000f, 1159.0627441f}};
			case 487:
				return {{1000.0000000f, 1159.0627441f}};
			case 488:
				return {{1000.0000000f, 1159.0627441f}};
			case 489:
				return {{1000.0000000f, 1159.0627441f}};
			case 490:
				return {{1000.0000000f, 1159.0627441f}};
			case 491:
				return {{1000.0000000f, 1159.0627441f}};
			case 492:
				return {{1000.0000000f, 1159.0627441f}};
			case 493:
				return {{1000.0000000f, 1159.0627441f}};
			case 494:
				return {{1000.0000000f, 1159.0627441f}};
			case 495:
				return {{1000.0000000f, 1159.0627441f}};
			case 496:
				return {{1000.0000000f, 1159.0627441f}};
			case 497:
				return {{1000.0000000f, 1159.0627441f}};
			case 498:
				return {{1000.0000000f, 1159.0627441f}};
			case 499:
				return {{1000.0000000f, 1159.0627441f}};
			case 500:
				return {{1000.0000000f, 1159.0627441f}};
			case 501:
				return {{1000.0000000f, 1159.0627441f}};
			case 502:
				return {{1000.0000000f, 1159.0627441f}};
			case 503:
				return {{1000.0000000f, 1159.0556641f}};
			case 504:
				return {{1000.0000000f, 1158.9788818f}};
			case 505:
				return {{1000.0000000f, 1158.9747314f}};
			case 506:
				return {{1000.0000000f, 1158.9708252f}};
			case 507:
				return {{1000.0000000f, 1158.9665527f}};
			case 508:
				return {{1000.0000000f, 1158.8607178f}};
			case 509:
				return {{1000.0000000f, 1158.8513184f}};
			case 510:
				return {{1000.0000000f, 1158.6680908f}};
			case 511:
				return {{1000.0000000f, 1158.6555176f}};
			case 512:
				return {{1000.0000000f, 1158.4096680f}};
			case 513:
				return {{1000.0000000f, 1158.3889160f}};
			case 514:
				return {{1000.0000000f, 1158.3708496f}};
			case 515:
				return {{1000.0000000f, 1158.3513184f}};
			case 516:
				return {{1000.0000000f, 1157.9824219f}};
			case 517:
				return {{1000.0000000f, 1157.9565430f}};
			case 518:
				return {{1000.0000000f, 1157.4881592f}};
			case 519:
				return {{1000.0000000f, 1157.4548340f}};
			case 520:
				return {{1000.0000000f, 1157.4221191f}};
			case 521:
				return {{1000.0000000f, 1156.8894043f}};
			case 522:
				return {{1000.0000000f, 1156.8496094f}};
			case 523:
				return {{1000.0000000f, 1156.8107910f}};
			case 524:
				return {{1000.0000000f, 1156.1440430f}};
			case 525:
				return {{1000.0000000f, 1156.0985107f}};
			case 526:
				return {{1000.0000000f, 1156.0523682f}};
			case 527:
				return {{1000.0000000f, 1155.3383789f}};
			case 528:
				return {{1000.0000000f, 1155.2858887f}};
			case 529:
				return {{1000.0000000f, 1155.2373047f}};
			case 530:
				return {{1000.0000000f, 1154.3837891f}};
			case 531:
				return {{1000.0000000f, 1154.3273926f}};
			case 532:
				return {{1000.0000000f, 1154.2712402f}};
			case 533:
				return {{1000.0000000f, 1153.3339844f}};
			case 534:
				return {{1000.0000000f, 1153.2744141f}};
			case 535:
				return {{1000.0000000f, 1153.2131348f}};
			case 536:
				return {{1000.0000000f, 1151.2115479f}};
			case 537:
				return {{1000.0000000f, 1151.1442871f}};
			case 538:
				return {{1000.0000000f, 1150.0732422f}};
			case 539:
				return {{1000.0000000f, 1150.0034180f}};
			case 540:
				return {{1000.0000000f, 1149.9394531f}};
			case 541:
				return {{1000.0000000f, 1148.9256592f}};
			case 542:
				return {{1000.0000000f, 1148.8596191f}};
			case 543:
				return {{1000.0000000f, 1148.7905273f}};
			case 544:
				return {{1000.0000000f, 1147.6929932f}};
			case 545:
				return {{1000.0000000f, 1147.6243896f}};
			case 546:
				return {{1000.0000000f, 1147.5548096f}};
			case 547:
				return {{1000.0000000f, 1146.5052490f}};
			case 548:
				return {{1000.0000000f, 1146.4353027f}};
			case 549:
				return {{1000.0000000f, 1146.3643799f}};
			case 550:
				return {{1000.0000000f, 1145.2187500f}};
			case 551:
				return {{1000.0000000f, 1145.1457520f}};
			case 552:
				return {{1000.0000000f, 1143.9833984f}};
			case 553:
				return {{1000.0000000f, 1143.9104004f}};
			case 554:
				return {{1000.0000000f, 1142.8126221f}};
			case 555:
				return {{1000.0000000f, 1142.7387695f}};
			case 556:
				return {{1000.0000000f, 1142.6663818f}};
			case 557:
				return {{1000.0000000f, 1141.4885254f}};
			case 558:
				return {{1000.0000000f, 1141.4157715f}};
			case 559:
				return {{1000.0000000f, 1141.3427734f}};
			case 560:
				return {{1000.0000000f, 1140.1538086f}};
			case 561:
				return {{1000.0000000f, 1140.0792236f}};
			case 562:
				return {{1000.0000000f, 1140.0041504f}};
			case 563:
				return {{1000.0000000f, 1138.8764648f}};
			case 564:
				return {{1000.0000000f, 1138.8011475f}};
			case 565:
				return {{1000.0000000f, 1138.7241211f}};
			case 566:
				return {{1000.0000000f, 1137.5059814f}};
			case 567:
				return {{1000.0000000f, 1137.4290771f}};
			case 568:
				return {{1000.0000000f, 1136.2747803f}};
			case 569:
				return {{1000.0000000f, 1135.0505371f}};
			case 570:
				return {{1000.0000000f, 1133.8515625f}};
			case 571:
				return {{1000.0000000f, 1133.7790527f}};
			case 572:
				return {{1000.0000000f, 1133.7050781f}};
			case 573:
				return {{1000.0000000f, 1132.6080322f}};
			case 574:
				return {{1000.0000000f, 1132.5357666f}};
			case 575:
				return {{1000.0000000f, 1132.4631348f}};
			case 576:
				return {{1000.0000000f, 1131.3038330f}};
			case 577:
				return {{1000.0000000f, 1131.2316895f}};
			case 578:
				return {{1000.0000000f, 1131.1604004f}};
			case 579:
				return {{1000.0000000f, 1130.0019531f}};
			case 580:
				return {{1000.0000000f, 1129.9295654f}};
			case 581:
				return {{1000.0000000f, 1129.8558350f}};
			case 582:
				return {{1000.0000000f, 1128.7622070f}};
			case 583:
				return {{1000.0000000f, 1128.6893311f}};
			case 584:
				return {{1000.0000000f, 1127.5163574f}};
			case 585:
				return {{1000.0000000f, 1127.4442139f}};
			case 586:
				return {{1000.0000000f, 1127.3704834f}};
			case 587:
				return {{1000.0000000f, 1126.2668457f}};
			case 588:
				return {{1000.0000000f, 1126.1925049f}};
			case 589:
				return {{1000.0000000f, 1126.1182861f}};
			case 590:
				return {{1000.0000000f, 1124.9278564f}};
			case 591:
				return {{1000.0000000f, 1124.8529053f}};
			case 592:
				return {{1000.0000000f, 1124.7800293f}};
			case 593:
				return {{1000.0000000f, 1123.5731201f}};
			case 594:
				return {{1000.0000000f, 1123.4982910f}};
			case 595:
				return {{1000.0000000f, 1123.4221191f}};
			case 596:
				return {{1000.0000000f, 1122.2763672f}};
			case 597:
				return {{1000.0000000f, 1122.1998291f}};
			case 598:
				return {{1000.0000000f, 1122.1239014f}};
			case 599:
				return {{1000.0000000f, 1120.8834229f}};
			case 600:
				return {{1000.0000000f, 1120.8054199f}};
			case 601:
				return {{1000.0000000f, 1118.5238037f}};
			case 602:
				return {{1000.0000000f, 1118.4548340f}};
			case 603:
				return {{1000.0000000f, 1117.3618164f}};
			case 604:
				return {{1000.0000000f, 1117.2938232f}};
			case 605:
				return {{1000.0000000f, 1116.3078613f}};
			case 606:
				return {{1000.0000000f, 1116.2424316f}};
			case 607:
				return {{1000.0000000f, 1116.1796875f}};
			case 608:
				return {{1000.0000000f, 1115.1962891f}};
			case 609:
				return {{1000.0000000f, 1115.1368408f}};
			case 610:
				return {{1000.0000000f, 1114.2341309f}};
			case 611:
				return {{1000.0000000f, 1114.1816406f}};
			case 612:
				return {{1000.0000000f, 1113.4143066f}};
			case 613:
				return {{1000.0000000f, 1113.3665771f}};
			case 614:
				return {{1000.0000000f, 1112.6353760f}};
			case 615:
				return {{1000.0000000f, 1112.5933838f}};
			case 616:
				return {{1000.0000000f, 1112.5507812f}};
			case 617:
				return {{1000.0000000f, 1111.9138184f}};
			case 618:
				return {{1000.0000000f, 1111.8769531f}};
			case 619:
				return {{1000.0000000f, 1111.8399658f}};
			case 620:
				return {{1000.0000000f, 1111.3237305f}};
			case 621:
				return {{1000.0000000f, 1111.2934570f}};
			case 622:
				return {{1000.0000000f, 1110.8183594f}};
			case 623:
				return {{1000.0000000f, 1110.7900391f}};
			case 624:
				return {{1000.0000000f, 1110.7618408f}};
			case 625:
				return {{1000.0000000f, 1110.3862305f}};
			case 626:
				return {{1000.0000000f, 1110.3624268f}};
			case 627:
				return {{1000.0000000f, 1110.3404541f}};
			case 628:
				return {{1000.0000000f, 1109.7215576f}};
			case 629:
				return {{1000.0000000f, 1109.7049561f}};
			case 630:
				return {{1000.0000000f, 1109.4852295f}};
			case 631:
				return {{1000.0000000f, 1109.4719238f}};
			case 632:
				return {{1000.0000000f, 1109.4587402f}};
			case 633:
				return {{1000.0000000f, 1109.2697754f}};
			case 634:
				return {{1000.0000000f, 1109.2592773f}};
			case 635:
				return {{1000.0000000f, 1109.1052246f}};
			case 636:
				return {{1000.0000000f, 1109.0957031f}};
			case 637:
				return {{1000.0000000f, 1109.0872803f}};
			case 638:
				return {{1000.0000000f, 1108.9757080f}};
			case 639:
				return {{1000.0000000f, 1108.9674072f}};
			case 640:
				return {{1000.0000000f, 1108.9606934f}};
			case 641:
				return {{1000.0000000f, 1108.8662109f}};
			case 642:
				return {{1000.0000000f, 1108.8615723f}};
			case 643:
				return {{1000.0000000f, 1108.7926025f}};
			case 644:
				return {{1000.0000000f, 1108.7883301f}};
			case 645:
				return {{1000.0000000f, 1108.7843018f}};
			case 646:
				return {{1000.0000000f, 1108.7290039f}};
			case 647:
				return {{1000.0000000f, 1108.7255859f}};
			case 648:
				return {{1000.0000000f, 1108.7227783f}};
			case 649:
				return {{1000.0000000f, 1108.6815186f}};
			case 650:
				return {{1000.0000000f, 1108.6812744f}};
			case 651:
				return {{1000.0000000f, 1108.6540527f}};
			case 652:
				return {{1000.0000000f, 1108.6522217f}};
			case 653:
				return {{1000.0000000f, 1108.6508789f}};
			case 654:
				return {{1000.0000000f, 1108.6318359f}};
			case 655:
				return {{1000.0000000f, 1108.6298828f}};
			case 656:
				return {{1000.0000000f, 1108.6170654f}};
			case 657:
				return {{1000.0000000f, 1108.6163330f}};
			case 658:
				return {{1000.0000000f, 1108.6103516f}};
			case 659:
				return {{1000.0000000f, 1108.6055908f}};
			case 660:
				return {{1000.0000000f, 1108.6057129f}};
			case 661:
				return {{1000.0000000f, 1108.6057129f}};
			case 662:
				return {{1000.0000000f, 1108.6057129f}};
			case 663:
				return {{1000.0000000f, 1108.6060791f}};
			case 664:
				return {{1000.0000000f, 1108.6072998f}};
			case 665:
				return {{1000.0000000f, 1108.6103516f}};
			case 666:
				return {{1000.0000000f, 1108.6105957f}};
			case 667:
				return {{1000.0000000f, 1108.6097412f}};
			case 668:
				return {{1000.0000000f, 1108.6146240f}};
			case 669:
				return {{1000.0000000f, 1108.6147461f}};
			case 670:
				return {{1000.0000000f, 1108.6140137f}};
			case 671:
				return {{1000.0000000f, 1108.6196289f}};
			case 672:
				return {{1000.0000000f, 1108.6188965f}};
			case 673:
				return {{1000.0000000f, 1108.6237793f}};
			case 674:
				return {{1000.0000000f, 1108.6240234f}};
			case 675:
				return {{1000.0000000f, 1108.6242676f}};
			case 676:
				return {{1000.0000000f, 1108.6297607f}};
			case 677:
				return {{1000.0000000f, 1108.6300049f}};
			case 678:
				return {{1000.0000000f, 1108.6292725f}};
			case 679:
				return {{1000.0000000f, 1108.6348877f}};
			case 680:
				return {{1000.0000000f, 1108.6341553f}};
			case 681:
				return {{1000.0000000f, 1108.6381836f}};
			case 682:
				return {{1000.0000000f, 1108.6395264f}};
			case 683:
				return {{1000.0000000f, 1108.6397705f}};
			case 684:
				return {{1000.0000000f, 1108.6437988f}};
			case 685:
				return {{1000.0000000f, 1108.6440430f}};
			case 686:
				return {{1000.0000000f, 1108.6475830f}};
			case 687:
				return {{1000.0000000f, 1108.6469727f}};
			case 688:
				return {{1000.0000000f, 1108.6500244f}};
			case 689:
				return {{1000.0000000f, 1108.6528320f}};
			case 690:
				return {{1000.0000000f, 1108.6555176f}};
			case 691:
				return {{1000.0000000f, 1108.6564941f}};
			case 692:
				return {{1000.0000000f, 1108.6557617f}};
			case 693:
				return {{1000.0000000f, 1108.6577148f}};
			case 694:
				return {{1000.0000000f, 1108.6579590f}};
			case 695:
				return {{1000.0000000f, 1108.6597900f}};
			case 696:
				return {{1000.0000000f, 1108.6607666f}};
			case 697:
				return {{1000.0000000f, 1108.6599121f}};
			case 698:
				return {{1000.0000000f, 1108.6613770f}};
			case 699:
				return {{1000.0000000f, 1108.6616211f}};
			case 700:
				return {{1000.0000000f, 1108.6638184f}};
			case 701:
				return {{1000.0000000f, 1108.6630859f}};
			case 702:
				return {{1000.0000000f, 1108.6650391f}};
			case 703:
				return {{1000.0000000f, 1108.6641846f}};
			case 704:
				return {{1000.0000000f, 1108.6652832f}};
			case 705:
				return {{1000.0000000f, 1108.6651611f}};
			case 706:
				return {{1000.0000000f, 1108.6651611f}};
			case 707:
				return {{1000.0000000f, 1108.6651611f}};
			case 708:
				return {{1000.0000000f, 1108.6668701f}};
			case 709:
				return {{1000.0000000f, 1108.6668701f}};
			case 710:
				return {{1000.0000000f, 1108.6674805f}};
			case 711:
				return {{1000.0000000f, 1108.6674805f}};
			case 712:
				return {{1000.0000000f, 1108.6674805f}};
			case 713:
				return {{1000.0000000f, 1108.6668701f}};
			case 714:
				return {{1000.0000000f, 1108.6668701f}};
			case 715:
				return {{1000.0000000f, 1108.6668701f}};
			case 716:
				return {{1000.0000000f, 1108.6672363f}};
			case 717:
				return {{1000.0000000f, 1108.6672363f}};
			case 718:
				return {{1000.0000000f, 1108.6674805f}};
			case 719:
				return {{1000.0000000f, 1108.6687012f}};
			case 720:
				return {{1000.0000000f, 1108.6678467f}};
			case 721:
				return {{1000.0000000f, 1108.6678467f}};
			case 722:
				return {{1000.0000000f, 1108.6678467f}};
			case 723:
				return {{1000.0000000f, 1108.6689453f}};
			case 724:
				return {{1000.0000000f, 1108.6689453f}};
			case 725:
				return {{1000.0000000f, 1108.6680908f}};
			case 726:
				return {{1000.0000000f, 1108.6680908f}};
			case 727:
				return {{1000.0000000f, 1108.6680908f}};
			case 728:
				return {{1000.0000000f, 1108.6680908f}};
			case 729:
				return {{1000.0000000f, 1108.6680908f}};
			case 730:
				return {{1000.0000000f, 1108.6680908f}};
			case 731:
				return {{1000.0000000f, 1108.6680908f}};
			case 732:
				return {{1000.0000000f, 1108.6680908f}};
			case 733:
				return {{1000.0000000f, 1108.6680908f}};
			case 734:
				return {{1000.0000000f, 1108.6680908f}};
			case 735:
				return {{1000.0000000f, 1108.6680908f}};
			case 736:
				return {{1000.0000000f, 1108.6680908f}};
			case 737:
				return {{1000.0000000f, 1108.6680908f}};
			case 738:
				return {{1000.0000000f, 1108.6680908f}};
			case 739:
				return {{1000.0000000f, 1108.6680908f}};
			case 740:
				return {{1000.0000000f, 1108.6680908f}};
			case 741:
				return {{1000.0000000f, 1108.6680908f}};
			case 742:
				return {{1000.0000000f, 1108.6680908f}};
			case 743:
				return {{1000.0000000f, 1108.6680908f}};
			case 744:
				return {{1000.0000000f, 1108.6680908f}};
			case 745:
				return {{1000.0000000f, 1108.6680908f}};
			case 746:
				return {{1000.0000000f, 1108.6680908f}};
			case 747:
				return {{1000.0000000f, 1108.6680908f}};
			case 748:
				return {{1000.0000000f, 1108.6680908f}};
			case 749:
				return {{1000.0000000f, 1108.6680908f}};
			case 750:
				return {{1000.0000000f, 1108.6680908f}};
			case 751:
				return {{1000.0000000f, 1108.6680908f}};
			case 752:
				return {{1000.0000000f, 1108.6680908f}};
			case 753:
				return {{1000.0000000f, 1108.6680908f}};
			case 754:
				return {{1000.0000000f, 1108.6680908f}};
			case 755:
				return {{1000.0000000f, 1108.6680908f}};
			case 756:
				return {{1000.0000000f, 1108.6680908f}};
			case 757:
				return {{1000.0000000f, 1108.6680908f}};
			case 758:
				return {{1000.0000000f, 1108.6680908f}};
			case 759:
				return {{1000.0000000f, 1108.6680908f}};
			case 760:
				return {{1000.0000000f, 1108.6680908f}};
			case 761:
				return {{1000.0000000f, 1108.6680908f}};
			case 762:
				return {{1000.0000000f, 1108.6680908f}};
			case 763:
				return {{1000.0000000f, 1108.6680908f}};
			case 764:
				return {{1000.0000000f, 1108.6680908f}};
			case 765:
				return {{1000.0000000f, 1108.6680908f}};
			case 766:
				return {{1000.0000000f, 1108.6680908f}};
			case 767:
				return {{1000.0000000f, 1108.6680908f}};
			case 768:
				return {{1000.0000000f, 1108.6680908f}};
			case 769:
				return {{1000.0000000f, 1108.6680908f}};
			case 770:
				return {{1000.0000000f, 1108.6680908f}};
			case 771:
				return {{1000.0000000f, 1108.6680908f}};
			case 772:
				return {{1000.0000000f, 1108.6680908f}};
			case 773:
				return {{1000.0000000f, 1108.6680908f}};
			case 774:
				return {{1000.0000000f, 1108.6680908f}};
			case 775:
				return {{1000.0000000f, 1108.6680908f}};
			case 776:
				return {{1000.0000000f, 1108.6680908f}};
			case 777:
				return {{1000.0000000f, 1108.6680908f}};
			case 778:
				return {{1000.0000000f, 1108.6680908f}};
			case 779:
				return {{1000.0000000f, 1108.6680908f}};
			case 780:
				return {{1000.0000000f, 1108.6680908f}};
			case 781:
				return {{1000.0000000f, 1108.6680908f}};
			case 782:
				return {{1000.0000000f, 1108.6680908f}};
			case 783:
				return {{1000.0000000f, 1108.6680908f}};
			case 784:
				return {{1000.0000000f, 1108.6680908f}};
			case 785:
				return {{1000.0000000f, 1108.6680908f}};
			case 786:
				return {{1000.0000000f, 1108.6680908f}};
			case 787:
				return {{1000.0000000f, 1108.6680908f}};
			case 788:
				return {{1000.0000000f, 1108.6680908f}};
			case 789:
				return {{1000.0000000f, 1108.6680908f}};
			case 790:
				return {{1000.0000000f, 1108.6680908f}};
			case 791:
				return {{1000.0000000f, 1108.6680908f}};
			case 792:
				return {{1000.0000000f, 1108.6680908f}};
			case 793:
				return {{1000.0000000f, 1108.6680908f}};
			case 794:
				return {{1000.0000000f, 1108.6680908f}};
			case 795:
				return {{1000.0000000f, 1108.6680908f}};
			case 796:
				return {{1000.0000000f, 1108.6680908f}};
			case 797:
				return {{1000.0000000f, 1108.6680908f}};
			case 798:
				return {{1000.0000000f, 1108.6680908f}};
			case 799:
				return {{1000.0000000f, 1108.6680908f}};
			case 800:
				return {{1000.0000000f, 1108.6680908f}};
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
			case 301:
				return {{1000.0000000f, 1150.9470215f}};
			case 502:
				return {{1000.0000000f, 1161.3671875f}};
			case 600:
				return {{1000.0000000f, 1123.0528564f}};
			case 601:
				return {{1000.0000000f, 1120.7691650f}};
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
			case 301:
				return {{1000.0000000f, 1152.5194092f}};
			case 502:
				return {{1000.0000000f, 1162.9497070f}};
			case 600:
				return {{1000.0000000f, 1124.5964355f}};
			case 601:
				return {{1000.0000000f, 1122.3103027f}};
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
			case 301:
				return {{1000.0000000f, 1153.6135254f}};
			case 502:
				return {{1000.0000000f, 1164.0526123f}};
			case 600:
				return {{1000.0000000f, 1125.6705322f}};
			case 601:
				return {{1000.0000000f, 1123.3833008f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		assert(false); // Shouldn't be any unaccounted raycasts
		return std::nullopt;
	}
};
// NOLINTEND(bugprone-branch-clone, google-readability-function-size)
