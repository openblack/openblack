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

class MoveBackwardForwardMockAction final: public MockAction
{
	static constexpr uint32_t k_Start1 = k_StabilizeFrames + 1;
	static constexpr uint32_t k_End1 = k_StabilizeFrames + k_InteractionFrames - 1;
	static constexpr uint32_t k_Start2 = k_StabilizeFrames + k_InteractionFrames + k_StabilizeFrames + 1;
	static constexpr uint32_t k_End2 = k_StabilizeFrames + k_InteractionFrames + k_StabilizeFrames + k_InteractionFrames - 1;

public:
	~MoveBackwardForwardMockAction() final = default;

	[[nodiscard]] bool GetBindable(openblack::input::BindableActionMap action) const final
	{
		using openblack::input::BindableActionMap;
		if (frameNumber >= k_Start1 && frameNumber <= k_End1)
		{
			return (static_cast<uint32_t>(action) & static_cast<uint32_t>(BindableActionMap::MOVE_BACKWARDS)) != 0;
		}

		if (frameNumber >= k_Start2 && frameNumber <= k_End2)
		{
			return (static_cast<uint32_t>(action) & static_cast<uint32_t>(BindableActionMap::MOVE_FORWARDS)) != 0;
		}

		return false;
	}
};

// NOLINTBEGIN(bugprone-branch-clone, google-readability-function-size)
class MoveBackwardForwardMockDynamicsSystem: public MockDynamicsSystem
{
public:
	MoveBackwardForwardMockDynamicsSystem() = default;

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
				return {{1000.0000000f, -368.5462646f}};
			case 204:
				return {{1000.0000000f, -368.5007324f}};
			case 205:
				return {{1000.0000000f, -368.4958496f}};
			case 206:
				return {{1000.0000000f, -368.1545410f}};
			case 207:
				return {{1000.0000000f, -367.8437500f}};
			case 208:
				return {{1000.0000000f, -367.8183594f}};
			case 209:
				return {{1000.0000000f, -367.7923584f}};
			case 210:
				return {{1000.0000000f, -367.2883301f}};
			case 211:
				return {{1000.0000000f, -367.2518311f}};
			case 212:
				return {{1000.0000000f, -366.5830078f}};
			case 213:
				return {{1000.0000000f, -366.5363770f}};
			case 214:
				return {{1000.0000000f, -365.7644043f}};
			case 215:
				return {{1000.0000000f, -365.7086182f}};
			case 216:
				return {{1000.0000000f, -364.7390137f}};
			case 217:
				return {{1000.0000000f, -364.6740723f}};
			case 218:
				return {{1000.0000000f, -363.5664062f}};
			case 219:
				return {{1000.0000000f, -363.4934082f}};
			case 220:
				return {{1000.0000000f, -362.3406982f}};
			case 221:
				return {{1000.0000000f, -362.2607422f}};
			case 222:
				return {{1000.0000000f, -360.9213867f}};
			case 223:
				return {{1000.0000000f, -360.8345947f}};
			case 224:
				return {{1000.0000000f, -359.4886475f}};
			case 225:
				return {{1000.0000000f, -359.3962402f}};
			case 226:
				return {{1000.0000000f, -357.8757324f}};
			case 227:
				return {{1000.0000000f, -357.7783203f}};
			case 228:
				return {{1000.0000000f, -356.1829834f}};
			case 229:
				return {{1000.0000000f, -356.0814209f}};
			case 230:
				return {{1000.0000000f, -352.8350830f}};
			case 231:
				return {{1000.0000000f, -351.1096191f}};
			case 232:
				return {{1000.0000000f, -351.0003662f}};
			case 233:
				return {{1000.0000000f, -350.8908691f}};
			case 234:
				return {{1000.0000000f, -349.2266846f}};
			case 235:
				return {{1000.0000000f, -347.4167480f}};
			case 236:
				return {{1000.0000000f, -347.3027344f}};
			case 237:
				return {{1000.0000000f, -345.5773926f}};
			case 238:
				return {{1000.0000000f, -345.4616699f}};
			case 239:
				return {{1000.0000000f, -343.5964355f}};
			case 240:
				return {{1000.0000000f, -343.4793701f}};
			case 241:
				return {{1000.0000000f, -341.5949707f}};
			case 242:
				return {{1000.0000000f, -341.4768066f}};
			case 243:
				return {{1000.0000000f, -339.6964111f}};
			case 244:
				return {{1000.0000000f, -339.5775146f}};
			case 245:
				return {{1000.0000000f, -337.6667480f}};
			case 246:
				return {{1000.0000000f, -337.5472412f}};
			case 247:
				return {{1000.0000000f, -335.6284180f}};
			case 248:
				return {{1000.0000000f, -335.5084229f}};
			case 249:
				return {{1000.0000000f, -333.7041016f}};
			case 250:
				return {{1000.0000000f, -333.5838623f}};
			case 251:
				return {{1000.0000000f, -331.6549072f}};
			case 252:
				return {{1000.0000000f, -331.5344238f}};
			case 253:
				return {{1000.0000000f, -331.4138184f}};
			case 254:
				return {{1000.0000000f, -327.6844482f}};
			case 255:
				return {{1000.0000000f, -325.7774658f}};
			case 256:
				return {{1000.0000000f, -325.6580811f}};
			case 257:
				return {{1000.0000000f, -323.8619385f}};
			case 258:
				return {{1000.0000000f, -323.7420654f}};
			case 259:
				return {{1000.0000000f, -323.6221924f}};
			case 260:
				return {{1000.0000000f, -321.6982422f}};
			case 261:
				return {{1000.0000000f, -321.5780029f}};
			case 262:
				return {{1000.0000000f, -319.6501465f}};
			case 263:
				return {{1000.0000000f, -319.5297852f}};
			case 264:
				return {{1000.0000000f, -317.7199707f}};
			case 265:
				return {{1000.0000000f, -315.7883301f}};
			case 266:
				return {{1000.0000000f, -315.6677246f}};
			case 267:
				return {{1000.0000000f, -313.8559570f}};
			case 268:
				return {{1000.0000000f, -313.7353516f}};
			case 269:
				return {{1000.0000000f, -311.8021240f}};
			case 270:
				return {{1000.0000000f, -311.6815186f}};
			case 271:
				return {{1000.0000000f, -309.7489014f}};
			case 272:
				return {{1000.0000000f, -309.6282959f}};
			case 273:
				return {{1000.0000000f, -307.8171387f}};
			case 274:
				return {{1000.0000000f, -307.6965332f}};
			case 275:
				return {{1000.0000000f, -307.5759277f}};
			case 276:
				return {{1000.0000000f, -305.6435547f}};
			case 277:
				return {{1000.0000000f, -305.5230713f}};
			case 278:
				return {{1000.0000000f, -305.4024658f}};
			case 279:
				return {{1000.0000000f, -303.4709473f}};
			case 280:
				return {{1000.0000000f, -301.6623535f}};
			case 281:
				return {{1000.0000000f, -299.7344971f}};
			case 282:
				return {{1000.0000000f, -299.6142578f}};
			case 283:
				return {{1000.0000000f, -299.4938965f}};
			case 284:
				return {{1000.0000000f, -297.6867676f}};
			case 285:
				return {{1000.0000000f, -297.5664062f}};
			case 286:
				return {{1000.0000000f, -297.4460449f}};
			case 287:
				return {{1000.0000000f, -295.5183105f}};
			case 288:
				return {{1000.0000000f, -295.3980713f}};
			case 289:
				return {{1000.0000000f, -295.2777100f}};
			case 290:
				return {{1000.0000000f, -293.3509521f}};
			case 291:
				return {{1000.0000000f, -293.2307129f}};
			case 292:
				return {{1000.0000000f, -293.1104736f}};
			case 293:
				return {{1000.0000000f, -291.3048096f}};
			case 294:
				return {{1000.0000000f, -291.1845703f}};
			case 295:
				return {{1000.0000000f, -289.2593994f}};
			case 296:
				return {{1000.0000000f, -289.1392822f}};
			case 297:
				return {{1000.0000000f, -289.0191650f}};
			case 298:
				return {{1000.0000000f, -287.0947266f}};
			case 299:
				return {{1000.0000000f, -286.9747314f}};
			case 300:
				return {{1000.0000000f, -286.8546143f}};
			case 301:
				return {{1000.0000000f, -285.0509033f}};
			case 302:
				return {{1000.0000000f, -284.9309082f}};
			case 303:
				return {{1000.0000000f, -283.0413818f}};
			case 304:
				return {{1000.0000000f, -282.9254150f}};
			case 305:
				return {{1000.0000000f, -282.8098145f}};
			case 306:
				return {{1000.0000000f, -281.1241455f}};
			case 307:
				return {{1000.0000000f, -281.0151367f}};
			case 308:
				return {{1000.0000000f, -279.3377686f}};
			case 309:
				return {{1000.0000000f, -277.7954102f}};
			case 310:
				return {{1000.0000000f, -277.7037354f}};
			case 311:
				return {{1000.0000000f, -276.3975830f}};
			case 312:
				return {{1000.0000000f, -276.3150635f}};
			case 313:
				return {{1000.0000000f, -276.2331543f}};
			case 314:
				return {{1000.0000000f, -275.0008545f}};
			case 315:
				return {{1000.0000000f, -274.9285889f}};
			case 316:
				return {{1000.0000000f, -274.8569336f}};
			case 317:
				return {{1000.0000000f, -273.7854004f}};
			case 318:
				return {{1000.0000000f, -273.7230225f}};
			case 319:
				return {{1000.0000000f, -272.8492432f}};
			case 320:
				return {{1000.0000000f, -272.7950439f}};
			case 321:
				return {{1000.0000000f, -272.7413330f}};
			case 322:
				return {{1000.0000000f, -271.9462891f}};
			case 323:
				return {{1000.0000000f, -271.9005127f}};
			case 324:
				return {{1000.0000000f, -271.8552246f}};
			case 325:
				return {{1000.0000000f, -271.2264404f}};
			case 326:
				return {{1000.0000000f, -271.1878662f}};
			case 327:
				return {{1000.0000000f, -271.1496582f}};
			case 328:
				return {{1000.0000000f, -270.5898438f}};
			case 329:
				return {{1000.0000000f, -270.5579834f}};
			case 330:
				return {{1000.0000000f, -270.5264893f}};
			case 331:
				return {{1000.0000000f, -270.0667725f}};
			case 332:
				return {{1000.0000000f, -270.0407715f}};
			case 333:
				return {{1000.0000000f, -270.0150146f}};
			case 334:
				return {{1000.0000000f, -269.6632080f}};
			case 335:
				return {{1000.0000000f, -269.6419678f}};
			case 336:
				return {{1000.0000000f, -269.6209717f}};
			case 337:
				return {{1000.0000000f, -269.3181152f}};
			case 338:
				return {{1000.0000000f, -269.3011475f}};
			case 339:
				return {{1000.0000000f, -269.2844238f}};
			case 340:
				return {{1000.0000000f, -269.0446777f}};
			case 341:
				return {{1000.0000000f, -269.0313721f}};
			case 342:
				return {{1000.0000000f, -268.8522949f}};
			case 343:
				return {{1000.0000000f, -268.6999512f}};
			case 344:
				return {{1000.0000000f, -268.5876465f}};
			case 345:
				return {{1000.0000000f, -268.5810547f}};
			case 346:
				return {{1000.0000000f, -268.5745850f}};
			case 347:
				return {{1000.0000000f, -268.4848633f}};
			case 348:
				return {{1000.0000000f, -268.4801025f}};
			case 349:
				return {{1000.0000000f, -268.4140625f}};
			case 350:
				return {{1000.0000000f, -268.4105225f}};
			case 351:
				return {{1000.0000000f, -268.4071045f}};
			case 352:
				return {{1000.0000000f, -268.3635254f}};
			case 353:
				return {{1000.0000000f, -268.3610840f}};
			case 354:
				return {{1000.0000000f, -268.3291016f}};
			case 355:
				return {{1000.0000000f, -268.3275146f}};
			case 356:
				return {{1000.0000000f, -268.3259277f}};
			case 357:
				return {{1000.0000000f, -268.3061523f}};
			case 358:
				return {{1000.0000000f, -268.3051758f}};
			case 359:
				return {{1000.0000000f, -268.2946777f}};
			case 360:
				return {{1000.0000000f, -268.2941895f}};
			case 361:
				return {{1000.0000000f, -268.2937012f}};
			case 362:
				return {{1000.0000000f, -268.2893066f}};
			case 363:
				return {{1000.0000000f, -268.2891846f}};
			case 364:
				return {{1000.0000000f, -268.2890625f}};
			case 365:
				return {{1000.0000000f, -268.2889404f}};
			case 366:
				return {{1000.0000000f, -268.2896729f}};
			case 367:
				return {{1000.0000000f, -268.2897949f}};
			case 368:
				return {{1000.0000000f, -268.2937012f}};
			case 369:
				return {{1000.0000000f, -268.2940674f}};
			case 370:
				return {{1000.0000000f, -268.3001709f}};
			case 371:
				return {{1000.0000000f, -268.3006592f}};
			case 372:
				return {{1000.0000000f, -268.3011475f}};
			case 373:
				return {{1000.0000000f, -268.3082275f}};
			case 374:
				return {{1000.0000000f, -268.3164062f}};
			case 375:
				return {{1000.0000000f, -268.3248291f}};
			case 376:
				return {{1000.0000000f, -268.3253174f}};
			case 377:
				return {{1000.0000000f, -268.3258057f}};
			case 378:
				return {{1000.0000000f, -268.3334961f}};
			case 379:
				return {{1000.0000000f, -268.3339844f}};
			case 380:
				return {{1000.0000000f, -268.3344727f}};
			case 381:
				return {{1000.0000000f, -268.3422852f}};
			case 382:
				return {{1000.0000000f, -268.3427734f}};
			case 383:
				return {{1000.0000000f, -268.3432617f}};
			case 384:
				return {{1000.0000000f, -268.3500977f}};
			case 385:
				return {{1000.0000000f, -268.3505859f}};
			case 386:
				return {{1000.0000000f, -268.3510742f}};
			case 387:
				return {{1000.0000000f, -268.3577881f}};
			case 388:
				return {{1000.0000000f, -268.3581543f}};
			case 389:
				return {{1000.0000000f, -268.3641357f}};
			case 390:
				return {{1000.0000000f, -268.3645020f}};
			case 391:
				return {{1000.0000000f, -268.3648682f}};
			case 392:
				return {{1000.0000000f, -268.3698730f}};
			case 393:
				return {{1000.0000000f, -268.3702393f}};
			case 394:
				return {{1000.0000000f, -268.3706055f}};
			case 395:
				return {{1000.0000000f, -268.3752441f}};
			case 396:
				return {{1000.0000000f, -268.3754883f}};
			case 397:
				return {{1000.0000000f, -268.3757324f}};
			case 398:
				return {{1000.0000000f, -268.3797607f}};
			case 399:
				return {{1000.0000000f, -268.3800049f}};
			case 400:
				return {{1000.0000000f, -268.3831787f}};
			case 401:
				return {{1000.0000000f, -268.3834229f}};
			case 402:
				return {{1000.0000000f, -268.3836670f}};
			case 403:
				return {{1000.0000000f, -268.3865967f}};
			case 404:
				return {{1000.0000000f, -268.3867188f}};
			case 405:
				return {{1000.0000000f, -268.3868408f}};
			case 406:
				return {{1000.0000000f, -268.3890381f}};
			case 407:
				return {{1000.0000000f, -268.3911133f}};
			case 408:
				return {{1000.0000000f, -268.3928223f}};
			case 409:
				return {{1000.0000000f, -268.3929443f}};
			case 410:
				return {{1000.0000000f, -268.3930664f}};
			case 411:
				return {{1000.0000000f, -268.3942871f}};
			case 412:
				return {{1000.0000000f, -268.3944092f}};
			case 413:
				return {{1000.0000000f, -268.3945312f}};
			case 414:
				return {{1000.0000000f, -268.3956299f}};
			case 415:
				return {{1000.0000000f, -268.3956299f}};
			case 416:
				return {{1000.0000000f, -268.3956299f}};
			case 417:
				return {{1000.0000000f, -268.3964844f}};
			case 418:
				return {{1000.0000000f, -268.3964844f}};
			case 419:
				return {{1000.0000000f, -268.3964844f}};
			case 420:
				return {{1000.0000000f, -268.3970947f}};
			case 421:
				return {{1000.0000000f, -268.3970947f}};
			case 422:
				return {{1000.0000000f, -268.3975830f}};
			case 423:
				return {{1000.0000000f, -268.3975830f}};
			case 424:
				return {{1000.0000000f, -268.3975830f}};
			case 425:
				return {{1000.0000000f, -268.3979492f}};
			case 426:
				return {{1000.0000000f, -268.3979492f}};
			case 427:
				return {{1000.0000000f, -268.3979492f}};
			case 428:
				return {{1000.0000000f, -268.3981934f}};
			case 429:
				return {{1000.0000000f, -268.3981934f}};
			case 430:
				return {{1000.0000000f, -268.3981934f}};
			case 431:
				return {{1000.0000000f, -268.3984375f}};
			case 432:
				return {{1000.0000000f, -268.3984375f}};
			case 433:
				return {{1000.0000000f, -268.3984375f}};
			case 434:
				return {{1000.0000000f, -268.3985596f}};
			case 435:
				return {{1000.0000000f, -268.3985596f}};
			case 436:
				return {{1000.0000000f, -268.3985596f}};
			case 437:
				return {{1000.0000000f, -268.3986816f}};
			case 438:
				return {{1000.0000000f, -268.3986816f}};
			case 439:
				return {{1000.0000000f, -268.3986816f}};
			case 440:
				return {{1000.0000000f, -268.3986816f}};
			case 441:
				return {{1000.0000000f, -268.3986816f}};
			case 442:
				return {{1000.0000000f, -268.3986816f}};
			case 443:
				return {{1000.0000000f, -268.3986816f}};
			case 444:
				return {{1000.0000000f, -268.3986816f}};
			case 445:
				return {{1000.0000000f, -268.3986816f}};
			case 446:
				return {{1000.0000000f, -268.3986816f}};
			case 447:
				return {{1000.0000000f, -268.3986816f}};
			case 448:
				return {{1000.0000000f, -268.3986816f}};
			case 449:
				return {{1000.0000000f, -268.3986816f}};
			case 450:
				return {{1000.0000000f, -268.3986816f}};
			case 451:
				return {{1000.0000000f, -268.3986816f}};
			case 452:
				return {{1000.0000000f, -268.3986816f}};
			case 453:
				return {{1000.0000000f, -268.3986816f}};
			case 454:
				return {{1000.0000000f, -268.3986816f}};
			case 455:
				return {{1000.0000000f, -268.3986816f}};
			case 456:
				return {{1000.0000000f, -268.3986816f}};
			case 457:
				return {{1000.0000000f, -268.3986816f}};
			case 458:
				return {{1000.0000000f, -268.3986816f}};
			case 459:
				return {{1000.0000000f, -268.3986816f}};
			case 460:
				return {{1000.0000000f, -268.3986816f}};
			case 461:
				return {{1000.0000000f, -268.3986816f}};
			case 462:
				return {{1000.0000000f, -268.3986816f}};
			case 463:
				return {{1000.0000000f, -268.3986816f}};
			case 464:
				return {{1000.0000000f, -268.3986816f}};
			case 465:
				return {{1000.0000000f, -268.3986816f}};
			case 466:
				return {{1000.0000000f, -268.3986816f}};
			case 467:
				return {{1000.0000000f, -268.3986816f}};
			case 468:
				return {{1000.0000000f, -268.3986816f}};
			case 469:
				return {{1000.0000000f, -268.3986816f}};
			case 470:
				return {{1000.0000000f, -268.3986816f}};
			case 471:
				return {{1000.0000000f, -268.3986816f}};
			case 472:
				return {{1000.0000000f, -268.3986816f}};
			case 473:
				return {{1000.0000000f, -268.3986816f}};
			case 474:
				return {{1000.0000000f, -268.3986816f}};
			case 475:
				return {{1000.0000000f, -268.3986816f}};
			case 476:
				return {{1000.0000000f, -268.3986816f}};
			case 477:
				return {{1000.0000000f, -268.3986816f}};
			case 478:
				return {{1000.0000000f, -268.3986816f}};
			case 479:
				return {{1000.0000000f, -268.3986816f}};
			case 480:
				return {{1000.0000000f, -268.3986816f}};
			case 481:
				return {{1000.0000000f, -268.3986816f}};
			case 482:
				return {{1000.0000000f, -268.3986816f}};
			case 483:
				return {{1000.0000000f, -268.3986816f}};
			case 484:
				return {{1000.0000000f, -268.3986816f}};
			case 485:
				return {{1000.0000000f, -268.3986816f}};
			case 486:
				return {{1000.0000000f, -268.3986816f}};
			case 487:
				return {{1000.0000000f, -268.3986816f}};
			case 488:
				return {{1000.0000000f, -268.3986816f}};
			case 489:
				return {{1000.0000000f, -268.3986816f}};
			case 490:
				return {{1000.0000000f, -268.3986816f}};
			case 491:
				return {{1000.0000000f, -268.3986816f}};
			case 492:
				return {{1000.0000000f, -268.3986816f}};
			case 493:
				return {{1000.0000000f, -268.3986816f}};
			case 494:
				return {{1000.0000000f, -268.3986816f}};
			case 495:
				return {{1000.0000000f, -268.3986816f}};
			case 496:
				return {{1000.0000000f, -268.3986816f}};
			case 497:
				return {{1000.0000000f, -268.3986816f}};
			case 498:
				return {{1000.0000000f, -268.3986816f}};
			case 499:
				return {{1000.0000000f, -268.3986816f}};
			case 500:
				return {{1000.0000000f, -268.3986816f}};
			case 501:
				return {{1000.0000000f, -268.3986816f}};
			case 502:
				return {{1000.0000000f, -268.3986816f}};
			case 503:
				return {{1000.0000000f, -268.4003906f}};
			case 504:
				return {{1000.0000000f, -268.5119629f}};
			case 505:
				return {{1000.0000000f, -268.5187988f}};
			case 506:
				return {{1000.0000000f, -268.5261230f}};
			case 507:
				return {{1000.0000000f, -268.5340576f}};
			case 508:
				return {{1000.0000000f, -268.7397461f}};
			case 509:
				return {{1000.0000000f, -268.7570801f}};
			case 510:
				return {{1000.0000000f, -269.1196289f}};
			case 511:
				return {{1000.0000000f, -269.1470947f}};
			case 512:
				return {{1000.0000000f, -269.6362305f}};
			case 513:
				return {{1000.0000000f, -269.6735840f}};
			case 514:
				return {{1000.0000000f, -269.7115479f}};
			case 515:
				return {{1000.0000000f, -269.7502441f}};
			case 516:
				return {{1000.0000000f, -270.4521484f}};
			case 517:
				return {{1000.0000000f, -270.5007324f}};
			case 518:
				return {{1000.0000000f, -271.3583984f}};
			case 519:
				return {{1000.0000000f, -271.4165039f}};
			case 520:
				return {{1000.0000000f, -271.4752197f}};
			case 521:
				return {{1000.0000000f, -272.4211426f}};
			case 522:
				return {{1000.0000000f, -272.4881592f}};
			case 523:
				return {{1000.0000000f, -272.5556641f}};
			case 524:
				return {{1000.0000000f, -273.7036133f}};
			case 525:
				return {{1000.0000000f, -273.7790527f}};
			case 526:
				return {{1000.0000000f, -273.8549805f}};
			case 527:
				return {{1000.0000000f, -275.0477295f}};
			case 528:
				return {{1000.0000000f, -275.1303711f}};
			case 529:
				return {{1000.0000000f, -275.2135010f}};
			case 530:
				return {{1000.0000000f, -276.5964355f}};
			case 531:
				return {{1000.0000000f, -276.6856689f}};
			case 532:
				return {{1000.0000000f, -276.7753906f}};
			case 533:
				return {{1000.0000000f, -278.2567139f}};
			case 534:
				return {{1000.0000000f, -278.3516846f}};
			case 535:
				return {{1000.0000000f, -278.4470215f}};
			case 536:
				return {{1000.0000000f, -281.5242920f}};
			case 537:
				return {{1000.0000000f, -281.6258545f}};
			case 538:
				return {{1000.0000000f, -283.2861328f}};
			case 539:
				return {{1000.0000000f, -283.3916016f}};
			case 540:
				return {{1000.0000000f, -283.4974365f}};
			case 541:
				return {{1000.0000000f, -285.1104736f}};
			case 542:
				return {{1000.0000000f, -285.2193604f}};
			case 543:
				return {{1000.0000000f, -285.3284912f}};
			case 544:
				return {{1000.0000000f, -287.0988770f}};
			case 545:
				return {{1000.0000000f, -287.2105713f}};
			case 546:
				return {{1000.0000000f, -287.3225098f}};
			case 547:
				return {{1000.0000000f, -289.0195312f}};
			case 548:
				return {{1000.0000000f, -289.1335449f}};
			case 549:
				return {{1000.0000000f, -289.2476807f}};
			case 550:
				return {{1000.0000000f, -291.0899658f}};
			case 551:
				return {{1000.0000000f, -291.2056885f}};
			case 552:
				return {{1000.0000000f, -293.0711670f}};
			case 553:
				return {{1000.0000000f, -293.1882324f}};
			case 554:
				return {{1000.0000000f, -294.9539795f}};
			case 555:
				return {{1000.0000000f, -295.0720215f}};
			case 556:
				return {{1000.0000000f, -295.1901855f}};
			case 557:
				return {{1000.0000000f, -297.0895996f}};
			case 558:
				return {{1000.0000000f, -297.2084961f}};
			case 559:
				return {{1000.0000000f, -297.3275146f}};
			case 560:
				return {{1000.0000000f, -299.2384033f}};
			case 561:
				return {{1000.0000000f, -299.3579102f}};
			case 562:
				return {{1000.0000000f, -299.4775391f}};
			case 563:
				return {{1000.0000000f, -301.2768555f}};
			case 564:
				return {{1000.0000000f, -301.3968506f}};
			case 565:
				return {{1000.0000000f, -301.5169678f}};
			case 566:
				return {{1000.0000000f, -303.4426270f}};
			case 567:
				return {{1000.0000000f, -303.5628662f}};
			case 568:
				return {{1000.0000000f, -305.3707275f}};
			case 569:
				return {{1000.0000000f, -307.3005371f}};
			case 570:
				return {{1000.0000000f, -309.2303467f}};
			case 571:
				return {{1000.0000000f, -309.3508301f}};
			case 572:
				return {{1000.0000000f, -309.4713135f}};
			case 573:
				return {{1000.0000000f, -311.2819824f}};
			case 574:
				return {{1000.0000000f, -311.4025879f}};
			case 575:
				return {{1000.0000000f, -311.5231934f}};
			case 576:
				return {{1000.0000000f, -313.4558105f}};
			case 577:
				return {{1000.0000000f, -313.5764160f}};
			case 578:
				return {{1000.0000000f, -313.6970215f}};
			case 579:
				return {{1000.0000000f, -315.6292725f}};
			case 580:
				return {{1000.0000000f, -315.7498779f}};
			case 581:
				return {{1000.0000000f, -315.8704834f}};
			case 582:
				return {{1000.0000000f, -317.6815186f}};
			case 583:
				return {{1000.0000000f, -317.8021240f}};
			case 584:
				return {{1000.0000000f, -319.7330322f}};
			case 585:
				return {{1000.0000000f, -319.8535156f}};
			case 586:
				return {{1000.0000000f, -319.9739990f}};
			case 587:
				return {{1000.0000000f, -321.7834473f}};
			case 588:
				return {{1000.0000000f, -321.9039307f}};
			case 589:
				return {{1000.0000000f, -322.0244141f}};
			case 590:
				return {{1000.0000000f, -323.9541016f}};
			case 591:
				return {{1000.0000000f, -324.0744629f}};
			case 592:
				return {{1000.0000000f, -324.1948242f}};
			case 593:
				return {{1000.0000000f, -326.1231689f}};
			case 594:
				return {{1000.0000000f, -326.2434082f}};
			case 595:
				return {{1000.0000000f, -326.3637695f}};
			case 596:
				return {{1000.0000000f, -328.1706543f}};
			case 597:
				return {{1000.0000000f, -328.2908936f}};
			case 598:
				return {{1000.0000000f, -328.4112549f}};
			case 599:
				return {{1000.0000000f, -330.3381348f}};
			case 600:
				return {{1000.0000000f, -330.4583740f}};
			case 601:
				return {{1000.0000000f, -334.1710205f}};
			case 602:
				return {{1000.0000000f, -334.2888184f}};
			case 603:
				return {{1000.0000000f, -336.1181641f}};
			case 604:
				return {{1000.0000000f, -336.2288818f}};
			case 605:
				return {{1000.0000000f, -337.8266602f}};
			case 606:
				return {{1000.0000000f, -337.9289551f}};
			case 607:
				return {{1000.0000000f, -338.0307617f}};
			case 608:
				return {{1000.0000000f, -339.5810547f}};
			case 609:
				return {{1000.0000000f, -339.6730957f}};
			case 610:
				return {{1000.0000000f, -341.0654297f}};
			case 611:
				return {{1000.0000000f, -341.1474609f}};
			case 612:
				return {{1000.0000000f, -342.3079834f}};
			case 613:
				return {{1000.0000000f, -342.3807373f}};
			case 614:
				return {{1000.0000000f, -343.4685059f}};
			case 615:
				return {{1000.0000000f, -343.5318604f}};
			case 616:
				return {{1000.0000000f, -343.5946045f}};
			case 617:
				return {{1000.0000000f, -344.5278320f}};
			case 618:
				return {{1000.0000000f, -344.5817871f}};
			case 619:
				return {{1000.0000000f, -344.6352539f}};
			case 620:
				return {{1000.0000000f, -345.3803711f}};
			case 621:
				return {{1000.0000000f, -345.4263916f}};
			case 622:
				return {{1000.0000000f, -346.1027832f}};
			case 623:
				return {{1000.0000000f, -346.1414795f}};
			case 624:
				return {{1000.0000000f, -346.1798096f}};
			case 625:
				return {{1000.0000000f, -346.7084961f}};
			case 626:
				return {{1000.0000000f, -346.7408447f}};
			case 627:
				return {{1000.0000000f, -346.7728271f}};
			case 628:
				return {{1000.0000000f, -347.6241455f}};
			case 629:
				return {{1000.0000000f, -347.6458740f}};
			case 630:
				return {{1000.0000000f, -347.9412842f}};
			case 631:
				return {{1000.0000000f, -347.9591064f}};
			case 632:
				return {{1000.0000000f, -347.9766846f}};
			case 633:
				return {{1000.0000000f, -348.2282715f}};
			case 634:
				return {{1000.0000000f, -348.2423096f}};
			case 635:
				return {{1000.0000000f, -348.4417725f}};
			case 636:
				return {{1000.0000000f, -348.4527588f}};
			case 637:
				return {{1000.0000000f, -348.4636230f}};
			case 638:
				return {{1000.0000000f, -348.6086426f}};
			case 639:
				return {{1000.0000000f, -348.6171875f}};
			case 640:
				return {{1000.0000000f, -348.6256104f}};
			case 641:
				return {{1000.0000000f, -348.7436523f}};
			case 642:
				return {{1000.0000000f, -348.7500000f}};
			case 643:
				return {{1000.0000000f, -348.8339844f}};
			case 644:
				return {{1000.0000000f, -348.8388672f}};
			case 645:
				return {{1000.0000000f, -348.8436279f}};
			case 646:
				return {{1000.0000000f, -348.9088135f}};
			case 647:
				return {{1000.0000000f, -348.9122314f}};
			case 648:
				return {{1000.0000000f, -348.9156494f}};
			case 649:
				return {{1000.0000000f, -348.9610596f}};
			case 650:
				return {{1000.0000000f, -348.9633789f}};
			case 651:
				return {{1000.0000000f, -348.9924316f}};
			case 652:
				return {{1000.0000000f, -348.9940186f}};
			case 653:
				return {{1000.0000000f, -348.9956055f}};
			case 654:
				return {{1000.0000000f, -349.0148926f}};
			case 655:
				return {{1000.0000000f, -349.0157471f}};
			case 656:
				return {{1000.0000000f, -349.0262451f}};
			case 657:
				return {{1000.0000000f, -349.0267334f}};
			case 658:
				return {{1000.0000000f, -349.0306396f}};
			case 659:
				return {{1000.0000000f, -349.0303955f}};
			case 660:
				return {{1000.0000000f, -349.0270996f}};
			case 661:
				return {{1000.0000000f, -349.0268555f}};
			case 662:
				return {{1000.0000000f, -349.0264893f}};
			case 663:
				return {{1000.0000000f, -349.0205078f}};
			case 664:
				return {{1000.0000000f, -349.0200195f}};
			case 665:
				return {{1000.0000000f, -349.0125732f}};
			case 666:
				return {{1000.0000000f, -349.0120850f}};
			case 667:
				return {{1000.0000000f, -349.0115967f}};
			case 668:
				return {{1000.0000000f, -349.0039062f}};
			case 669:
				return {{1000.0000000f, -349.0034180f}};
			case 670:
				return {{1000.0000000f, -349.0029297f}};
			case 671:
				return {{1000.0000000f, -348.9945068f}};
			case 672:
				return {{1000.0000000f, -348.9940186f}};
			case 673:
				return {{1000.0000000f, -348.9855957f}};
			case 674:
				return {{1000.0000000f, -348.9851074f}};
			case 675:
				return {{1000.0000000f, -348.9846191f}};
			case 676:
				return {{1000.0000000f, -348.9771729f}};
			case 677:
				return {{1000.0000000f, -348.9766846f}};
			case 678:
				return {{1000.0000000f, -348.9761963f}};
			case 679:
				return {{1000.0000000f, -348.9687500f}};
			case 680:
				return {{1000.0000000f, -348.9682617f}};
			case 681:
				return {{1000.0000000f, -348.9617920f}};
			case 682:
				return {{1000.0000000f, -348.9614258f}};
			case 683:
				return {{1000.0000000f, -348.9610596f}};
			case 684:
				return {{1000.0000000f, -348.9549561f}};
			case 685:
				return {{1000.0000000f, -348.9545898f}};
			case 686:
				return {{1000.0000000f, -348.9490967f}};
			case 687:
				return {{1000.0000000f, -348.9487305f}};
			case 688:
				return {{1000.0000000f, -348.9442139f}};
			case 689:
				return {{1000.0000000f, -348.9399414f}};
			case 690:
				return {{1000.0000000f, -348.9362793f}};
			case 691:
				return {{1000.0000000f, -348.9360352f}};
			case 692:
				return {{1000.0000000f, -348.9357910f}};
			case 693:
				return {{1000.0000000f, -348.9328613f}};
			case 694:
				return {{1000.0000000f, -348.9327393f}};
			case 695:
				return {{1000.0000000f, -348.9301758f}};
			case 696:
				return {{1000.0000000f, -348.9300537f}};
			case 697:
				return {{1000.0000000f, -348.9299316f}};
			case 698:
				return {{1000.0000000f, -348.9278564f}};
			case 699:
				return {{1000.0000000f, -348.9277344f}};
			case 700:
				return {{1000.0000000f, -348.9259033f}};
			case 701:
				return {{1000.0000000f, -348.9257812f}};
			case 702:
				return {{1000.0000000f, -348.9243164f}};
			case 703:
				return {{1000.0000000f, -348.9241943f}};
			case 704:
				return {{1000.0000000f, -348.9240723f}};
			case 705:
				return {{1000.0000000f, -348.9229736f}};
			case 706:
				return {{1000.0000000f, -348.9228516f}};
			case 707:
				return {{1000.0000000f, -348.9227295f}};
			case 708:
				return {{1000.0000000f, -348.9218750f}};
			case 709:
				return {{1000.0000000f, -348.9218750f}};
			case 710:
				return {{1000.0000000f, -348.9211426f}};
			case 711:
				return {{1000.0000000f, -348.9211426f}};
			case 712:
				return {{1000.0000000f, -348.9211426f}};
			case 713:
				return {{1000.0000000f, -348.9206543f}};
			case 714:
				return {{1000.0000000f, -348.9206543f}};
			case 715:
				return {{1000.0000000f, -348.9206543f}};
			case 716:
				return {{1000.0000000f, -348.9202881f}};
			case 717:
				return {{1000.0000000f, -348.9202881f}};
			case 718:
				return {{1000.0000000f, -348.9200439f}};
			case 719:
				return {{1000.0000000f, -348.9197998f}};
			case 720:
				return {{1000.0000000f, -348.9196777f}};
			case 721:
				return {{1000.0000000f, -348.9196777f}};
			case 722:
				return {{1000.0000000f, -348.9196777f}};
			case 723:
				return {{1000.0000000f, -348.9195557f}};
			case 724:
				return {{1000.0000000f, -348.9195557f}};
			case 725:
				return {{1000.0000000f, -348.9194336f}};
			case 726:
				return {{1000.0000000f, -348.9194336f}};
			case 727:
				return {{1000.0000000f, -348.9194336f}};
			case 728:
				return {{1000.0000000f, -348.9194336f}};
			case 729:
				return {{1000.0000000f, -348.9194336f}};
			case 730:
				return {{1000.0000000f, -348.9194336f}};
			case 731:
				return {{1000.0000000f, -348.9194336f}};
			case 732:
				return {{1000.0000000f, -348.9194336f}};
			case 733:
				return {{1000.0000000f, -348.9194336f}};
			case 734:
				return {{1000.0000000f, -348.9194336f}};
			case 735:
				return {{1000.0000000f, -348.9194336f}};
			case 736:
				return {{1000.0000000f, -348.9194336f}};
			case 737:
				return {{1000.0000000f, -348.9194336f}};
			case 738:
				return {{1000.0000000f, -348.9194336f}};
			case 739:
				return {{1000.0000000f, -348.9194336f}};
			case 740:
				return {{1000.0000000f, -348.9194336f}};
			case 741:
				return {{1000.0000000f, -348.9194336f}};
			case 742:
				return {{1000.0000000f, -348.9194336f}};
			case 743:
				return {{1000.0000000f, -348.9194336f}};
			case 744:
				return {{1000.0000000f, -348.9194336f}};
			case 745:
				return {{1000.0000000f, -348.9194336f}};
			case 746:
				return {{1000.0000000f, -348.9194336f}};
			case 747:
				return {{1000.0000000f, -348.9194336f}};
			case 748:
				return {{1000.0000000f, -348.9195557f}};
			case 749:
				return {{1000.0000000f, -348.9195557f}};
			case 750:
				return {{1000.0000000f, -348.9195557f}};
			case 751:
				return {{1000.0000000f, -348.9195557f}};
			case 752:
				return {{1000.0000000f, -348.9195557f}};
			case 753:
				return {{1000.0000000f, -348.9195557f}};
			case 754:
				return {{1000.0000000f, -348.9195557f}};
			case 755:
				return {{1000.0000000f, -348.9195557f}};
			case 756:
				return {{1000.0000000f, -348.9195557f}};
			case 757:
				return {{1000.0000000f, -348.9195557f}};
			case 758:
				return {{1000.0000000f, -348.9195557f}};
			case 759:
				return {{1000.0000000f, -348.9195557f}};
			case 760:
				return {{1000.0000000f, -348.9195557f}};
			case 761:
				return {{1000.0000000f, -348.9195557f}};
			case 762:
				return {{1000.0000000f, -348.9195557f}};
			case 763:
				return {{1000.0000000f, -348.9195557f}};
			case 764:
				return {{1000.0000000f, -348.9195557f}};
			case 765:
				return {{1000.0000000f, -348.9195557f}};
			case 766:
				return {{1000.0000000f, -348.9195557f}};
			case 767:
				return {{1000.0000000f, -348.9195557f}};
			case 768:
				return {{1000.0000000f, -348.9195557f}};
			case 769:
				return {{1000.0000000f, -348.9195557f}};
			case 770:
				return {{1000.0000000f, -348.9195557f}};
			case 771:
				return {{1000.0000000f, -348.9195557f}};
			case 772:
				return {{1000.0000000f, -348.9195557f}};
			case 773:
				return {{1000.0000000f, -348.9195557f}};
			case 774:
				return {{1000.0000000f, -348.9195557f}};
			case 775:
				return {{1000.0000000f, -348.9195557f}};
			case 776:
				return {{1000.0000000f, -348.9195557f}};
			case 777:
				return {{1000.0000000f, -348.9195557f}};
			case 778:
				return {{1000.0000000f, -348.9195557f}};
			case 779:
				return {{1000.0000000f, -348.9195557f}};
			case 780:
				return {{1000.0000000f, -348.9195557f}};
			case 781:
				return {{1000.0000000f, -348.9195557f}};
			case 782:
				return {{1000.0000000f, -348.9195557f}};
			case 783:
				return {{1000.0000000f, -348.9195557f}};
			case 784:
				return {{1000.0000000f, -348.9195557f}};
			case 785:
				return {{1000.0000000f, -348.9195557f}};
			case 786:
				return {{1000.0000000f, -348.9195557f}};
			case 787:
				return {{1000.0000000f, -348.9195557f}};
			case 788:
				return {{1000.0000000f, -348.9195557f}};
			case 789:
				return {{1000.0000000f, -348.9195557f}};
			case 790:
				return {{1000.0000000f, -348.9195557f}};
			case 791:
				return {{1000.0000000f, -348.9195557f}};
			case 792:
				return {{1000.0000000f, -348.9195557f}};
			case 793:
				return {{1000.0000000f, -348.9195557f}};
			case 794:
				return {{1000.0000000f, -348.9195557f}};
			case 795:
				return {{1000.0000000f, -348.9195557f}};
			case 796:
				return {{1000.0000000f, -348.9195557f}};
			case 797:
				return {{1000.0000000f, -348.9195557f}};
			case 798:
				return {{1000.0000000f, -348.9195557f}};
			case 799:
				return {{1000.0000000f, -348.9195557f}};
			case 800:
				return {{1000.0000000f, -348.9195557f}};
			case 801:
				return {{1000.0000000f, -348.9195557f}};
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
				return {{1000.0000000f, 1158.5111084f}};
			case 502:
				return {{1000.0000000f, 1175.1633301f}};
			case 601:
				return {{1000.0000000f, 1109.3956299f}};
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
				return {{1000.0000000f, 1180.9300537f}};
			case 502:
				return {{1000.0000000f, 1197.5822754f}};
			case 601:
				return {{1000.0000000f, 1131.8121338f}};
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
				return {{1000.0000000f, 1188.2886963f}};
			case 502:
				return {{1000.0000000f, 1204.9409180f}};
			case 601:
				return {{1000.0000000f, 1139.1701660f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == k_ScreenCentreLine[12])
		{
			switch (frameNumber)
			{
			case 0:
				return {{0.0000000f, -4.5307636f}};
			case 202:
				return {{1000.0000000f, 1108.6668701f}};
			case 301:
				return {{1000.0000000f, 1192.0982666f}};
			case 502:
				return {{1000.0000000f, 1208.7504883f}};
			case 601:
				return {{1000.0000000f, 1142.9772949f}};
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
				return {{1000.0000000f, 1194.3256836f}};
			case 502:
				return {{1000.0000000f, 1210.9779053f}};
			case 601:
				return {{1000.0000000f, 1145.2064209f}};
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
				return {{1000.0000000f, 1195.8568115f}};
			case 502:
				return {{1000.0000000f, 1212.5090332f}};
			case 601:
				return {{1000.0000000f, 1146.7368164f}};
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
				return {{1000.0000000f, 1196.9222412f}};
			case 502:
				return {{1000.0000000f, 1213.5744629f}};
			case 601:
				return {{1000.0000000f, 1147.8027344f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == k_MockMousePos)
		{
			switch (frameNumber)
			{
			case 0:
				return {{15.8141842f, -13.4212151f}};
			case 1:
				return {{998.6565552f, 1118.0327148f}};
			case 2:
				return {{998.6566772f, 1118.0330811f}};
			case 3:
				return {{998.6575928f, 1118.0344238f}};
			case 4:
				return {{998.6578369f, 1118.0350342f}};
			case 5:
				return {{998.6578979f, 1118.0350342f}};
			case 6:
				return {{998.6578979f, 1118.0351562f}};
			case 7:
				return {{998.6581421f, 1118.0354004f}};
			case 8:
				return {{998.6581421f, 1118.0354004f}};
			case 9:
				return {{998.6583862f, 1118.0360107f}};
			case 10:
				return {{998.6583862f, 1118.0361328f}};
			case 11:
				return {{998.6583862f, 1118.0361328f}};
			case 12:
				return {{998.6585083f, 1118.0363770f}};
			case 13:
				return {{998.6585083f, 1118.0363770f}};
			case 14:
				return {{998.6585083f, 1118.0363770f}};
			case 15:
				return {{998.6585083f, 1118.0363770f}};
			case 16:
				return {{998.6586914f, 1118.0366211f}};
			case 17:
				return {{998.6586914f, 1118.0366211f}};
			case 18:
				return {{998.6588135f, 1118.0368652f}};
			case 19:
				return {{998.6589355f, 1118.0369873f}};
			case 20:
				return {{998.6591187f, 1118.0371094f}};
			case 21:
				return {{998.6591187f, 1118.0371094f}};
			case 22:
				return {{998.6591187f, 1118.0375977f}};
			case 23:
				return {{998.6591187f, 1118.0375977f}};
			case 24:
				return {{998.6593018f, 1118.0377197f}};
			case 25:
				return {{998.6593018f, 1118.0377197f}};
			case 26:
				return {{998.6593628f, 1118.0378418f}};
			case 27:
				return {{998.6593628f, 1118.0378418f}};
			case 28:
				return {{998.6593628f, 1118.0378418f}};
			case 29:
				return {{998.6594238f, 1118.0379639f}};
			case 30:
				return {{998.6594238f, 1118.0380859f}};
			case 31:
				return {{998.6594238f, 1118.0380859f}};
			case 32:
				return {{998.6595459f, 1118.0380859f}};
			case 33:
				return {{998.6595459f, 1118.0380859f}};
			case 34:
				return {{998.6596069f, 1118.0382080f}};
			case 35:
				return {{998.6596069f, 1118.0382080f}};
			case 36:
				return {{998.6596680f, 1118.0382080f}};
			case 37:
				return {{998.6596680f, 1118.0382080f}};
			case 38:
				return {{998.6596680f, 1118.0382080f}};
			case 39:
				return {{998.6050415f, 1117.9583740f}};
			case 40:
				return {{998.6004639f, 1117.9519043f}};
			case 41:
				return {{998.5250854f, 1117.8411865f}};
			case 42:
				return {{998.5202026f, 1117.8342285f}};
			case 43:
				return {{998.4447632f, 1117.7239990f}};
			case 44:
				return {{998.3740845f, 1117.6204834f}};
			case 45:
				return {{998.2989502f, 1117.5107422f}};
			case 46:
				return {{998.2943726f, 1117.5034180f}};
			case 47:
				return {{998.2241821f, 1117.4010010f}};
			case 48:
				return {{998.2192993f, 1117.3937988f}};
			case 49:
				return {{998.1444702f, 1117.2852783f}};
			case 50:
				return {{998.1400146f, 1117.2783203f}};
			case 51:
				return {{998.0661011f, 1117.1699219f}};
			case 52:
				return {{998.0614624f, 1117.1633301f}};
			case 53:
				return {{997.9931030f, 1117.0632324f}};
			case 54:
				return {{997.9883423f, 1117.0559082f}};
			case 55:
				return {{997.9157104f, 1116.9499512f}};
			case 56:
				return {{997.9113770f, 1116.9432373f}};
			case 57:
				return {{997.8395996f, 1116.8388672f}};
			case 58:
				return {{997.8350830f, 1116.8316650f}};
			case 59:
				return {{997.7686157f, 1116.7346191f}};
			case 60:
				return {{997.7645264f, 1116.7280273f}};
			case 61:
				return {{997.6946411f, 1116.6263428f}};
			case 62:
				return {{997.6901855f, 1116.6201172f}};
			case 63:
				return {{997.6260376f, 1116.5261230f}};
			case 64:
				return {{997.6213379f, 1116.5197754f}};
			case 65:
				return {{997.5543213f, 1116.4202881f}};
			case 66:
				return {{997.4876709f, 1116.3229980f}};
			case 67:
				return {{997.4268799f, 1116.2340088f}};
			case 68:
				return {{997.4226074f, 1116.2282715f}};
			case 69:
				return {{997.3584595f, 1116.1345215f}};
			case 70:
				return {{997.3546753f, 1116.1284180f}};
			case 71:
				return {{997.2918701f, 1116.0371094f}};
			case 72:
				return {{997.2882690f, 1116.0312500f}};
			case 73:
				return {{997.2311401f, 1115.9484863f}};
			case 74:
				return {{997.2274780f, 1115.9427490f}};
			case 75:
				return {{997.2233887f, 1115.9372559f}};
			case 76:
				return {{997.1643066f, 1115.8500977f}};
			case 77:
				return {{997.1604004f, 1115.8443604f}};
			case 78:
				return {{997.1060791f, 1115.7656250f}};
			case 79:
				return {{997.1025391f, 1115.7595215f}};
			case 80:
				return {{997.0460205f, 1115.6776123f}};
			case 81:
				return {{997.0431519f, 1115.6727295f}};
			case 82:
				return {{996.9878540f, 1115.5917969f}};
			case 83:
				return {{996.9848022f, 1115.5877686f}};
			case 84:
				return {{996.9347534f, 1115.5148926f}};
			case 85:
				return {{996.9316406f, 1115.5096436f}};
			case 86:
				return {{996.8798828f, 1115.4344482f}};
			case 87:
				return {{996.8767700f, 1115.4294434f}};
			case 88:
				return {{996.8270874f, 1115.3565674f}};
			case 89:
				return {{996.8242188f, 1115.3520508f}};
			case 90:
				return {{996.7781982f, 1115.2857666f}};
			case 91:
				return {{996.7322388f, 1115.2180176f}};
			case 92:
				return {{996.6903687f, 1115.1567383f}};
			case 93:
				return {{996.6872559f, 1115.1528320f}};
			case 94:
				return {{996.6439209f, 1115.0891113f}};
			case 95:
				return {{996.6413574f, 1115.0844727f}};
			case 96:
				return {{996.5997314f, 1115.0242920f}};
			case 97:
				return {{996.5974121f, 1115.0205078f}};
			case 98:
				return {{996.5593872f, 1114.9663086f}};
			case 99:
				return {{996.5569458f, 1114.9620361f}};
			case 100:
				return {{996.5190430f, 1114.9064941f}};
			case 101:
				return {{996.5166626f, 1114.9024658f}};
			case 102:
				return {{996.4803467f, 1114.8493652f}};
			case 103:
				return {{996.4471436f, 1114.8009033f}};
			case 104:
				return {{996.4453735f, 1114.7979736f}};
			case 105:
				return {{996.4117432f, 1114.7495117f}};
			case 106:
				return {{996.4096069f, 1114.7465820f}};
			case 107:
				return {{996.4075928f, 1114.7440186f}};
			case 108:
				return {{996.3780518f, 1114.7004395f}};
			case 109:
				return {{996.3764648f, 1114.6977539f}};
			case 110:
				return {{996.3460693f, 1114.6536865f}};
			case 111:
				return {{996.3442383f, 1114.6506348f}};
			case 112:
				return {{996.3157959f, 1114.6094971f}};
			case 113:
				return {{996.3142090f, 1114.6070557f}};
			case 114:
				return {{996.2634277f, 1114.5322266f}};
			case 115:
				return {{996.2390137f, 1114.4976807f}};
			case 116:
				return {{996.2378540f, 1114.4953613f}};
			case 117:
				return {{996.2164307f, 1114.4639893f}};
			case 118:
				return {{996.2153320f, 1114.4620361f}};
			case 119:
				return {{996.1939087f, 1114.4304199f}};
			case 120:
				return {{996.1929932f, 1114.4294434f}};
			case 121:
				return {{996.1745605f, 1114.4014893f}};
			case 122:
				return {{996.1728516f, 1114.3999023f}};
			case 123:
				return {{996.1718140f, 1114.3980713f}};
			case 124:
				return {{996.1535034f, 1114.3715820f}};
			case 125:
				return {{996.1361694f, 1114.3464355f}};
			case 126:
				return {{996.1347656f, 1114.3447266f}};
			case 127:
				return {{996.1205444f, 1114.3227539f}};
			case 128:
				return {{996.1194458f, 1114.3221436f}};
			case 129:
				return {{996.1185303f, 1114.3204346f}};
			case 130:
				return {{996.1038818f, 1114.2991943f}};
			case 131:
				return {{996.0902100f, 1114.2783203f}};
			case 132:
				return {{996.0897217f, 1114.2772217f}};
			case 133:
				return {{996.0779419f, 1114.2603760f}};
			case 134:
				return {{996.0769653f, 1114.2600098f}};
			case 135:
				return {{996.0654297f, 1114.2429199f}};
			case 136:
				return {{996.0649414f, 1114.2418213f}};
			case 137:
				return {{996.0555420f, 1114.2282715f}};
			case 138:
				return {{996.0457764f, 1114.2138672f}};
			case 139:
				return {{996.0378418f, 1114.2017822f}};
			case 140:
				return {{996.0368652f, 1114.2006836f}};
			case 141:
				return {{996.0293579f, 1114.1906738f}};
			case 142:
				return {{996.0292969f, 1114.1895752f}};
			case 143:
				return {{996.0226440f, 1114.1788330f}};
			case 144:
				return {{996.0218506f, 1114.1791992f}};
			case 145:
				return {{996.0155640f, 1114.1700439f}};
			case 146:
				return {{996.0155029f, 1114.1693115f}};
			case 147:
				return {{996.0106812f, 1114.1623535f}};
			case 148:
				return {{996.0104370f, 1114.1616211f}};
			case 149:
				return {{996.0059204f, 1114.1550293f}};
			case 150:
				return {{996.0051270f, 1114.1542969f}};
			case 151:
				return {{996.0014648f, 1114.1489258f}};
			case 152:
				return {{996.0012817f, 1114.1485596f}};
			case 153:
				return {{995.9979248f, 1114.1435547f}};
			case 154:
				return {{995.9971924f, 1114.1424561f}};
			case 155:
				return {{995.9947510f, 1114.1383057f}};
			case 156:
				return {{995.9942017f, 1114.1379395f}};
			case 157:
				return {{995.9913330f, 1114.1345215f}};
			case 158:
				return {{995.9914551f, 1114.1347656f}};
			case 159:
				return {{995.9893799f, 1114.1317139f}};
			case 160:
				return {{995.9888916f, 1114.1317139f}};
			case 161:
				return {{995.9858398f, 1114.1260986f}};
			case 162:
				return {{995.9853516f, 1114.1248779f}};
			case 163:
				return {{995.9848633f, 1114.1246338f}};
			case 164:
				return {{995.9840088f, 1114.1234131f}};
			case 165:
				return {{995.9839478f, 1114.1235352f}};
			case 166:
				return {{995.9832764f, 1114.1224365f}};
			case 167:
				return {{995.9831543f, 1114.1213379f}};
			case 168:
				return {{995.9825439f, 1114.1206055f}};
			case 169:
				return {{995.9827881f, 1114.1210938f}};
			case 170:
				return {{995.9824219f, 1114.1206055f}};
			case 171:
				return {{995.9821167f, 1114.1203613f}};
			case 172:
				return {{995.9817505f, 1114.1198730f}};
			case 173:
				return {{995.9819336f, 1114.1199951f}};
			case 174:
				return {{995.9816895f, 1114.1196289f}};
			case 175:
				return {{995.9813232f, 1114.1198730f}};
			case 176:
				return {{995.9811401f, 1114.1196289f}};
			case 177:
				return {{995.9811401f, 1114.1197510f}};
			case 178:
				return {{995.9810791f, 1114.1196289f}};
			case 179:
				return {{995.9812012f, 1114.1191406f}};
			case 180:
				return {{995.9811401f, 1114.1190186f}};
			case 181:
				return {{995.9811401f, 1114.1191406f}};
			case 182:
				return {{995.9811401f, 1114.1190186f}};
			case 183:
				return {{995.9811401f, 1114.1190186f}};
			case 184:
				return {{995.9811401f, 1114.1191406f}};
			case 185:
				return {{995.9811401f, 1114.1191406f}};
			case 186:
				return {{995.9811401f, 1114.1191406f}};
			case 187:
				return {{995.9811401f, 1114.1191406f}};
			case 188:
				return {{995.9811401f, 1114.1191406f}};
			case 189:
				return {{995.9811401f, 1114.1191406f}};
			case 190:
				return {{995.9812012f, 1114.1191406f}};
			case 191:
				return {{995.9811401f, 1114.1191406f}};
			case 192:
				return {{995.9812012f, 1114.1191406f}};
			case 193:
				return {{995.9812012f, 1114.1191406f}};
			case 194:
				return {{995.9812012f, 1114.1191406f}};
			case 195:
				return {{995.9812012f, 1114.1191406f}};
			case 196:
				return {{995.9812012f, 1114.1191406f}};
			case 197:
				return {{995.9811401f, 1114.1197510f}};
			case 198:
				return {{995.9811401f, 1114.1197510f}};
			case 199:
				return {{995.9811401f, 1114.1197510f}};
			case 200:
				return {{995.9811401f, 1114.1197510f}};
			case 201:
				return {{995.9811401f, 1114.1197510f}};
			case 202:
				return {{995.9812012f, 1114.1197510f}};
			case 203:
				return {{995.9812012f, 1114.1199951f}};
			case 204:
				return {{995.9812012f, 1114.1557617f}};
			case 205:
				return {{995.9811401f, 1114.1600342f}};
			case 206:
				return {{995.9812012f, 1114.4857178f}};
			case 207:
				return {{995.9812012f, 1114.7900391f}};
			case 208:
				return {{995.9812012f, 1114.8150635f}};
			case 209:
				return {{995.9812012f, 1114.8406982f}};
			case 210:
				return {{995.9812012f, 1115.3388672f}};
			case 211:
				return {{995.9812012f, 1115.3750000f}};
			case 212:
				return {{995.9813232f, 1116.0388184f}};
			case 213:
				return {{995.9812012f, 1116.0852051f}};
			case 214:
				return {{995.9812012f, 1116.8531494f}};
			case 215:
				return {{995.9812012f, 1116.9086914f}};
			case 216:
				return {{995.9812012f, 1117.8747559f}};
			case 217:
				return {{995.9813232f, 1117.9394531f}};
			case 218:
				return {{995.9812012f, 1119.0441895f}};
			case 219:
				return {{995.9813232f, 1119.1170654f}};
			case 220:
				return {{995.9813232f, 1120.2674561f}};
			case 221:
				return {{995.9813232f, 1120.3474121f}};
			case 222:
				return {{995.9813232f, 1121.6846924f}};
			case 223:
				return {{995.9813232f, 1121.7713623f}};
			case 224:
				return {{995.9813232f, 1123.1158447f}};
			case 225:
				return {{995.9813232f, 1123.2080078f}};
			case 226:
				return {{995.9813232f, 1124.7272949f}};
			case 227:
				return {{995.9813843f, 1124.8245850f}};
			case 228:
				return {{995.9813232f, 1126.4194336f}};
			case 229:
				return {{995.9813232f, 1126.5205078f}};
			case 230:
				return {{995.9813843f, 1129.7656250f}};
			case 231:
				return {{995.9813232f, 1131.4906006f}};
			case 232:
				return {{995.9813232f, 1131.5998535f}};
			case 233:
				return {{995.9813843f, 1131.7093506f}};
			case 234:
				return {{995.9813232f, 1133.3737793f}};
			case 235:
				return {{995.9813232f, 1135.1834717f}};
			case 236:
				return {{995.9813843f, 1135.2969971f}};
			case 237:
				return {{995.9813232f, 1137.0222168f}};
			case 238:
				return {{995.9813232f, 1137.1384277f}};
			case 239:
				return {{995.9813843f, 1139.0030518f}};
			case 240:
				return {{995.9813232f, 1139.1201172f}};
			case 241:
				return {{995.9813232f, 1141.0050049f}};
			case 242:
				return {{995.9813232f, 1141.1226807f}};
			case 243:
				return {{995.9813843f, 1142.9030762f}};
			case 244:
				return {{995.9813232f, 1143.0219727f}};
			case 245:
				return {{995.9813232f, 1144.9332275f}};
			case 246:
				return {{995.9813232f, 1145.0522461f}};
			case 247:
				return {{995.9813843f, 1146.9710693f}};
			case 248:
				return {{995.9813843f, 1147.0910645f}};
			case 249:
				return {{995.9813232f, 1148.8953857f}};
			case 250:
				return {{995.9813232f, 1149.0156250f}};
			case 251:
				return {{995.9813232f, 1150.9451904f}};
			case 252:
				return {{995.9813232f, 1151.0651855f}};
			case 253:
				return {{995.9813232f, 1151.1862793f}};
			case 254:
				return {{995.9813232f, 1154.9152832f}};
			case 255:
				return {{995.9813843f, 1156.8228760f}};
			case 256:
				return {{995.9813232f, 1156.9417725f}};
			case 257:
				return {{995.9813232f, 1158.7380371f}};
			case 258:
				return {{995.9813843f, 1158.8583984f}};
			case 259:
				return {{995.9813232f, 1158.9777832f}};
			case 260:
				return {{995.9813843f, 1160.9017334f}};
			case 261:
				return {{995.9813232f, 1161.0219727f}};
			case 262:
				return {{995.9813843f, 1162.9498291f}};
			case 263:
				return {{995.9813232f, 1163.0701904f}};
			case 264:
				return {{995.9813232f, 1164.8800049f}};
			case 265:
				return {{995.9813232f, 1166.8116455f}};
			case 266:
				return {{995.9813232f, 1166.9327393f}};
			case 267:
				return {{995.9813232f, 1168.7440186f}};
			case 268:
				return {{995.9813843f, 1168.8651123f}};
			case 269:
				return {{995.9813232f, 1170.7978516f}};
			case 270:
				return {{995.9813232f, 1170.9189453f}};
			case 271:
				return {{995.9813232f, 1172.8510742f}};
			case 272:
				return {{995.9813232f, 1172.9721680f}};
			case 273:
				return {{995.9813232f, 1174.7828369f}};
			case 274:
				return {{995.9813232f, 1174.9039307f}};
			case 275:
				return {{995.9813232f, 1175.0240479f}};
			case 276:
				return {{995.9813843f, 1176.9564209f}};
			case 277:
				return {{995.9813843f, 1177.0773926f}};
			case 278:
				return {{995.9813232f, 1177.1975098f}};
			case 279:
				return {{995.9813232f, 1179.1290283f}};
			case 280:
				return {{995.9813843f, 1180.9376221f}};
			case 281:
				return {{995.9813232f, 1182.8659668f}};
			case 282:
				return {{995.9813232f, 1182.9862061f}};
			case 283:
				return {{995.9813232f, 1183.1060791f}};
			case 284:
				return {{995.9813232f, 1184.9136963f}};
			case 285:
				return {{995.9813843f, 1185.0335693f}};
			case 286:
				return {{995.9813232f, 1185.1539307f}};
			case 287:
				return {{995.9813843f, 1187.0816650f}};
			case 288:
				return {{995.9813232f, 1187.2019043f}};
			case 289:
				return {{995.9813232f, 1187.3227539f}};
			case 290:
				return {{995.9813232f, 1189.2490234f}};
			case 291:
				return {{995.9813232f, 1189.3692627f}};
			case 292:
				return {{995.9813232f, 1189.4895020f}};
			case 293:
				return {{995.9813232f, 1191.2951660f}};
			case 294:
				return {{995.9813232f, 1191.4154053f}};
			case 295:
				return {{995.9813232f, 1193.3405762f}};
			case 296:
				return {{995.9813232f, 1193.4606934f}};
			case 297:
				return {{995.9813232f, 1193.5812988f}};
			case 298:
				return {{995.9813232f, 1195.5057373f}};
			case 299:
				return {{995.9813232f, 1195.6252441f}};
			case 300:
				return {{995.9813232f, 1195.7453613f}};
			case 301:
				return {{995.9813843f, 1197.5490723f}};
			case 302:
				return {{995.9813843f, 1197.6690674f}};
			case 303:
				return {{995.9813232f, 1199.5590820f}};
			case 304:
				return {{995.9813232f, 1199.6750488f}};
			case 305:
				return {{995.9813843f, 1199.7901611f}};
			case 306:
				return {{995.9813232f, 1201.4758301f}};
			case 307:
				return {{995.9813232f, 1201.5848389f}};
			case 308:
				return {{995.9813232f, 1203.2626953f}};
			case 309:
				return {{995.9813232f, 1204.8050537f}};
			case 310:
				return {{995.9813843f, 1204.8962402f}};
			case 311:
				return {{995.9813232f, 1206.2023926f}};
			case 312:
				return {{995.9813843f, 1206.2849121f}};
			case 313:
				return {{995.9813232f, 1206.3668213f}};
			case 314:
				return {{995.9813232f, 1207.5991211f}};
			case 315:
				return {{995.9813843f, 1207.6718750f}};
			case 316:
				return {{995.9813232f, 1207.7430420f}};
			case 317:
				return {{995.9813232f, 1208.8150635f}};
			case 318:
				return {{995.9813232f, 1208.8769531f}};
			case 319:
				return {{995.9813232f, 1209.7507324f}};
			case 320:
				return {{995.9813232f, 1209.8049316f}};
			case 321:
				return {{995.9813843f, 1209.8586426f}};
			case 322:
				return {{995.9813232f, 1210.6536865f}};
			case 323:
				return {{995.9813232f, 1210.6994629f}};
			case 324:
				return {{995.9813232f, 1210.7447510f}};
			case 325:
				return {{995.9813232f, 1211.3735352f}};
			case 326:
				return {{995.9813232f, 1211.4121094f}};
			case 327:
				return {{995.9813232f, 1211.4503174f}};
			case 328:
				return {{995.9813843f, 1212.0101318f}};
			case 329:
				return {{995.9813232f, 1212.0419922f}};
			case 330:
				return {{995.9813843f, 1212.0734863f}};
			case 331:
				return {{995.9813232f, 1212.5332031f}};
			case 332:
				return {{995.9813232f, 1212.5592041f}};
			case 333:
				return {{995.9813232f, 1212.5849609f}};
			case 334:
				return {{995.9813232f, 1212.9367676f}};
			case 335:
				return {{995.9813232f, 1212.9580078f}};
			case 336:
				return {{995.9813232f, 1212.9794922f}};
			case 337:
				return {{995.9813232f, 1213.2818604f}};
			case 338:
				return {{995.9813232f, 1213.2993164f}};
			case 339:
				return {{995.9813232f, 1213.3155518f}};
			case 340:
				return {{995.9813232f, 1213.5552979f}};
			case 341:
				return {{995.9813232f, 1213.5690918f}};
			case 342:
				return {{995.9813232f, 1213.7476807f}};
			case 343:
				return {{995.9813232f, 1213.9005127f}};
			case 344:
				return {{995.9813232f, 1214.0128174f}};
			case 345:
				return {{995.9813232f, 1214.0189209f}};
			case 346:
				return {{995.9813232f, 1214.0253906f}};
			case 347:
				return {{995.9813232f, 1214.1151123f}};
			case 348:
				return {{995.9813232f, 1214.1198730f}};
			case 349:
				return {{995.9813232f, 1214.1859131f}};
			case 350:
				return {{995.9813232f, 1214.1894531f}};
			case 351:
				return {{995.9813232f, 1214.1928711f}};
			case 352:
				return {{995.9813232f, 1214.2364502f}};
			case 353:
				return {{995.9813232f, 1214.2388916f}};
			case 354:
				return {{995.9813232f, 1214.2708740f}};
			case 355:
				return {{995.9813232f, 1214.2724609f}};
			case 356:
				return {{995.9813232f, 1214.2740479f}};
			case 357:
				return {{995.9813232f, 1214.2938232f}};
			case 358:
				return {{995.9813232f, 1214.2947998f}};
			case 359:
				return {{995.9813232f, 1214.3057861f}};
			case 360:
				return {{995.9813232f, 1214.3057861f}};
			case 361:
				return {{995.9813232f, 1214.3062744f}};
			case 362:
				return {{995.9813232f, 1214.3106689f}};
			case 363:
				return {{995.9813232f, 1214.3112793f}};
			case 364:
				return {{995.9813232f, 1214.3109131f}};
			case 365:
				return {{995.9813232f, 1214.3110352f}};
			case 366:
				return {{995.9813232f, 1214.3103027f}};
			case 367:
				return {{995.9813232f, 1214.3106689f}};
			case 368:
				return {{995.9813232f, 1214.3062744f}};
			case 369:
				return {{995.9813232f, 1214.3063965f}};
			case 370:
				return {{995.9813232f, 1214.3002930f}};
			case 371:
				return {{995.9813232f, 1214.2993164f}};
			case 372:
				return {{995.9813232f, 1214.2988281f}};
			case 373:
				return {{995.9813232f, 1214.2917480f}};
			case 374:
				return {{995.9813232f, 1214.2835693f}};
			case 375:
				return {{995.9813843f, 1214.2751465f}};
			case 376:
				return {{995.9813232f, 1214.2746582f}};
			case 377:
				return {{995.9813232f, 1214.2746582f}};
			case 378:
				return {{995.9813232f, 1214.2664795f}};
			case 379:
				return {{995.9813843f, 1214.2659912f}};
			case 380:
				return {{995.9813232f, 1214.2655029f}};
			case 381:
				return {{995.9813232f, 1214.2581787f}};
			case 382:
				return {{995.9813232f, 1214.2572021f}};
			case 383:
				return {{995.9813232f, 1214.2567139f}};
			case 384:
				return {{995.9813232f, 1214.2498779f}};
			case 385:
				return {{995.9813232f, 1214.2493896f}};
			case 386:
				return {{995.9813232f, 1214.2489014f}};
			case 387:
				return {{995.9813232f, 1214.2421875f}};
			case 388:
				return {{995.9813232f, 1214.2423096f}};
			case 389:
				return {{995.9813232f, 1214.2358398f}};
			case 390:
				return {{995.9813232f, 1214.2354736f}};
			case 391:
				return {{995.9813232f, 1214.2355957f}};
			case 392:
				return {{995.9813232f, 1214.2301025f}};
			case 393:
				return {{995.9813232f, 1214.2297363f}};
			case 394:
				return {{995.9813843f, 1214.2293701f}};
			case 395:
				return {{995.9813232f, 1214.2252197f}};
			case 396:
				return {{995.9813232f, 1214.2244873f}};
			case 397:
				return {{995.9813232f, 1214.2242432f}};
			case 398:
				return {{995.9813232f, 1214.2202148f}};
			case 399:
				return {{995.9813232f, 1214.2199707f}};
			case 400:
				return {{995.9813232f, 1214.2172852f}};
			case 401:
				return {{995.9813232f, 1214.2165527f}};
			case 402:
				return {{995.9813232f, 1214.2163086f}};
			case 403:
				return {{995.9813232f, 1214.2133789f}};
			case 404:
				return {{995.9813232f, 1214.2132568f}};
			case 405:
				return {{995.9813232f, 1214.2136230f}};
			case 406:
				return {{995.9813232f, 1214.2109375f}};
			case 407:
				return {{995.9813232f, 1214.2093506f}};
			case 408:
				return {{995.9813232f, 1214.2071533f}};
			case 409:
				return {{995.9813232f, 1214.2075195f}};
			case 410:
				return {{995.9813232f, 1214.2069092f}};
			case 411:
				return {{995.9813232f, 1214.2056885f}};
			case 412:
				return {{995.9813232f, 1214.2055664f}};
			case 413:
				return {{995.9813232f, 1214.2054443f}};
			case 414:
				return {{995.9813232f, 1214.2043457f}};
			case 415:
				return {{995.9813232f, 1214.2043457f}};
			case 416:
				return {{995.9813232f, 1214.2043457f}};
			case 417:
				return {{995.9813232f, 1214.2034912f}};
			case 418:
				return {{995.9813232f, 1214.2034912f}};
			case 419:
				return {{995.9813232f, 1214.2034912f}};
			case 420:
				return {{995.9813232f, 1214.2028809f}};
			case 421:
				return {{995.9813232f, 1214.2028809f}};
			case 422:
				return {{995.9813232f, 1214.2023926f}};
			case 423:
				return {{995.9813232f, 1214.2023926f}};
			case 424:
				return {{995.9813232f, 1214.2023926f}};
			case 425:
				return {{995.9813232f, 1214.2020264f}};
			case 426:
				return {{995.9813232f, 1214.2020264f}};
			case 427:
				return {{995.9813232f, 1214.2020264f}};
			case 428:
				return {{995.9813232f, 1214.2017822f}};
			case 429:
				return {{995.9813232f, 1214.2017822f}};
			case 430:
				return {{995.9813232f, 1214.2017822f}};
			case 431:
				return {{995.9813232f, 1214.2020264f}};
			case 432:
				return {{995.9813232f, 1214.2020264f}};
			case 433:
				return {{995.9813232f, 1214.2020264f}};
			case 434:
				return {{995.9813232f, 1214.2014160f}};
			case 435:
				return {{995.9813232f, 1214.2014160f}};
			case 436:
				return {{995.9813232f, 1214.2014160f}};
			case 437:
				return {{995.9813232f, 1214.2012939f}};
			case 438:
				return {{995.9813232f, 1214.2012939f}};
			case 439:
				return {{995.9813232f, 1214.2012939f}};
			case 440:
				return {{995.9813232f, 1214.2012939f}};
			case 441:
				return {{995.9813232f, 1214.2012939f}};
			case 442:
				return {{995.9813232f, 1214.2012939f}};
			case 443:
				return {{995.9813232f, 1214.2012939f}};
			case 444:
				return {{995.9813232f, 1214.2012939f}};
			case 445:
				return {{995.9813232f, 1214.2012939f}};
			case 446:
				return {{995.9813232f, 1214.2012939f}};
			case 447:
				return {{995.9813232f, 1214.2012939f}};
			case 448:
				return {{995.9813232f, 1214.2012939f}};
			case 449:
				return {{995.9813232f, 1214.2012939f}};
			case 450:
				return {{995.9813232f, 1214.2012939f}};
			case 451:
				return {{995.9813232f, 1214.2012939f}};
			case 452:
				return {{995.9813232f, 1214.2012939f}};
			case 453:
				return {{995.9813232f, 1214.2012939f}};
			case 454:
				return {{995.9813232f, 1214.2012939f}};
			case 455:
				return {{995.9813232f, 1214.2012939f}};
			case 456:
				return {{995.9813232f, 1214.2012939f}};
			case 457:
				return {{995.9813232f, 1214.2012939f}};
			case 458:
				return {{995.9813232f, 1214.2012939f}};
			case 459:
				return {{995.9813232f, 1214.2012939f}};
			case 460:
				return {{995.9813232f, 1214.2012939f}};
			case 461:
				return {{995.9813232f, 1214.2012939f}};
			case 462:
				return {{995.9813232f, 1214.2012939f}};
			case 463:
				return {{995.9813232f, 1214.2012939f}};
			case 464:
				return {{995.9813232f, 1214.2012939f}};
			case 465:
				return {{995.9813232f, 1214.2012939f}};
			case 466:
				return {{995.9813232f, 1214.2012939f}};
			case 467:
				return {{995.9813232f, 1214.2012939f}};
			case 468:
				return {{995.9813232f, 1214.2012939f}};
			case 469:
				return {{995.9813232f, 1214.2012939f}};
			case 470:
				return {{995.9813232f, 1214.2012939f}};
			case 471:
				return {{995.9813232f, 1214.2012939f}};
			case 472:
				return {{995.9813232f, 1214.2012939f}};
			case 473:
				return {{995.9813232f, 1214.2012939f}};
			case 474:
				return {{995.9813232f, 1214.2012939f}};
			case 475:
				return {{995.9813232f, 1214.2012939f}};
			case 476:
				return {{995.9813232f, 1214.2012939f}};
			case 477:
				return {{995.9813232f, 1214.2012939f}};
			case 478:
				return {{995.9813232f, 1214.2012939f}};
			case 479:
				return {{995.9813232f, 1214.2012939f}};
			case 480:
				return {{995.9813232f, 1214.2012939f}};
			case 481:
				return {{995.9813232f, 1214.2012939f}};
			case 482:
				return {{995.9813232f, 1214.2012939f}};
			case 483:
				return {{995.9813232f, 1214.2012939f}};
			case 484:
				return {{995.9813232f, 1214.2012939f}};
			case 485:
				return {{995.9813232f, 1214.2012939f}};
			case 486:
				return {{995.9813232f, 1214.2012939f}};
			case 487:
				return {{995.9813232f, 1214.2012939f}};
			case 488:
				return {{995.9813232f, 1214.2012939f}};
			case 489:
				return {{995.9813232f, 1214.2012939f}};
			case 490:
				return {{995.9813232f, 1214.2012939f}};
			case 491:
				return {{995.9813232f, 1214.2012939f}};
			case 492:
				return {{995.9813232f, 1214.2012939f}};
			case 493:
				return {{995.9813232f, 1214.2012939f}};
			case 494:
				return {{995.9813232f, 1214.2012939f}};
			case 495:
				return {{995.9813232f, 1214.2012939f}};
			case 496:
				return {{995.9813232f, 1214.2012939f}};
			case 497:
				return {{995.9813232f, 1214.2012939f}};
			case 498:
				return {{995.9813232f, 1214.2012939f}};
			case 499:
				return {{995.9813232f, 1214.2012939f}};
			case 500:
				return {{995.9813232f, 1214.2012939f}};
			case 501:
				return {{995.9813232f, 1214.2012939f}};
			case 502:
				return {{995.9813232f, 1214.2012939f}};
			case 503:
				return {{995.9813232f, 1214.1995850f}};
			case 504:
				return {{995.9813232f, 1214.0881348f}};
			case 505:
				return {{995.9813232f, 1214.0817871f}};
			case 506:
				return {{995.9813843f, 1214.0744629f}};
			case 507:
				return {{995.9813232f, 1214.0665283f}};
			case 508:
				return {{995.9813232f, 1213.8608398f}};
			case 509:
				return {{995.9813843f, 1213.8430176f}};
			case 510:
				return {{995.9813232f, 1213.4804688f}};
			case 511:
				return {{995.9813232f, 1213.4530029f}};
			case 512:
				return {{995.9813232f, 1212.9638672f}};
			case 513:
				return {{995.9813843f, 1212.9270020f}};
			case 514:
				return {{995.9813232f, 1212.8885498f}};
			case 515:
				return {{995.9813232f, 1212.8498535f}};
			case 516:
				return {{995.9813843f, 1212.1479492f}};
			case 517:
				return {{995.9813232f, 1212.0993652f}};
			case 518:
				return {{995.9813232f, 1211.2416992f}};
			case 519:
				return {{995.9813232f, 1211.1835938f}};
			case 520:
				return {{995.9813843f, 1211.1248779f}};
			case 521:
				return {{995.9813232f, 1210.1789551f}};
			case 522:
				return {{995.9813843f, 1210.1119385f}};
			case 523:
				return {{995.9813232f, 1210.0449219f}};
			case 524:
				return {{995.9813843f, 1208.8964844f}};
			case 525:
				return {{995.9813232f, 1208.8210449f}};
			case 526:
				return {{995.9813232f, 1208.7451172f}};
			case 527:
				return {{995.9813232f, 1207.5528564f}};
			case 528:
				return {{995.9813843f, 1207.4697266f}};
			case 529:
				return {{995.9813232f, 1207.3865967f}};
			case 530:
				return {{995.9813843f, 1206.0036621f}};
			case 531:
				return {{995.9813843f, 1205.9144287f}};
			case 532:
				return {{995.9813232f, 1205.8247070f}};
			case 533:
				return {{995.9813232f, 1204.3433838f}};
			case 534:
				return {{995.9813232f, 1204.2484131f}};
			case 535:
				return {{995.9813232f, 1204.1530762f}};
			case 536:
				return {{995.9813232f, 1201.0758057f}};
			case 537:
				return {{995.9813843f, 1200.9742432f}};
			case 538:
				return {{995.9813843f, 1199.3139648f}};
			case 539:
				return {{995.9813232f, 1199.2084961f}};
			case 540:
				return {{995.9813232f, 1199.1031494f}};
			case 541:
				return {{995.9813232f, 1197.4896240f}};
			case 542:
				return {{995.9813232f, 1197.3807373f}};
			case 543:
				return {{995.9813232f, 1197.2720947f}};
			case 544:
				return {{995.9813232f, 1195.5012207f}};
			case 545:
				return {{995.9813843f, 1195.3895264f}};
			case 546:
				return {{995.9813232f, 1195.2780762f}};
			case 547:
				return {{995.9813843f, 1193.5805664f}};
			case 548:
				return {{995.9813232f, 1193.4665527f}};
			case 549:
				return {{995.9813232f, 1193.3524170f}};
			case 550:
				return {{995.9813232f, 1191.5101318f}};
			case 551:
				return {{995.9813232f, 1191.3944092f}};
			case 552:
				return {{995.9813232f, 1189.5289307f}};
			case 553:
				return {{995.9813232f, 1189.4118652f}};
			case 554:
				return {{995.9813232f, 1187.6461182f}};
			case 555:
				return {{995.9813843f, 1187.5280762f}};
			case 556:
				return {{995.9813232f, 1187.4099121f}};
			case 557:
				return {{995.9813232f, 1185.5104980f}};
			case 558:
				return {{995.9813232f, 1185.3916016f}};
			case 559:
				return {{995.9813232f, 1185.2725830f}};
			case 560:
				return {{995.9813232f, 1183.3621826f}};
			case 561:
				return {{995.9813232f, 1183.2421875f}};
			case 562:
				return {{995.9813232f, 1183.1225586f}};
			case 563:
				return {{995.9813843f, 1181.3232422f}};
			case 564:
				return {{995.9813843f, 1181.2032471f}};
			case 565:
				return {{995.9813232f, 1181.0831299f}};
			case 566:
				return {{995.9813232f, 1179.1574707f}};
			case 567:
				return {{995.9813843f, 1179.0372314f}};
			case 568:
				return {{995.9813232f, 1177.2293701f}};
			case 569:
				return {{995.9813843f, 1175.3000488f}};
			case 570:
				return {{995.9813232f, 1173.3697510f}};
			case 571:
				return {{995.9813232f, 1173.2492676f}};
			case 572:
				return {{995.9813232f, 1173.1287842f}};
			case 573:
				return {{995.9813232f, 1171.3181152f}};
			case 574:
				return {{995.9813232f, 1171.1975098f}};
			case 575:
				return {{995.9813232f, 1171.0769043f}};
			case 576:
				return {{995.9813843f, 1169.1442871f}};
			case 577:
				return {{995.9813232f, 1169.0236816f}};
			case 578:
				return {{995.9813232f, 1168.9035645f}};
			case 579:
				return {{995.9813843f, 1166.9708252f}};
			case 580:
				return {{995.9813232f, 1166.8502197f}};
			case 581:
				return {{995.9813232f, 1166.7301025f}};
			case 582:
				return {{995.9813843f, 1164.9185791f}};
			case 583:
				return {{995.9813232f, 1164.7979736f}};
			case 584:
				return {{995.9813232f, 1162.8670654f}};
			case 585:
				return {{995.9813232f, 1162.7465820f}};
			case 586:
				return {{995.9813232f, 1162.6260986f}};
			case 587:
				return {{995.9813843f, 1160.8166504f}};
			case 588:
				return {{995.9813232f, 1160.6961670f}};
			case 589:
				return {{995.9813232f, 1160.5756836f}};
			case 590:
				return {{995.9813232f, 1158.6459961f}};
			case 591:
				return {{995.9813843f, 1158.5256348f}};
			case 592:
				return {{995.9813843f, 1158.4052734f}};
			case 593:
				return {{995.9813843f, 1156.4774170f}};
			case 594:
				return {{995.9813232f, 1156.3571777f}};
			case 595:
				return {{995.9813232f, 1156.2363281f}};
			case 596:
				return {{995.9813232f, 1154.4294434f}};
			case 597:
				return {{995.9813232f, 1154.3092041f}};
			case 598:
				return {{995.9813232f, 1154.1888428f}};
			case 599:
				return {{995.9813843f, 1152.2619629f}};
			case 600:
				return {{995.9813843f, 1152.1417236f}};
			case 601:
				return {{995.9813232f, 1148.4290771f}};
			case 602:
				return {{995.9813843f, 1148.3112793f}};
			case 603:
				return {{995.9813232f, 1146.4819336f}};
			case 604:
				return {{995.9813843f, 1146.3712158f}};
			case 605:
				return {{995.9813232f, 1144.7739258f}};
			case 606:
				return {{995.9813232f, 1144.6711426f}};
			case 607:
				return {{995.9813232f, 1144.5693359f}};
			case 608:
				return {{995.9813232f, 1143.0190430f}};
			case 609:
				return {{995.9813232f, 1142.9270020f}};
			case 610:
				return {{995.9813232f, 1141.5346680f}};
			case 611:
				return {{995.9813232f, 1141.4526367f}};
			case 612:
				return {{995.9813232f, 1140.2921143f}};
			case 613:
				return {{995.9813232f, 1140.2193604f}};
			case 614:
				return {{995.9813232f, 1139.1315918f}};
			case 615:
				return {{995.9813232f, 1139.0682373f}};
			case 616:
				return {{995.9813232f, 1139.0054932f}};
			case 617:
				return {{995.9813843f, 1138.0722656f}};
			case 618:
				return {{995.9813232f, 1138.0183105f}};
			case 619:
				return {{995.9813232f, 1137.9648438f}};
			case 620:
				return {{995.9813232f, 1137.2202148f}};
			case 621:
				return {{995.9813232f, 1137.1737061f}};
			case 622:
				return {{995.9813232f, 1136.4973145f}};
			case 623:
				return {{995.9813843f, 1136.4591064f}};
			case 624:
				return {{995.9813232f, 1136.4202881f}};
			case 625:
				return {{995.9813843f, 1135.8920898f}};
			case 626:
				return {{995.9813232f, 1135.8597412f}};
			case 627:
				return {{995.9813843f, 1135.8272705f}};
			case 628:
				return {{995.9813843f, 1134.9759521f}};
			case 629:
				return {{995.9813232f, 1134.9542236f}};
			case 630:
				return {{995.9813232f, 1134.6588135f}};
			case 631:
				return {{995.9813232f, 1134.6414795f}};
			case 632:
				return {{995.9813232f, 1134.6234131f}};
			case 633:
				return {{995.9813232f, 1134.3723145f}};
			case 634:
				return {{995.9813232f, 1134.3582764f}};
			case 635:
				return {{995.9813232f, 1134.1583252f}};
			case 636:
				return {{995.9813232f, 1134.1473389f}};
			case 637:
				return {{995.9813232f, 1134.1364746f}};
			case 638:
				return {{995.9813843f, 1133.9914551f}};
			case 639:
				return {{995.9813232f, 1133.9829102f}};
			case 640:
				return {{995.9813232f, 1133.9749756f}};
			case 641:
				return {{995.9813232f, 1133.8564453f}};
			case 642:
				return {{995.9813232f, 1133.8500977f}};
			case 643:
				return {{995.9813232f, 1133.7661133f}};
			case 644:
				return {{995.9813232f, 1133.7612305f}};
			case 645:
				return {{995.9813843f, 1133.7564697f}};
			case 646:
				return {{995.9813232f, 1133.6917725f}};
			case 647:
				return {{995.9813232f, 1133.6878662f}};
			case 648:
				return {{995.9813232f, 1133.6844482f}};
			case 649:
				return {{995.9813232f, 1133.6390381f}};
			case 650:
				return {{995.9813843f, 1133.6367188f}};
			case 651:
				return {{995.9813232f, 1133.6081543f}};
			case 652:
				return {{995.9813232f, 1133.6060791f}};
			case 653:
				return {{995.9813232f, 1133.6044922f}};
			case 654:
				return {{995.9813232f, 1133.5852051f}};
			case 655:
				return {{995.9813232f, 1133.5843506f}};
			case 656:
				return {{995.9813232f, 1133.5738525f}};
			case 657:
				return {{995.9813232f, 1133.5733643f}};
			case 658:
				return {{995.9813232f, 1133.5694580f}};
			case 659:
				return {{995.9813232f, 1133.5697021f}};
			case 660:
				return {{995.9813232f, 1133.5729980f}};
			case 661:
				return {{995.9813232f, 1133.5732422f}};
			case 662:
				return {{995.9813232f, 1133.5736084f}};
			case 663:
				return {{995.9813232f, 1133.5800781f}};
			case 664:
				return {{995.9813232f, 1133.5800781f}};
			case 665:
				return {{995.9813232f, 1133.5880127f}};
			case 666:
				return {{995.9813843f, 1133.5880127f}};
			case 667:
				return {{995.9813232f, 1133.5885010f}};
			case 668:
				return {{995.9813232f, 1133.5961914f}};
			case 669:
				return {{995.9813843f, 1133.5971680f}};
			case 670:
				return {{995.9813843f, 1133.5971680f}};
			case 671:
				return {{995.9813232f, 1133.6055908f}};
			case 672:
				return {{995.9813232f, 1133.6060791f}};
			case 673:
				return {{995.9813232f, 1133.6145020f}};
			case 674:
				return {{995.9813843f, 1133.6154785f}};
			case 675:
				return {{995.9813843f, 1133.6154785f}};
			case 676:
				return {{995.9813232f, 1133.6234131f}};
			case 677:
				return {{995.9813232f, 1133.6234131f}};
			case 678:
				return {{995.9813232f, 1133.6239014f}};
			case 679:
				return {{995.9813232f, 1133.6313477f}};
			case 680:
				return {{995.9813232f, 1133.6318359f}};
			case 681:
				return {{995.9813232f, 1133.6383057f}};
			case 682:
				return {{995.9813232f, 1133.6386719f}};
			case 683:
				return {{995.9813232f, 1133.6390381f}};
			case 684:
				return {{995.9813232f, 1133.6451416f}};
			case 685:
				return {{995.9813232f, 1133.6459961f}};
			case 686:
				return {{995.9813232f, 1133.6514893f}};
			case 687:
				return {{995.9813232f, 1133.6513672f}};
			case 688:
				return {{995.9813232f, 1133.6563721f}};
			case 689:
				return {{995.9813232f, 1133.6606445f}};
			case 690:
				return {{995.9813232f, 1133.6643066f}};
			case 691:
				return {{995.9813232f, 1133.6640625f}};
			case 692:
				return {{995.9813232f, 1133.6643066f}};
			case 693:
				return {{995.9813232f, 1133.6672363f}};
			case 694:
				return {{995.9813232f, 1133.6673584f}};
			case 695:
				return {{995.9813232f, 1133.6704102f}};
			case 696:
				return {{995.9813232f, 1133.6700439f}};
			case 697:
				return {{995.9813843f, 1133.6701660f}};
			case 698:
				return {{995.9813232f, 1133.6722412f}};
			case 699:
				return {{995.9813843f, 1133.6728516f}};
			case 700:
				return {{995.9813232f, 1133.6746826f}};
			case 701:
				return {{995.9813232f, 1133.6743164f}};
			case 702:
				return {{995.9813232f, 1133.6757812f}};
			case 703:
				return {{995.9813232f, 1133.6759033f}};
			case 704:
				return {{995.9813232f, 1133.6765137f}};
			case 705:
				return {{995.9813232f, 1133.6771240f}};
			case 706:
				return {{995.9813232f, 1133.6777344f}};
			case 707:
				return {{995.9813232f, 1133.6773682f}};
			case 708:
				return {{995.9813232f, 1133.6782227f}};
			case 709:
				return {{995.9813232f, 1133.6782227f}};
			case 710:
				return {{995.9813232f, 1133.6789551f}};
			case 711:
				return {{995.9813232f, 1133.6789551f}};
			case 712:
				return {{995.9813232f, 1133.6789551f}};
			case 713:
				return {{995.9813232f, 1133.6794434f}};
			case 714:
				return {{995.9813232f, 1133.6794434f}};
			case 715:
				return {{995.9813232f, 1133.6794434f}};
			case 716:
				return {{995.9813232f, 1133.6798096f}};
			case 717:
				return {{995.9813232f, 1133.6798096f}};
			case 718:
				return {{995.9813232f, 1133.6800537f}};
			case 719:
				return {{995.9813232f, 1133.6807861f}};
			case 720:
				return {{995.9813232f, 1133.6804199f}};
			case 721:
				return {{995.9813232f, 1133.6804199f}};
			case 722:
				return {{995.9813232f, 1133.6804199f}};
			case 723:
				return {{995.9813843f, 1133.6805420f}};
			case 724:
				return {{995.9813843f, 1133.6805420f}};
			case 725:
				return {{995.9813232f, 1133.6806641f}};
			case 726:
				return {{995.9813232f, 1133.6806641f}};
			case 727:
				return {{995.9813232f, 1133.6806641f}};
			case 728:
				return {{995.9813232f, 1133.6806641f}};
			case 729:
				return {{995.9813232f, 1133.6806641f}};
			case 730:
				return {{995.9813232f, 1133.6806641f}};
			case 731:
				return {{995.9813232f, 1133.6806641f}};
			case 732:
				return {{995.9813232f, 1133.6806641f}};
			case 733:
				return {{995.9813232f, 1133.6806641f}};
			case 734:
				return {{995.9813232f, 1133.6806641f}};
			case 735:
				return {{995.9813232f, 1133.6806641f}};
			case 736:
				return {{995.9813232f, 1133.6806641f}};
			case 737:
				return {{995.9813232f, 1133.6806641f}};
			case 738:
				return {{995.9813232f, 1133.6806641f}};
			case 739:
				return {{995.9813232f, 1133.6806641f}};
			case 740:
				return {{995.9813232f, 1133.6806641f}};
			case 741:
				return {{995.9813232f, 1133.6806641f}};
			case 742:
				return {{995.9813232f, 1133.6806641f}};
			case 743:
				return {{995.9813232f, 1133.6806641f}};
			case 744:
				return {{995.9813232f, 1133.6806641f}};
			case 745:
				return {{995.9813232f, 1133.6806641f}};
			case 746:
				return {{995.9813232f, 1133.6806641f}};
			case 747:
				return {{995.9813232f, 1133.6806641f}};
			case 748:
				return {{995.9813843f, 1133.6805420f}};
			case 749:
				return {{995.9813843f, 1133.6805420f}};
			case 750:
				return {{995.9813843f, 1133.6805420f}};
			case 751:
				return {{995.9813843f, 1133.6805420f}};
			case 752:
				return {{995.9813843f, 1133.6805420f}};
			case 753:
				return {{995.9813843f, 1133.6805420f}};
			case 754:
				return {{995.9813843f, 1133.6805420f}};
			case 755:
				return {{995.9813843f, 1133.6805420f}};
			case 756:
				return {{995.9813843f, 1133.6805420f}};
			case 757:
				return {{995.9813843f, 1133.6805420f}};
			case 758:
				return {{995.9813843f, 1133.6805420f}};
			case 759:
				return {{995.9813843f, 1133.6805420f}};
			case 760:
				return {{995.9813843f, 1133.6805420f}};
			case 761:
				return {{995.9813843f, 1133.6805420f}};
			case 762:
				return {{995.9813843f, 1133.6805420f}};
			case 763:
				return {{995.9813843f, 1133.6805420f}};
			case 764:
				return {{995.9813843f, 1133.6805420f}};
			case 765:
				return {{995.9813843f, 1133.6805420f}};
			case 766:
				return {{995.9813843f, 1133.6805420f}};
			case 767:
				return {{995.9813843f, 1133.6805420f}};
			case 768:
				return {{995.9813843f, 1133.6805420f}};
			case 769:
				return {{995.9813843f, 1133.6805420f}};
			case 770:
				return {{995.9813843f, 1133.6805420f}};
			case 771:
				return {{995.9813843f, 1133.6805420f}};
			case 772:
				return {{995.9813843f, 1133.6805420f}};
			case 773:
				return {{995.9813843f, 1133.6805420f}};
			case 774:
				return {{995.9813843f, 1133.6805420f}};
			case 775:
				return {{995.9813843f, 1133.6805420f}};
			case 776:
				return {{995.9813843f, 1133.6805420f}};
			case 777:
				return {{995.9813843f, 1133.6805420f}};
			case 778:
				return {{995.9813843f, 1133.6805420f}};
			case 779:
				return {{995.9813843f, 1133.6805420f}};
			case 780:
				return {{995.9813843f, 1133.6805420f}};
			case 781:
				return {{995.9813843f, 1133.6805420f}};
			case 782:
				return {{995.9813843f, 1133.6805420f}};
			case 783:
				return {{995.9813843f, 1133.6805420f}};
			case 784:
				return {{995.9813843f, 1133.6805420f}};
			case 785:
				return {{995.9813843f, 1133.6805420f}};
			case 786:
				return {{995.9813843f, 1133.6805420f}};
			case 787:
				return {{995.9813843f, 1133.6805420f}};
			case 788:
				return {{995.9813843f, 1133.6805420f}};
			case 789:
				return {{995.9813843f, 1133.6805420f}};
			case 790:
				return {{995.9813843f, 1133.6805420f}};
			case 791:
				return {{995.9813843f, 1133.6805420f}};
			case 792:
				return {{995.9813843f, 1133.6805420f}};
			case 793:
				return {{995.9813843f, 1133.6805420f}};
			case 794:
				return {{995.9813843f, 1133.6805420f}};
			case 795:
				return {{995.9813843f, 1133.6805420f}};
			case 796:
				return {{995.9813843f, 1133.6805420f}};
			case 797:
				return {{995.9813843f, 1133.6805420f}};
			case 798:
				return {{995.9813843f, 1133.6805420f}};
			case 799:
				return {{995.9813843f, 1133.6805420f}};
			case 800:
				return {{995.9813843f, 1133.6805420f}};
			case 801:
				return {{995.9813843f, 1133.6805420f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		assert(false); // Shouldn't be any unaccounted raycasts
		return std::nullopt;
	}
};
// NOLINTEND(bugprone-branch-clone, google-readability-function-size)
