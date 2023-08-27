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

class MoveRightLeftMockAction final: public MockAction
{
public:
	static constexpr uint32_t k_Start1 = k_StabilizeFrames + 1;
	static constexpr uint32_t k_End1 = k_StabilizeFrames + k_InteractionFrames - 1;
	static constexpr uint32_t k_Start2 = k_StabilizeFrames + k_InteractionFrames + k_StabilizeFrames + 1;
	static constexpr uint32_t k_End2 = k_StabilizeFrames + k_InteractionFrames + k_StabilizeFrames + k_InteractionFrames - 1;

	~MoveRightLeftMockAction() final = default;

	[[nodiscard]] bool GetBindable(openblack::input::BindableActionMap action) const final
	{
		using openblack::input::BindableActionMap;
		if (frameNumber >= k_Start1 && frameNumber <= k_End1)
		{
			return (static_cast<uint32_t>(action) & static_cast<uint32_t>(BindableActionMap::MOVE_RIGHT)) != 0;
		}

		if (frameNumber >= k_Start2 && frameNumber <= k_End2)
		{
			return (static_cast<uint32_t>(action) & static_cast<uint32_t>(BindableActionMap::MOVE_LEFT)) != 0;
		}

		return false;
	}
};

class MoveRightLeftMockDynamicsSystem: public MockDynamicsSystem
{
public:
	MoveRightLeftMockDynamicsSystem() = default;

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
				return {{1000.0000000f, -368.5463867f}};
			case 204:
				return {{999.9928589f, -368.5367432f}};
			case 205:
				return {{999.9920044f, -368.5361328f}};
			case 206:
				return {{999.9268799f, -368.5203857f}};
			case 207:
				return {{999.8660278f, -368.5139160f}};
			case 208:
				return {{999.8610229f, -368.5135498f}};
			case 209:
				return {{999.8558960f, -368.5131836f}};
			case 210:
				return {{999.7562866f, -368.5073242f}};
			case 211:
				return {{999.7490845f, -368.5069580f}};
			case 212:
				return {{999.6163330f, -368.5019531f}};
			case 213:
				return {{999.6070557f, -368.5017090f}};
			case 214:
				return {{999.4534912f, -368.4976807f}};
			case 215:
				return {{999.4423828f, -368.4974365f}};
			case 216:
				return {{999.2492065f, -368.4938965f}};
			case 217:
				return {{999.2362671f, -368.4936523f}};
			case 218:
				return {{999.0153198f, -368.4907227f}};
			case 219:
				return {{999.0007324f, -368.4906006f}};
			case 220:
				return {{998.7706909f, -368.4882812f}};
			case 221:
				return {{998.7546997f, -368.4882812f}};
			case 222:
				return {{998.4872437f, -368.4862061f}};
			case 223:
				return {{998.4699097f, -368.4860840f}};
			case 224:
				return {{998.2010498f, -368.4846191f}};
			case 225:
				return {{998.1826172f, -368.4843750f}};
			case 226:
				return {{997.8787842f, -368.4831543f}};
			case 227:
				return {{997.8593140f, -368.4830322f}};
			case 228:
				return {{997.5404663f, -368.4820557f}};
			case 229:
				return {{997.5201416f, -368.4820557f}};
			case 230:
				return {{996.8711548f, -368.4808350f}};
			case 231:
				return {{996.5261841f, -368.4803467f}};
			case 232:
				return {{996.5043335f, -368.4803467f}};
			case 233:
				return {{996.4824219f, -368.4803467f}};
			case 234:
				return {{996.1496582f, -368.4801025f}};
			case 235:
				return {{995.7877197f, -368.4798584f}};
			case 236:
				return {{995.7649536f, -368.4798584f}};
			case 237:
				return {{995.4199219f, -368.4797363f}};
			case 238:
				return {{995.3967896f, -368.4797363f}};
			case 239:
				return {{995.0238037f, -368.4796143f}};
			case 240:
				return {{995.0003662f, -368.4796143f}};
			case 241:
				return {{994.6235352f, -368.4796143f}};
			case 242:
				return {{994.5999146f, -368.4796143f}};
			case 243:
				return {{994.2438965f, -368.4796143f}};
			case 244:
				return {{994.2200928f, -368.4796143f}};
			case 245:
				return {{993.8380127f, -368.4796143f}};
			case 246:
				return {{993.8140869f, -368.4796143f}};
			case 247:
				return {{993.4303589f, -368.4796143f}};
			case 248:
				return {{993.4063721f, -368.4796143f}};
			case 249:
				return {{993.0455322f, -368.4796143f}};
			case 250:
				return {{993.0214844f, -368.4796143f}};
			case 251:
				return {{992.6356812f, -368.4797363f}};
			case 252:
				return {{992.6115723f, -368.4797363f}};
			case 253:
				return {{992.5874634f, -368.4797363f}};
			case 254:
				return {{991.8416748f, -368.4798584f}};
			case 255:
				return {{991.4602661f, -368.4799805f}};
			case 256:
				return {{991.4364014f, -368.4799805f}};
			case 257:
				return {{991.0772095f, -368.4801025f}};
			case 258:
				return {{991.0532227f, -368.4801025f}};
			case 259:
				return {{991.0292358f, -368.4801025f}};
			case 260:
				return {{990.6444702f, -368.4801025f}};
			case 261:
				return {{990.6204224f, -368.4801025f}};
			case 262:
				return {{990.2349243f, -368.4801025f}};
			case 263:
				return {{990.2108765f, -368.4801025f}};
			case 264:
				return {{989.8489380f, -368.4801025f}};
			case 265:
				return {{989.4626465f, -368.4801025f}};
			case 266:
				return {{989.4385376f, -368.4801025f}};
			case 267:
				return {{989.0762329f, -368.4801025f}};
			case 268:
				return {{989.0521240f, -368.4801025f}};
			case 269:
				return {{988.6655273f, -368.4801025f}};
			case 270:
				return {{988.6414185f, -368.4801025f}};
			case 271:
				return {{988.2549438f, -368.4801025f}};
			case 272:
				return {{988.2308350f, -368.4801025f}};
			case 273:
				return {{987.8686523f, -368.4801025f}};
			case 274:
				return {{987.8445435f, -368.4801025f}};
			case 275:
				return {{987.8204346f, -368.4801025f}};
			case 276:
				return {{987.4340210f, -368.4801025f}};
			case 277:
				return {{987.4099121f, -368.4801025f}};
			case 278:
				return {{987.3858032f, -368.4801025f}};
			case 279:
				return {{986.9995728f, -368.4801025f}};
			case 280:
				return {{986.6378784f, -368.4801025f}};
			case 281:
				return {{986.2523804f, -368.4801025f}};
			case 282:
				return {{986.2283325f, -368.4801025f}};
			case 283:
				return {{986.2042847f, -368.4801025f}};
			case 284:
				return {{985.8428955f, -368.4801025f}};
			case 285:
				return {{985.8188477f, -368.4801025f}};
			case 286:
				return {{985.7947998f, -368.4801025f}};
			case 287:
				return {{985.4093018f, -368.4801025f}};
			case 288:
				return {{985.3852539f, -368.4801025f}};
			case 289:
				return {{985.3612061f, -368.4801025f}};
			case 290:
				return {{984.9758911f, -368.4801025f}};
			case 291:
				return {{984.9518433f, -368.4801025f}};
			case 292:
				return {{984.9277954f, -368.4801025f}};
			case 293:
				return {{984.5667114f, -368.4801025f}};
			case 294:
				return {{984.5426636f, -368.4801025f}};
			case 295:
				return {{984.1576538f, -368.4801025f}};
			case 296:
				return {{984.1336670f, -368.4801025f}};
			case 297:
				return {{984.1096191f, -368.4801025f}};
			case 298:
				return {{983.7247925f, -368.4801025f}};
			case 299:
				return {{983.7008057f, -368.4801025f}};
			case 300:
				return {{983.6768188f, -368.4801025f}};
			case 301:
				return {{983.3161621f, -368.4801025f}};
			case 302:
				return {{983.2921753f, -368.4801025f}};
			case 303:
				return {{982.9143677f, -368.4801025f}};
			case 304:
				return {{982.8911743f, -368.4801025f}};
			case 305:
				return {{982.8680420f, -368.4801025f}};
			case 306:
				return {{982.5310059f, -368.4801025f}};
			case 307:
				return {{982.5092163f, -368.4801025f}};
			case 308:
				return {{982.1738281f, -368.4801025f}};
			case 309:
				return {{981.8654175f, -368.4801025f}};
			case 310:
				return {{981.8471069f, -368.4801025f}};
			case 311:
				return {{981.5859375f, -368.4801025f}};
			case 312:
				return {{981.5694580f, -368.4801025f}};
			case 313:
				return {{981.5531006f, -368.4801025f}};
			case 314:
				return {{981.3067017f, -368.4801025f}};
			case 315:
				return {{981.2922363f, -368.4801025f}};
			case 316:
				return {{981.2778931f, -368.4801025f}};
			case 317:
				return {{981.0635986f, -368.4801025f}};
			case 318:
				return {{981.0511475f, -368.4801025f}};
			case 319:
				return {{980.8764038f, -368.4801025f}};
			case 320:
				return {{980.8655396f, -368.4801025f}};
			case 321:
				return {{980.8547974f, -368.4801025f}};
			case 322:
				return {{980.6958008f, -368.4801025f}};
			case 323:
				return {{980.6866455f, -368.4801025f}};
			case 324:
				return {{980.6775513f, -368.4801025f}};
			case 325:
				return {{980.5518188f, -368.4801025f}};
			case 326:
				return {{980.5440674f, -368.4801025f}};
			case 327:
				return {{980.5364380f, -368.4801025f}};
			case 328:
				return {{980.4244995f, -368.4801025f}};
			case 329:
				return {{980.4181519f, -368.4801025f}};
			case 330:
				return {{980.4118652f, -368.4801025f}};
			case 331:
				return {{980.3199463f, -368.4801025f}};
			case 332:
				return {{980.3147583f, -368.4801025f}};
			case 333:
				return {{980.3096313f, -368.4801025f}};
			case 334:
				return {{980.2393188f, -368.4801025f}};
			case 335:
				return {{980.2350464f, -368.4801025f}};
			case 336:
				return {{980.2308350f, -368.4801025f}};
			case 337:
				return {{980.1702881f, -368.4801025f}};
			case 338:
				return {{980.1668701f, -368.4801025f}};
			case 339:
				return {{980.1635132f, -368.4801025f}};
			case 340:
				return {{980.1156006f, -368.4801025f}};
			case 341:
				return {{980.1129150f, -368.4801025f}};
			case 342:
				return {{980.0770874f, -368.4801025f}};
			case 343:
				return {{980.0466309f, -368.4801025f}};
			case 344:
				return {{980.0241699f, -368.4801025f}};
			case 345:
				return {{980.0228882f, -368.4801025f}};
			case 346:
				return {{980.0216064f, -368.4801025f}};
			case 347:
				return {{980.0036621f, -368.4801025f}};
			case 348:
				return {{980.0026855f, -368.4801025f}};
			case 349:
				return {{979.9895020f, -368.4801025f}};
			case 350:
				return {{979.9888306f, -368.4801025f}};
			case 351:
				return {{979.9881592f, -368.4801025f}};
			case 352:
				return {{979.9794312f, -368.4801025f}};
			case 353:
				return {{979.9789429f, -368.4801025f}};
			case 354:
				return {{979.9725342f, -368.4801025f}};
			case 355:
				return {{979.9722290f, -368.4801025f}};
			case 356:
				return {{979.9719238f, -368.4801025f}};
			case 357:
				return {{979.9679565f, -368.4801025f}};
			case 358:
				return {{979.9677734f, -368.4801025f}};
			case 359:
				return {{979.9656982f, -368.4801025f}};
			case 360:
				return {{979.9655762f, -368.4801025f}};
			case 361:
				return {{979.9654541f, -368.4801025f}};
			case 362:
				return {{979.9645386f, -368.4801025f}};
			case 363:
				return {{979.9645386f, -368.4801025f}};
			case 364:
				return {{979.9645386f, -368.4801025f}};
			case 365:
				return {{979.9645386f, -368.4801025f}};
			case 366:
				return {{979.9646606f, -368.4801025f}};
			case 367:
				return {{979.9647217f, -368.4801025f}};
			case 368:
				return {{979.9655151f, -368.4801025f}};
			case 369:
				return {{979.9655762f, -368.4801025f}};
			case 370:
				return {{979.9667969f, -368.4801025f}};
			case 371:
				return {{979.9668579f, -368.4801025f}};
			case 372:
				return {{979.9669189f, -368.4801025f}};
			case 373:
				return {{979.9683228f, -368.4801025f}};
			case 374:
				return {{979.9699707f, -368.4801025f}};
			case 375:
				return {{979.9716187f, -368.4801025f}};
			case 376:
				return {{979.9717407f, -368.4801025f}};
			case 377:
				return {{979.9718628f, -368.4801025f}};
			case 378:
				return {{979.9733887f, -368.4801025f}};
			case 379:
				return {{979.9735107f, -368.4801025f}};
			case 380:
				return {{979.9736328f, -368.4801025f}};
			case 381:
				return {{979.9752197f, -368.4801025f}};
			case 382:
				return {{979.9753418f, -368.4801025f}};
			case 383:
				return {{979.9754639f, -368.4801025f}};
			case 384:
				return {{979.9768066f, -368.4801025f}};
			case 385:
				return {{979.9768677f, -368.4801025f}};
			case 386:
				return {{979.9769287f, -368.4801025f}};
			case 387:
				return {{979.9782715f, -368.4801025f}};
			case 388:
				return {{979.9783325f, -368.4801025f}};
			case 389:
				return {{979.9795532f, -368.4801025f}};
			case 390:
				return {{979.9796143f, -368.4801025f}};
			case 391:
				return {{979.9796753f, -368.4801025f}};
			case 392:
				return {{979.9806519f, -368.4801025f}};
			case 393:
				return {{979.9807129f, -368.4801025f}};
			case 394:
				return {{979.9807739f, -368.4801025f}};
			case 395:
				return {{979.9816895f, -368.4801025f}};
			case 396:
				return {{979.9817505f, -368.4801025f}};
			case 397:
				return {{979.9818115f, -368.4801025f}};
			case 398:
				return {{979.9826050f, -368.4801025f}};
			case 399:
				return {{979.9826660f, -368.4801025f}};
			case 400:
				return {{979.9832764f, -368.4801025f}};
			case 401:
				return {{979.9833374f, -368.4801025f}};
			case 402:
				return {{979.9833984f, -368.4801025f}};
			case 403:
				return {{979.9839478f, -368.4801025f}};
			case 404:
				return {{979.9840088f, -368.4801025f}};
			case 405:
				return {{979.9840698f, -368.4801025f}};
			case 406:
				return {{979.9844971f, -368.4801025f}};
			case 407:
				return {{979.9849243f, -368.4801025f}};
			case 408:
				return {{979.9852295f, -368.4801025f}};
			case 409:
				return {{979.9852295f, -368.4801025f}};
			case 410:
				return {{979.9852295f, -368.4801025f}};
			case 411:
				return {{979.9854736f, -368.4801025f}};
			case 412:
				return {{979.9854736f, -368.4801025f}};
			case 413:
				return {{979.9854736f, -368.4801025f}};
			case 414:
				return {{979.9857178f, -368.4801025f}};
			case 415:
				return {{979.9857178f, -368.4801025f}};
			case 416:
				return {{979.9857178f, -368.4801025f}};
			case 417:
				return {{979.9859009f, -368.4801025f}};
			case 418:
				return {{979.9859009f, -368.4801025f}};
			case 419:
				return {{979.9859009f, -368.4801025f}};
			case 420:
				return {{979.9860229f, -368.4801025f}};
			case 421:
				return {{979.9860229f, -368.4801025f}};
			case 422:
				return {{979.9861450f, -368.4801025f}};
			case 423:
				return {{979.9861450f, -368.4801025f}};
			case 424:
				return {{979.9861450f, -368.4801025f}};
			case 425:
				return {{979.9862061f, -368.4801025f}};
			case 426:
				return {{979.9862061f, -368.4801025f}};
			case 427:
				return {{979.9862061f, -368.4801025f}};
			case 428:
				return {{979.9862671f, -368.4801025f}};
			case 429:
				return {{979.9862671f, -368.4801025f}};
			case 430:
				return {{979.9862671f, -368.4801025f}};
			case 431:
				return {{979.9863281f, -368.4801025f}};
			case 432:
				return {{979.9863281f, -368.4801025f}};
			case 433:
				return {{979.9863281f, -368.4801025f}};
			case 434:
				return {{979.9863281f, -368.4801025f}};
			case 435:
				return {{979.9863281f, -368.4801025f}};
			case 436:
				return {{979.9863281f, -368.4801025f}};
			case 437:
				return {{979.9863281f, -368.4801025f}};
			case 438:
				return {{979.9863281f, -368.4801025f}};
			case 439:
				return {{979.9863281f, -368.4801025f}};
			case 440:
				return {{979.9863281f, -368.4801025f}};
			case 441:
				return {{979.9863281f, -368.4801025f}};
			case 442:
				return {{979.9863281f, -368.4801025f}};
			case 443:
				return {{979.9863281f, -368.4801025f}};
			case 444:
				return {{979.9863281f, -368.4801025f}};
			case 445:
				return {{979.9863281f, -368.4801025f}};
			case 446:
				return {{979.9863281f, -368.4801025f}};
			case 447:
				return {{979.9863281f, -368.4801025f}};
			case 448:
				return {{979.9863281f, -368.4801025f}};
			case 449:
				return {{979.9863281f, -368.4801025f}};
			case 450:
				return {{979.9863281f, -368.4801025f}};
			case 451:
				return {{979.9863281f, -368.4801025f}};
			case 452:
				return {{979.9863281f, -368.4801025f}};
			case 453:
				return {{979.9863281f, -368.4801025f}};
			case 454:
				return {{979.9863281f, -368.4801025f}};
			case 455:
				return {{979.9863281f, -368.4801025f}};
			case 456:
				return {{979.9863281f, -368.4801025f}};
			case 457:
				return {{979.9863281f, -368.4801025f}};
			case 458:
				return {{979.9863281f, -368.4801025f}};
			case 459:
				return {{979.9863281f, -368.4801025f}};
			case 460:
				return {{979.9863281f, -368.4801025f}};
			case 461:
				return {{979.9863281f, -368.4801025f}};
			case 462:
				return {{979.9863281f, -368.4801025f}};
			case 463:
				return {{979.9863281f, -368.4801025f}};
			case 464:
				return {{979.9863281f, -368.4801025f}};
			case 465:
				return {{979.9863281f, -368.4801025f}};
			case 466:
				return {{979.9863281f, -368.4801025f}};
			case 467:
				return {{979.9863281f, -368.4801025f}};
			case 468:
				return {{979.9863281f, -368.4801025f}};
			case 469:
				return {{979.9863281f, -368.4801025f}};
			case 470:
				return {{979.9863281f, -368.4801025f}};
			case 471:
				return {{979.9863281f, -368.4801025f}};
			case 472:
				return {{979.9863281f, -368.4801025f}};
			case 473:
				return {{979.9863281f, -368.4801025f}};
			case 474:
				return {{979.9863281f, -368.4801025f}};
			case 475:
				return {{979.9863281f, -368.4801025f}};
			case 476:
				return {{979.9863281f, -368.4801025f}};
			case 477:
				return {{979.9863281f, -368.4801025f}};
			case 478:
				return {{979.9863281f, -368.4801025f}};
			case 479:
				return {{979.9863281f, -368.4801025f}};
			case 480:
				return {{979.9863281f, -368.4801025f}};
			case 481:
				return {{979.9863281f, -368.4801025f}};
			case 482:
				return {{979.9863281f, -368.4801025f}};
			case 483:
				return {{979.9863281f, -368.4801025f}};
			case 484:
				return {{979.9863281f, -368.4801025f}};
			case 485:
				return {{979.9863281f, -368.4801025f}};
			case 486:
				return {{979.9863281f, -368.4801025f}};
			case 487:
				return {{979.9863281f, -368.4801025f}};
			case 488:
				return {{979.9863281f, -368.4801025f}};
			case 489:
				return {{979.9863281f, -368.4801025f}};
			case 490:
				return {{979.9863281f, -368.4801025f}};
			case 491:
				return {{979.9863281f, -368.4801025f}};
			case 492:
				return {{979.9863281f, -368.4801025f}};
			case 493:
				return {{979.9863281f, -368.4801025f}};
			case 494:
				return {{979.9863281f, -368.4801025f}};
			case 495:
				return {{979.9863281f, -368.4801025f}};
			case 496:
				return {{979.9863281f, -368.4801025f}};
			case 497:
				return {{979.9863281f, -368.4801025f}};
			case 498:
				return {{979.9863281f, -368.4801025f}};
			case 499:
				return {{979.9863281f, -368.4801025f}};
			case 500:
				return {{979.9863281f, -368.4801025f}};
			case 501:
				return {{979.9863281f, -368.4801025f}};
			case 502:
				return {{979.9863281f, -368.4801025f}};
			case 503:
				return {{979.9866333f, -368.4801025f}};
			case 504:
				return {{980.0089111f, -368.4802246f}};
			case 505:
				return {{980.0102539f, -368.4802246f}};
			case 506:
				return {{980.0117188f, -368.4802246f}};
			case 507:
				return {{980.0133057f, -368.4802246f}};
			case 508:
				return {{980.0544434f, -368.4802246f}};
			case 509:
				return {{980.0579224f, -368.4802246f}};
			case 510:
				return {{980.1304321f, -368.4802246f}};
			case 511:
				return {{980.1359253f, -368.4802246f}};
			case 512:
				return {{980.2337646f, -368.4802246f}};
			case 513:
				return {{980.2412109f, -368.4802246f}};
			case 514:
				return {{980.2487793f, -368.4802246f}};
			case 515:
				return {{980.2565308f, -368.4802246f}};
			case 516:
				return {{980.3969116f, -368.4802246f}};
			case 517:
				return {{980.4066162f, -368.4802246f}};
			case 518:
				return {{980.5781250f, -368.4802246f}};
			case 519:
				return {{980.5897217f, -368.4802246f}};
			case 520:
				return {{980.6014404f, -368.4802246f}};
			case 521:
				return {{980.7905884f, -368.4802246f}};
			case 522:
				return {{980.8039551f, -368.4802246f}};
			case 523:
				return {{980.8174438f, -368.4802246f}};
			case 524:
				return {{981.0469971f, -368.4802246f}};
			case 525:
				return {{981.0620728f, -368.4802246f}};
			case 526:
				return {{981.0772705f, -368.4802246f}};
			case 527:
				return {{981.3157959f, -368.4802246f}};
			case 528:
				return {{981.3323364f, -368.4802246f}};
			case 529:
				return {{981.3489380f, -368.4802246f}};
			case 530:
				return {{981.6254883f, -368.4802246f}};
			case 531:
				return {{981.6433105f, -368.4802246f}};
			case 532:
				return {{981.6612549f, -368.4802246f}};
			case 533:
				return {{981.9574585f, -368.4802246f}};
			case 534:
				return {{981.9764404f, -368.4802246f}};
			case 535:
				return {{981.9954834f, -368.4802246f}};
			case 536:
				return {{982.6108398f, -368.4802246f}};
			case 537:
				return {{982.6311646f, -368.4802246f}};
			case 538:
				return {{982.9631348f, -368.4802246f}};
			case 539:
				return {{982.9842529f, -368.4802246f}};
			case 540:
				return {{983.0054321f, -368.4802246f}};
			case 541:
				return {{983.3280029f, -368.4802246f}};
			case 542:
				return {{983.3497925f, -368.4802246f}};
			case 543:
				return {{983.3716431f, -368.4802246f}};
			case 544:
				return {{983.7256470f, -368.4802246f}};
			case 545:
				return {{983.7479858f, -368.4802246f}};
			case 546:
				return {{983.7703857f, -368.4802246f}};
			case 547:
				return {{984.1097412f, -368.4802246f}};
			case 548:
				return {{984.1325073f, -368.4802246f}};
			case 549:
				return {{984.1553345f, -368.4802246f}};
			case 550:
				return {{984.5237427f, -368.4802246f}};
			case 551:
				return {{984.5468750f, -368.4802246f}};
			case 552:
				return {{984.9199219f, -368.4802246f}};
			case 553:
				return {{984.9432983f, -368.4802246f}};
			case 554:
				return {{985.2963867f, -368.4802246f}};
			case 555:
				return {{985.3200073f, -368.4802246f}};
			case 556:
				return {{985.3436279f, -368.4802246f}};
			case 557:
				return {{985.7234497f, -368.4802246f}};
			case 558:
				return {{985.7472534f, -368.4802246f}};
			case 559:
				return {{985.7710571f, -368.4802246f}};
			case 560:
				return {{986.1531982f, -368.4802246f}};
			case 561:
				return {{986.1771240f, -368.4802246f}};
			case 562:
				return {{986.2010498f, -368.4802246f}};
			case 563:
				return {{986.5608521f, -368.4802246f}};
			case 564:
				return {{986.5848389f, -368.4802246f}};
			case 565:
				return {{986.6088257f, -368.4802246f}};
			case 566:
				return {{986.9938965f, -368.4802246f}};
			case 567:
				return {{987.0179443f, -368.4802246f}};
			case 568:
				return {{987.3794556f, -368.4802246f}};
			case 569:
				return {{987.7653809f, -368.4802246f}};
			case 570:
				return {{988.1513062f, -368.4802246f}};
			case 571:
				return {{988.1754150f, -368.4802246f}};
			case 572:
				return {{988.1995239f, -368.4802246f}};
			case 573:
				return {{988.5615845f, -368.4802246f}};
			case 574:
				return {{988.5856934f, -368.4802246f}};
			case 575:
				return {{988.6098022f, -368.4802246f}};
			case 576:
				return {{988.9962769f, -368.4802246f}};
			case 577:
				return {{989.0203857f, -368.4802246f}};
			case 578:
				return {{989.0444946f, -368.4802246f}};
			case 579:
				return {{989.4308472f, -368.4802246f}};
			case 580:
				return {{989.4549561f, -368.4802246f}};
			case 581:
				return {{989.4790649f, -368.4802246f}};
			case 582:
				return {{989.8411865f, -368.4802246f}};
			case 583:
				return {{989.8652954f, -368.4802246f}};
			case 584:
				return {{990.2514038f, -368.4802246f}};
			case 585:
				return {{990.2755127f, -368.4802246f}};
			case 586:
				return {{990.2996216f, -368.4802246f}};
			case 587:
				return {{990.6614380f, -368.4802246f}};
			case 588:
				return {{990.6855469f, -368.4802246f}};
			case 589:
				return {{990.7096558f, -368.4802246f}};
			case 590:
				return {{991.0955200f, -368.4802246f}};
			case 591:
				return {{991.1195679f, -368.4802246f}};
			case 592:
				return {{991.1436157f, -368.4802246f}};
			case 593:
				return {{991.5292358f, -368.4802246f}};
			case 594:
				return {{991.5532837f, -368.4802246f}};
			case 595:
				return {{991.5773315f, -368.4802246f}};
			case 596:
				return {{991.9386597f, -368.4802246f}};
			case 597:
				return {{991.9627075f, -368.4802246f}};
			case 598:
				return {{991.9867554f, -368.4802246f}};
			case 599:
				return {{992.3720703f, -368.4802246f}};
			case 600:
				return {{992.3961182f, -368.4802246f}};
			case 601:
				return {{993.1384888f, -368.4802246f}};
			case 602:
				return {{993.1620483f, -368.4802246f}};
			case 603:
				return {{993.5278320f, -368.4802246f}};
			case 604:
				return {{993.5499878f, -368.4802246f}};
			case 605:
				return {{993.8695068f, -368.4802246f}};
			case 606:
				return {{993.8899536f, -368.4802246f}};
			case 607:
				return {{993.9102783f, -368.4802246f}};
			case 608:
				return {{994.2202759f, -368.4802246f}};
			case 609:
				return {{994.2387085f, -368.4802246f}};
			case 610:
				return {{994.5170898f, -368.4802246f}};
			case 611:
				return {{994.5335083f, -368.4802246f}};
			case 612:
				return {{994.7655640f, -368.4802246f}};
			case 613:
				return {{994.7800903f, -368.4802246f}};
			case 614:
				return {{994.9976196f, -368.4802246f}};
			case 615:
				return {{995.0102539f, -368.4802246f}};
			case 616:
				return {{995.0228271f, -368.4802246f}};
			case 617:
				return {{995.2094116f, -368.4802246f}};
			case 618:
				return {{995.2202148f, -368.4802246f}};
			case 619:
				return {{995.2308960f, -368.4802246f}};
			case 620:
				return {{995.3798828f, -368.4802246f}};
			case 621:
				return {{995.3890991f, -368.4802246f}};
			case 622:
				return {{995.5243530f, -368.4802246f}};
			case 623:
				return {{995.5321045f, -368.4802246f}};
			case 624:
				return {{995.5397949f, -368.4802246f}};
			case 625:
				return {{995.6455078f, -368.4802246f}};
			case 626:
				return {{995.6519775f, -368.4802246f}};
			case 627:
				return {{995.6583862f, -368.4802246f}};
			case 628:
				return {{995.8286133f, -368.4802246f}};
			case 629:
				return {{995.8329468f, -368.4802246f}};
			case 630:
				return {{995.8920288f, -368.4802246f}};
			case 631:
				return {{995.8955688f, -368.4802246f}};
			case 632:
				return {{995.8990479f, -368.4802246f}};
			case 633:
				return {{995.9493408f, -368.4802246f}};
			case 634:
				return {{995.9521484f, -368.4802246f}};
			case 635:
				return {{995.9920654f, -368.4802246f}};
			case 636:
				return {{995.9942627f, -368.4802246f}};
			case 637:
				return {{995.9964600f, -368.4802246f}};
			case 638:
				return {{996.0254517f, -368.4802246f}};
			case 639:
				return {{996.0271606f, -368.4802246f}};
			case 640:
				return {{996.0288696f, -368.4802246f}};
			case 641:
				return {{996.0524902f, -368.4802246f}};
			case 642:
				return {{996.0537720f, -368.4802246f}};
			case 643:
				return {{996.0705566f, -368.4802246f}};
			case 644:
				return {{996.0715332f, -368.4802246f}};
			case 645:
				return {{996.0725098f, -368.4802246f}};
			case 646:
				return {{996.0855713f, -368.4802246f}};
			case 647:
				return {{996.0862427f, -368.4802246f}};
			case 648:
				return {{996.0869141f, -368.4802246f}};
			case 649:
				return {{996.0959473f, -368.4802246f}};
			case 650:
				return {{996.0964355f, -368.4802246f}};
			case 651:
				return {{996.1022339f, -368.4802246f}};
			case 652:
				return {{996.1025391f, -368.4802246f}};
			case 653:
				return {{996.1028442f, -368.4802246f}};
			case 654:
				return {{996.1066895f, -368.4802246f}};
			case 655:
				return {{996.1068726f, -368.4802246f}};
			case 656:
				return {{996.1089478f, -368.4802246f}};
			case 657:
				return {{996.1090088f, -368.4802246f}};
			case 658:
				return {{996.1098022f, -368.4802246f}};
			case 659:
				return {{996.1097412f, -368.4802246f}};
			case 660:
				return {{996.1090698f, -368.4802246f}};
			case 661:
				return {{996.1090088f, -368.4802246f}};
			case 662:
				return {{996.1089478f, -368.4802246f}};
			case 663:
				return {{996.1077881f, -368.4802246f}};
			case 664:
				return {{996.1077271f, -368.4802246f}};
			case 665:
				return {{996.1062622f, -368.4802246f}};
			case 666:
				return {{996.1061401f, -368.4802246f}};
			case 667:
				return {{996.1060181f, -368.4802246f}};
			case 668:
				return {{996.1044922f, -368.4802246f}};
			case 669:
				return {{996.1043701f, -368.4802246f}};
			case 670:
				return {{996.1042480f, -368.4802246f}};
			case 671:
				return {{996.1025391f, -368.4802246f}};
			case 672:
				return {{996.1024170f, -368.4802246f}};
			case 673:
				return {{996.1007690f, -368.4802246f}};
			case 674:
				return {{996.1006470f, -368.4802246f}};
			case 675:
				return {{996.1005249f, -368.4802246f}};
			case 676:
				return {{996.0989990f, -368.4802246f}};
			case 677:
				return {{996.0988770f, -368.4802246f}};
			case 678:
				return {{996.0987549f, -368.4802246f}};
			case 679:
				return {{996.0972900f, -368.4802246f}};
			case 680:
				return {{996.0972290f, -368.4802246f}};
			case 681:
				return {{996.0959473f, -368.4802246f}};
			case 682:
				return {{996.0958862f, -368.4802246f}};
			case 683:
				return {{996.0958252f, -368.4802246f}};
			case 684:
				return {{996.0946045f, -368.4802246f}};
			case 685:
				return {{996.0945435f, -368.4802246f}};
			case 686:
				return {{996.0934448f, -368.4802246f}};
			case 687:
				return {{996.0933838f, -368.4802246f}};
			case 688:
				return {{996.0924683f, -368.4802246f}};
			case 689:
				return {{996.0916138f, -368.4802246f}};
			case 690:
				return {{996.0908813f, -368.4802246f}};
			case 691:
				return {{996.0908203f, -368.4802246f}};
			case 692:
				return {{996.0907593f, -368.4802246f}};
			case 693:
				return {{996.0902100f, -368.4802246f}};
			case 694:
				return {{996.0901489f, -368.4802246f}};
			case 695:
				return {{996.0896606f, -368.4802246f}};
			case 696:
				return {{996.0896606f, -368.4802246f}};
			case 697:
				return {{996.0896606f, -368.4802246f}};
			case 698:
				return {{996.0892334f, -368.4802246f}};
			case 699:
				return {{996.0892334f, -368.4802246f}};
			case 700:
				return {{996.0888672f, -368.4802246f}};
			case 701:
				return {{996.0888672f, -368.4802246f}};
			case 702:
				return {{996.0885620f, -368.4802246f}};
			case 703:
				return {{996.0885620f, -368.4802246f}};
			case 704:
				return {{996.0885620f, -368.4802246f}};
			case 705:
				return {{996.0883179f, -368.4802246f}};
			case 706:
				return {{996.0883179f, -368.4802246f}};
			case 707:
				return {{996.0883179f, -368.4802246f}};
			case 708:
				return {{996.0881348f, -368.4802246f}};
			case 709:
				return {{996.0881348f, -368.4802246f}};
			case 710:
				return {{996.0880127f, -368.4802246f}};
			case 711:
				return {{996.0880127f, -368.4802246f}};
			case 712:
				return {{996.0880127f, -368.4802246f}};
			case 713:
				return {{996.0878906f, -368.4802246f}};
			case 714:
				return {{996.0878906f, -368.4802246f}};
			case 715:
				return {{996.0878906f, -368.4802246f}};
			case 716:
				return {{996.0878296f, -368.4802246f}};
			case 717:
				return {{996.0878296f, -368.4802246f}};
			case 718:
				return {{996.0877686f, -368.4802246f}};
			case 719:
				return {{996.0877075f, -368.4802246f}};
			case 720:
				return {{996.0876465f, -368.4802246f}};
			case 721:
				return {{996.0876465f, -368.4802246f}};
			case 722:
				return {{996.0876465f, -368.4802246f}};
			case 723:
				return {{996.0876465f, -368.4802246f}};
			case 724:
				return {{996.0876465f, -368.4802246f}};
			case 725:
				return {{996.0876465f, -368.4802246f}};
			case 726:
				return {{996.0876465f, -368.4802246f}};
			case 727:
				return {{996.0876465f, -368.4802246f}};
			case 728:
				return {{996.0876465f, -368.4802246f}};
			case 729:
				return {{996.0876465f, -368.4802246f}};
			case 730:
				return {{996.0876465f, -368.4802246f}};
			case 731:
				return {{996.0876465f, -368.4802246f}};
			case 732:
				return {{996.0876465f, -368.4802246f}};
			case 733:
				return {{996.0876465f, -368.4802246f}};
			case 734:
				return {{996.0876465f, -368.4802246f}};
			case 735:
				return {{996.0876465f, -368.4802246f}};
			case 736:
				return {{996.0876465f, -368.4802246f}};
			case 737:
				return {{996.0876465f, -368.4802246f}};
			case 738:
				return {{996.0876465f, -368.4802246f}};
			case 739:
				return {{996.0876465f, -368.4802246f}};
			case 740:
				return {{996.0876465f, -368.4802246f}};
			case 741:
				return {{996.0876465f, -368.4802246f}};
			case 742:
				return {{996.0876465f, -368.4802246f}};
			case 743:
				return {{996.0876465f, -368.4802246f}};
			case 744:
				return {{996.0876465f, -368.4802246f}};
			case 745:
				return {{996.0876465f, -368.4802246f}};
			case 746:
				return {{996.0876465f, -368.4802246f}};
			case 747:
				return {{996.0876465f, -368.4802246f}};
			case 748:
				return {{996.0876465f, -368.4802246f}};
			case 749:
				return {{996.0876465f, -368.4802246f}};
			case 750:
				return {{996.0876465f, -368.4802246f}};
			case 751:
				return {{996.0876465f, -368.4802246f}};
			case 752:
				return {{996.0876465f, -368.4802246f}};
			case 753:
				return {{996.0876465f, -368.4802246f}};
			case 754:
				return {{996.0876465f, -368.4802246f}};
			case 755:
				return {{996.0876465f, -368.4802246f}};
			case 756:
				return {{996.0876465f, -368.4802246f}};
			case 757:
				return {{996.0876465f, -368.4802246f}};
			case 758:
				return {{996.0876465f, -368.4802246f}};
			case 759:
				return {{996.0876465f, -368.4802246f}};
			case 760:
				return {{996.0876465f, -368.4802246f}};
			case 761:
				return {{996.0876465f, -368.4802246f}};
			case 762:
				return {{996.0876465f, -368.4802246f}};
			case 763:
				return {{996.0876465f, -368.4802246f}};
			case 764:
				return {{996.0876465f, -368.4802246f}};
			case 765:
				return {{996.0876465f, -368.4802246f}};
			case 766:
				return {{996.0876465f, -368.4802246f}};
			case 767:
				return {{996.0876465f, -368.4802246f}};
			case 768:
				return {{996.0876465f, -368.4802246f}};
			case 769:
				return {{996.0876465f, -368.4802246f}};
			case 770:
				return {{996.0876465f, -368.4802246f}};
			case 771:
				return {{996.0876465f, -368.4802246f}};
			case 772:
				return {{996.0876465f, -368.4802246f}};
			case 773:
				return {{996.0876465f, -368.4802246f}};
			case 774:
				return {{996.0876465f, -368.4802246f}};
			case 775:
				return {{996.0876465f, -368.4802246f}};
			case 776:
				return {{996.0876465f, -368.4802246f}};
			case 777:
				return {{996.0876465f, -368.4802246f}};
			case 778:
				return {{996.0876465f, -368.4802246f}};
			case 779:
				return {{996.0876465f, -368.4802246f}};
			case 780:
				return {{996.0876465f, -368.4802246f}};
			case 781:
				return {{996.0876465f, -368.4802246f}};
			case 782:
				return {{996.0876465f, -368.4802246f}};
			case 783:
				return {{996.0876465f, -368.4802246f}};
			case 784:
				return {{996.0876465f, -368.4802246f}};
			case 785:
				return {{996.0876465f, -368.4802246f}};
			case 786:
				return {{996.0876465f, -368.4802246f}};
			case 787:
				return {{996.0876465f, -368.4802246f}};
			case 788:
				return {{996.0876465f, -368.4802246f}};
			case 789:
				return {{996.0876465f, -368.4802246f}};
			case 790:
				return {{996.0876465f, -368.4802246f}};
			case 791:
				return {{996.0876465f, -368.4802246f}};
			case 792:
				return {{996.0876465f, -368.4802246f}};
			case 793:
				return {{996.0876465f, -368.4802246f}};
			case 794:
				return {{996.0876465f, -368.4802246f}};
			case 795:
				return {{996.0876465f, -368.4802246f}};
			case 796:
				return {{996.0876465f, -368.4802246f}};
			case 797:
				return {{996.0876465f, -368.4802246f}};
			case 798:
				return {{996.0876465f, -368.4802246f}};
			case 799:
				return {{996.0876465f, -368.4802246f}};
			case 800:
				return {{996.0876465f, -368.4802246f}};
			case 801:
				return {{996.0876465f, -368.4802246f}};
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
				return {{983.3161621f, 1075.0819092f}};
			case 502:
				return {{979.9863281f, 1075.0822754f}};
			case 601:
				return {{993.1384888f, 1075.0822754f}};
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
				return {{983.3161621f, 1097.5008545f}};
			case 502:
				return {{979.9863281f, 1097.5008545f}};
			case 601:
				return {{993.1384888f, 1097.5008545f}};
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
				return {{983.3161621f, 1104.8609619f}};
			case 502:
				return {{979.9863281f, 1104.8609619f}};
			case 601:
				return {{993.1384888f, 1104.8609619f}};
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
				return {{983.3161621f, 1108.6680908f}};
			case 502:
				return {{979.9863281f, 1108.6680908f}};
			case 601:
				return {{993.1384888f, 1108.6680908f}};
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
				return {{983.3161621f, 1110.8964844f}};
			case 502:
				return {{979.9863281f, 1110.8964844f}};
			case 601:
				return {{993.1384888f, 1110.8964844f}};
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
				return {{983.3161621f, 1112.4276123f}};
			case 502:
				return {{979.9863281f, 1112.4276123f}};
			case 601:
				return {{993.1384888f, 1112.4276123f}};
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
				return {{983.3161621f, 1113.4935303f}};
			case 502:
				return {{979.9863281f, 1113.4935303f}};
			case 601:
				return {{993.1384888f, 1113.4935303f}};
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
				return {{995.9811401f, 1114.1197510f}};
			case 204:
				return {{995.9741211f, 1114.1197510f}};
			case 205:
				return {{995.9732056f, 1114.1197510f}};
			case 206:
				return {{995.9081421f, 1114.1198730f}};
			case 207:
				return {{995.8472290f, 1114.1198730f}};
			case 208:
				return {{995.8422852f, 1114.1198730f}};
			case 209:
				return {{995.8371582f, 1114.1198730f}};
			case 210:
				return {{995.7375488f, 1114.1198730f}};
			case 211:
				return {{995.7303467f, 1114.1198730f}};
			case 212:
				return {{995.5975952f, 1114.1198730f}};
			case 213:
				return {{995.5883179f, 1114.1198730f}};
			case 214:
				return {{995.4346924f, 1114.1198730f}};
			case 215:
				return {{995.4236450f, 1114.1198730f}};
			case 216:
				return {{995.2304688f, 1114.1198730f}};
			case 217:
				return {{995.2175293f, 1114.1198730f}};
			case 218:
				return {{994.9965820f, 1114.1198730f}};
			case 219:
				return {{994.9820557f, 1114.1198730f}};
			case 220:
				return {{994.7519531f, 1114.1198730f}};
			case 221:
				return {{994.7360229f, 1114.1198730f}};
			case 222:
				return {{994.4685059f, 1114.1198730f}};
			case 223:
				return {{994.4512329f, 1114.1198730f}};
			case 224:
				return {{994.1823730f, 1114.1198730f}};
			case 225:
				return {{994.1638794f, 1114.1198730f}};
			case 226:
				return {{993.8601074f, 1114.1198730f}};
			case 227:
				return {{993.8406372f, 1114.1198730f}};
			case 228:
				return {{993.5217896f, 1114.1198730f}};
			case 229:
				return {{993.5014648f, 1114.1198730f}};
			case 230:
				return {{992.8524780f, 1114.1198730f}};
			case 231:
				return {{992.5074463f, 1114.1198730f}};
			case 232:
				return {{992.4856567f, 1114.1198730f}};
			case 233:
				return {{992.4637451f, 1114.1198730f}};
			case 234:
				return {{992.1309814f, 1114.1198730f}};
			case 235:
				return {{991.7690430f, 1114.1198730f}};
			case 236:
				return {{991.7462769f, 1114.1198730f}};
			case 237:
				return {{991.4012451f, 1114.1198730f}};
			case 238:
				return {{991.3781128f, 1114.1198730f}};
			case 239:
				return {{991.0051270f, 1114.1198730f}};
			case 240:
				return {{990.9816895f, 1114.1198730f}};
			case 241:
				return {{990.6048584f, 1114.1198730f}};
			case 242:
				return {{990.5811768f, 1114.1198730f}};
			case 243:
				return {{990.2252197f, 1114.1198730f}};
			case 244:
				return {{990.2014160f, 1114.1198730f}};
			case 245:
				return {{989.8193359f, 1114.1199951f}};
			case 246:
				return {{989.7954102f, 1114.1198730f}};
			case 247:
				return {{989.4116821f, 1114.1198730f}};
			case 248:
				return {{989.3876953f, 1114.1198730f}};
			case 249:
				return {{989.0268555f, 1114.1198730f}};
			case 250:
				return {{989.0027466f, 1114.1198730f}};
			case 251:
				return {{988.6170044f, 1114.1198730f}};
			case 252:
				return {{988.5928955f, 1114.1198730f}};
			case 253:
				return {{988.5687866f, 1114.1199951f}};
			case 254:
				return {{987.8229370f, 1114.1199951f}};
			case 255:
				return {{987.4415283f, 1114.1198730f}};
			case 256:
				return {{987.4176636f, 1114.1198730f}};
			case 257:
				return {{987.0584717f, 1114.1198730f}};
			case 258:
				return {{987.0345459f, 1114.1198730f}};
			case 259:
				return {{987.0105591f, 1114.1199951f}};
			case 260:
				return {{986.6257324f, 1114.1198730f}};
			case 261:
				return {{986.6016846f, 1114.1198730f}};
			case 262:
				return {{986.2161865f, 1114.1198730f}};
			case 263:
				return {{986.1921387f, 1114.1198730f}};
			case 264:
				return {{985.8302002f, 1114.1198730f}};
			case 265:
				return {{985.4439697f, 1114.1198730f}};
			case 266:
				return {{985.4198608f, 1114.1198730f}};
			case 267:
				return {{985.0574951f, 1114.1198730f}};
			case 268:
				return {{985.0334473f, 1114.1198730f}};
			case 269:
				return {{984.6468506f, 1114.1198730f}};
			case 270:
				return {{984.6227417f, 1114.1198730f}};
			case 271:
				return {{984.2362061f, 1114.1198730f}};
			case 272:
				return {{984.2120972f, 1114.1198730f}};
			case 273:
				return {{983.8499756f, 1114.1198730f}};
			case 274:
				return {{983.8258057f, 1114.1198730f}};
			case 275:
				return {{983.8017578f, 1114.1198730f}};
			case 276:
				return {{983.4152832f, 1114.1198730f}};
			case 277:
				return {{983.3911743f, 1114.1198730f}};
			case 278:
				return {{983.3670654f, 1114.1198730f}};
			case 279:
				return {{982.9808960f, 1114.1198730f}};
			case 280:
				return {{982.6191406f, 1114.1198730f}};
			case 281:
				return {{982.2336426f, 1114.1198730f}};
			case 282:
				return {{982.2095947f, 1114.1198730f}};
			case 283:
				return {{982.1856079f, 1114.1198730f}};
			case 284:
				return {{981.8242188f, 1114.1198730f}};
			case 285:
				return {{981.8001709f, 1114.1198730f}};
			case 286:
				return {{981.7761230f, 1114.1198730f}};
			case 287:
				return {{981.3906250f, 1114.1198730f}};
			case 288:
				return {{981.3665771f, 1114.1198730f}};
			case 289:
				return {{981.3425293f, 1114.1198730f}};
			case 290:
				return {{980.9571533f, 1114.1198730f}};
			case 291:
				return {{980.9331665f, 1114.1198730f}};
			case 292:
				return {{980.9091187f, 1114.1198730f}};
			case 293:
				return {{980.5480347f, 1114.1198730f}};
			case 294:
				return {{980.5239868f, 1114.1198730f}};
			case 295:
				return {{980.1389160f, 1114.1198730f}};
			case 296:
				return {{980.1149902f, 1114.1198730f}};
			case 297:
				return {{980.0909424f, 1114.1198730f}};
			case 298:
				return {{979.7061157f, 1114.1198730f}};
			case 299:
				return {{979.6820679f, 1114.1198730f}};
			case 300:
				return {{979.6581421f, 1114.1198730f}};
			case 301:
				return {{979.2974854f, 1114.1198730f}};
			case 302:
				return {{979.2734375f, 1114.1199951f}};
			case 303:
				return {{978.8956299f, 1114.1198730f}};
			case 304:
				return {{978.8724365f, 1114.1198730f}};
			case 305:
				return {{978.8493652f, 1114.1198730f}};
			case 306:
				return {{978.5123291f, 1114.1198730f}};
			case 307:
				return {{978.4905396f, 1114.1198730f}};
			case 308:
				return {{978.1551514f, 1114.1198730f}};
			case 309:
				return {{977.8467407f, 1114.1198730f}};
			case 310:
				return {{977.8284302f, 1114.1198730f}};
			case 311:
				return {{977.5671997f, 1114.1198730f}};
			case 312:
				return {{977.5507202f, 1114.1198730f}};
			case 313:
				return {{977.5344238f, 1114.1198730f}};
			case 314:
				return {{977.2879639f, 1114.1198730f}};
			case 315:
				return {{977.2735596f, 1114.1198730f}};
			case 316:
				return {{977.2592163f, 1114.1198730f}};
			case 317:
				return {{977.0449219f, 1114.1198730f}};
			case 318:
				return {{977.0324707f, 1114.1199951f}};
			case 319:
				return {{976.8576660f, 1114.1198730f}};
			case 320:
				return {{976.8468628f, 1114.1198730f}};
			case 321:
				return {{976.8360596f, 1114.1198730f}};
			case 322:
				return {{976.6771240f, 1114.1198730f}};
			case 323:
				return {{976.6679688f, 1114.1198730f}};
			case 324:
				return {{976.6588745f, 1114.1198730f}};
			case 325:
				return {{976.5331421f, 1114.1198730f}};
			case 326:
				return {{976.5253906f, 1114.1198730f}};
			case 327:
				return {{976.5177002f, 1114.1198730f}};
			case 328:
				return {{976.4057617f, 1114.1198730f}};
			case 329:
				return {{976.3994751f, 1114.1198730f}};
			case 330:
				return {{976.3931885f, 1114.1199951f}};
			case 331:
				return {{976.3012695f, 1114.1198730f}};
			case 332:
				return {{976.2960815f, 1114.1198730f}};
			case 333:
				return {{976.2909546f, 1114.1198730f}};
			case 334:
				return {{976.2206421f, 1114.1198730f}};
			case 335:
				return {{976.2163696f, 1114.1198730f}};
			case 336:
				return {{976.2121582f, 1114.1198730f}};
			case 337:
				return {{976.1516113f, 1114.1198730f}};
			case 338:
				return {{976.1481323f, 1114.1198730f}};
			case 339:
				return {{976.1447754f, 1114.1198730f}};
			case 340:
				return {{976.0968628f, 1114.1198730f}};
			case 341:
				return {{976.0942383f, 1114.1198730f}};
			case 342:
				return {{976.0584106f, 1114.1198730f}};
			case 343:
				return {{976.0278931f, 1114.1198730f}};
			case 344:
				return {{976.0054932f, 1114.1198730f}};
			case 345:
				return {{976.0041504f, 1114.1198730f}};
			case 346:
				return {{976.0028687f, 1114.1198730f}};
			case 347:
				return {{975.9849854f, 1114.1198730f}};
			case 348:
				return {{975.9839478f, 1114.1198730f}};
			case 349:
				return {{975.9708252f, 1114.1198730f}};
			case 350:
				return {{975.9701538f, 1114.1198730f}};
			case 351:
				return {{975.9694824f, 1114.1198730f}};
			case 352:
				return {{975.9607544f, 1114.1198730f}};
			case 353:
				return {{975.9602051f, 1114.1198730f}};
			case 354:
				return {{975.9538574f, 1114.1198730f}};
			case 355:
				return {{975.9534912f, 1114.1198730f}};
			case 356:
				return {{975.9532471f, 1114.1198730f}};
			case 357:
				return {{975.9492188f, 1114.1198730f}};
			case 358:
				return {{975.9490967f, 1114.1198730f}};
			case 359:
				return {{975.9470215f, 1114.1198730f}};
			case 360:
				return {{975.9468994f, 1114.1198730f}};
			case 361:
				return {{975.9467163f, 1114.1198730f}};
			case 362:
				return {{975.9458008f, 1114.1198730f}};
			case 363:
				return {{975.9458008f, 1114.1198730f}};
			case 364:
				return {{975.9458008f, 1114.1198730f}};
			case 365:
				return {{975.9458008f, 1114.1198730f}};
			case 366:
				return {{975.9459839f, 1114.1198730f}};
			case 367:
				return {{975.9460449f, 1114.1198730f}};
			case 368:
				return {{975.9467773f, 1114.1198730f}};
			case 369:
				return {{975.9468994f, 1114.1198730f}};
			case 370:
				return {{975.9481201f, 1114.1198730f}};
			case 371:
				return {{975.9481812f, 1114.1198730f}};
			case 372:
				return {{975.9482422f, 1114.1198730f}};
			case 373:
				return {{975.9496460f, 1114.1198730f}};
			case 374:
				return {{975.9512939f, 1114.1198730f}};
			case 375:
				return {{975.9528809f, 1114.1198730f}};
			case 376:
				return {{975.9530640f, 1114.1198730f}};
			case 377:
				return {{975.9531250f, 1114.1198730f}};
			case 378:
				return {{975.9546509f, 1114.1198730f}};
			case 379:
				return {{975.9548340f, 1114.1198730f}};
			case 380:
				return {{975.9549561f, 1114.1198730f}};
			case 381:
				return {{975.9564819f, 1114.1198730f}};
			case 382:
				return {{975.9566650f, 1114.1198730f}};
			case 383:
				return {{975.9567871f, 1114.1198730f}};
			case 384:
				return {{975.9581299f, 1114.1198730f}};
			case 385:
				return {{975.9581909f, 1114.1198730f}};
			case 386:
				return {{975.9582520f, 1114.1198730f}};
			case 387:
				return {{975.9595337f, 1114.1198730f}};
			case 388:
				return {{975.9595947f, 1114.1198730f}};
			case 389:
				return {{975.9608154f, 1114.1198730f}};
			case 390:
				return {{975.9609375f, 1114.1198730f}};
			case 391:
				return {{975.9609985f, 1114.1198730f}};
			case 392:
				return {{975.9619751f, 1114.1198730f}};
			case 393:
				return {{975.9619751f, 1114.1198730f}};
			case 394:
				return {{975.9620361f, 1114.1198730f}};
			case 395:
				return {{975.9630127f, 1114.1198730f}};
			case 396:
				return {{975.9630737f, 1114.1198730f}};
			case 397:
				return {{975.9631348f, 1114.1198730f}};
			case 398:
				return {{975.9638672f, 1114.1198730f}};
			case 399:
				return {{975.9639893f, 1114.1198730f}};
			case 400:
				return {{975.9645996f, 1114.1198730f}};
			case 401:
				return {{975.9646606f, 1114.1198730f}};
			case 402:
				return {{975.9647217f, 1114.1198730f}};
			case 403:
				return {{975.9652710f, 1114.1198730f}};
			case 404:
				return {{975.9653320f, 1114.1198730f}};
			case 405:
				return {{975.9653320f, 1114.1198730f}};
			case 406:
				return {{975.9658203f, 1114.1198730f}};
			case 407:
				return {{975.9662476f, 1114.1198730f}};
			case 408:
				return {{975.9665527f, 1114.1198730f}};
			case 409:
				return {{975.9665527f, 1114.1198730f}};
			case 410:
				return {{975.9665527f, 1114.1198730f}};
			case 411:
				return {{975.9667969f, 1114.1198730f}};
			case 412:
				return {{975.9667969f, 1114.1198730f}};
			case 413:
				return {{975.9667969f, 1114.1198730f}};
			case 414:
				return {{975.9670410f, 1114.1198730f}};
			case 415:
				return {{975.9670410f, 1114.1198730f}};
			case 416:
				return {{975.9670410f, 1114.1198730f}};
			case 417:
				return {{975.9671631f, 1114.1198730f}};
			case 418:
				return {{975.9671631f, 1114.1198730f}};
			case 419:
				return {{975.9671631f, 1114.1198730f}};
			case 420:
				return {{975.9673462f, 1114.1198730f}};
			case 421:
				return {{975.9673462f, 1114.1198730f}};
			case 422:
				return {{975.9674683f, 1114.1198730f}};
			case 423:
				return {{975.9674683f, 1114.1198730f}};
			case 424:
				return {{975.9674683f, 1114.1198730f}};
			case 425:
				return {{975.9674683f, 1114.1198730f}};
			case 426:
				return {{975.9674683f, 1114.1198730f}};
			case 427:
				return {{975.9674683f, 1114.1198730f}};
			case 428:
				return {{975.9675293f, 1114.1198730f}};
			case 429:
				return {{975.9675293f, 1114.1198730f}};
			case 430:
				return {{975.9675293f, 1114.1198730f}};
			case 431:
				return {{975.9676514f, 1114.1198730f}};
			case 432:
				return {{975.9676514f, 1114.1198730f}};
			case 433:
				return {{975.9676514f, 1114.1198730f}};
			case 434:
				return {{975.9676514f, 1114.1198730f}};
			case 435:
				return {{975.9676514f, 1114.1198730f}};
			case 436:
				return {{975.9676514f, 1114.1198730f}};
			case 437:
				return {{975.9676514f, 1114.1198730f}};
			case 438:
				return {{975.9676514f, 1114.1198730f}};
			case 439:
				return {{975.9676514f, 1114.1198730f}};
			case 440:
				return {{975.9676514f, 1114.1198730f}};
			case 441:
				return {{975.9676514f, 1114.1198730f}};
			case 442:
				return {{975.9676514f, 1114.1198730f}};
			case 443:
				return {{975.9676514f, 1114.1198730f}};
			case 444:
				return {{975.9676514f, 1114.1198730f}};
			case 445:
				return {{975.9676514f, 1114.1198730f}};
			case 446:
				return {{975.9676514f, 1114.1198730f}};
			case 447:
				return {{975.9676514f, 1114.1198730f}};
			case 448:
				return {{975.9676514f, 1114.1198730f}};
			case 449:
				return {{975.9676514f, 1114.1198730f}};
			case 450:
				return {{975.9676514f, 1114.1198730f}};
			case 451:
				return {{975.9676514f, 1114.1198730f}};
			case 452:
				return {{975.9676514f, 1114.1198730f}};
			case 453:
				return {{975.9676514f, 1114.1198730f}};
			case 454:
				return {{975.9676514f, 1114.1198730f}};
			case 455:
				return {{975.9676514f, 1114.1198730f}};
			case 456:
				return {{975.9676514f, 1114.1198730f}};
			case 457:
				return {{975.9676514f, 1114.1198730f}};
			case 458:
				return {{975.9676514f, 1114.1198730f}};
			case 459:
				return {{975.9676514f, 1114.1198730f}};
			case 460:
				return {{975.9676514f, 1114.1198730f}};
			case 461:
				return {{975.9676514f, 1114.1198730f}};
			case 462:
				return {{975.9676514f, 1114.1198730f}};
			case 463:
				return {{975.9676514f, 1114.1198730f}};
			case 464:
				return {{975.9676514f, 1114.1198730f}};
			case 465:
				return {{975.9676514f, 1114.1198730f}};
			case 466:
				return {{975.9676514f, 1114.1198730f}};
			case 467:
				return {{975.9676514f, 1114.1198730f}};
			case 468:
				return {{975.9676514f, 1114.1198730f}};
			case 469:
				return {{975.9676514f, 1114.1198730f}};
			case 470:
				return {{975.9676514f, 1114.1198730f}};
			case 471:
				return {{975.9676514f, 1114.1198730f}};
			case 472:
				return {{975.9676514f, 1114.1198730f}};
			case 473:
				return {{975.9676514f, 1114.1198730f}};
			case 474:
				return {{975.9676514f, 1114.1198730f}};
			case 475:
				return {{975.9676514f, 1114.1198730f}};
			case 476:
				return {{975.9676514f, 1114.1198730f}};
			case 477:
				return {{975.9676514f, 1114.1198730f}};
			case 478:
				return {{975.9676514f, 1114.1198730f}};
			case 479:
				return {{975.9676514f, 1114.1198730f}};
			case 480:
				return {{975.9676514f, 1114.1198730f}};
			case 481:
				return {{975.9676514f, 1114.1198730f}};
			case 482:
				return {{975.9676514f, 1114.1198730f}};
			case 483:
				return {{975.9676514f, 1114.1198730f}};
			case 484:
				return {{975.9676514f, 1114.1198730f}};
			case 485:
				return {{975.9676514f, 1114.1198730f}};
			case 486:
				return {{975.9676514f, 1114.1198730f}};
			case 487:
				return {{975.9676514f, 1114.1198730f}};
			case 488:
				return {{975.9676514f, 1114.1198730f}};
			case 489:
				return {{975.9676514f, 1114.1198730f}};
			case 490:
				return {{975.9676514f, 1114.1198730f}};
			case 491:
				return {{975.9676514f, 1114.1198730f}};
			case 492:
				return {{975.9676514f, 1114.1198730f}};
			case 493:
				return {{975.9676514f, 1114.1198730f}};
			case 494:
				return {{975.9676514f, 1114.1198730f}};
			case 495:
				return {{975.9676514f, 1114.1198730f}};
			case 496:
				return {{975.9676514f, 1114.1198730f}};
			case 497:
				return {{975.9676514f, 1114.1198730f}};
			case 498:
				return {{975.9676514f, 1114.1198730f}};
			case 499:
				return {{975.9676514f, 1114.1198730f}};
			case 500:
				return {{975.9676514f, 1114.1198730f}};
			case 501:
				return {{975.9676514f, 1114.1198730f}};
			case 502:
				return {{975.9676514f, 1114.1198730f}};
			case 503:
				return {{975.9679565f, 1114.1198730f}};
			case 504:
				return {{975.9902344f, 1114.1198730f}};
			case 505:
				return {{975.9915771f, 1114.1198730f}};
			case 506:
				return {{975.9930420f, 1114.1198730f}};
			case 507:
				return {{975.9946289f, 1114.1198730f}};
			case 508:
				return {{976.0357666f, 1114.1198730f}};
			case 509:
				return {{976.0391846f, 1114.1198730f}};
			case 510:
				return {{976.1117554f, 1114.1198730f}};
			case 511:
				return {{976.1172485f, 1114.1198730f}};
			case 512:
				return {{976.2150879f, 1114.1198730f}};
			case 513:
				return {{976.2225342f, 1114.1198730f}};
			case 514:
				return {{976.2301025f, 1114.1198730f}};
			case 515:
				return {{976.2378540f, 1114.1198730f}};
			case 516:
				return {{976.3782349f, 1114.1198730f}};
			case 517:
				return {{976.3879395f, 1114.1199951f}};
			case 518:
				return {{976.5594482f, 1114.1198730f}};
			case 519:
				return {{976.5710449f, 1114.1198730f}};
			case 520:
				return {{976.5827026f, 1114.1198730f}};
			case 521:
				return {{976.7719727f, 1114.1198730f}};
			case 522:
				return {{976.7852783f, 1114.1198730f}};
			case 523:
				return {{976.7987671f, 1114.1198730f}};
			case 524:
				return {{977.0282593f, 1114.1198730f}};
			case 525:
				return {{977.0433960f, 1114.1198730f}};
			case 526:
				return {{977.0585938f, 1114.1198730f}};
			case 527:
				return {{977.2971191f, 1114.1198730f}};
			case 528:
				return {{977.3135986f, 1114.1198730f}};
			case 529:
				return {{977.3302612f, 1114.1198730f}};
			case 530:
				return {{977.6068115f, 1114.1198730f}};
			case 531:
				return {{977.6245728f, 1114.1198730f}};
			case 532:
				return {{977.6425781f, 1114.1199951f}};
			case 533:
				return {{977.9387817f, 1114.1198730f}};
			case 534:
				return {{977.9577637f, 1114.1198730f}};
			case 535:
				return {{977.9767456f, 1114.1198730f}};
			case 536:
				return {{978.5921631f, 1114.1198730f}};
			case 537:
				return {{978.6124268f, 1114.1199951f}};
			case 538:
				return {{978.9444580f, 1114.1198730f}};
			case 539:
				return {{978.9655151f, 1114.1198730f}};
			case 540:
				return {{978.9867554f, 1114.1198730f}};
			case 541:
				return {{979.3093262f, 1114.1198730f}};
			case 542:
				return {{979.3311157f, 1114.1198730f}};
			case 543:
				return {{979.3529663f, 1114.1198730f}};
			case 544:
				return {{979.7069702f, 1114.1198730f}};
			case 545:
				return {{979.7293091f, 1114.1198730f}};
			case 546:
				return {{979.7516479f, 1114.1198730f}};
			case 547:
				return {{980.0910034f, 1114.1198730f}};
			case 548:
				return {{980.1138306f, 1114.1198730f}};
			case 549:
				return {{980.1366577f, 1114.1198730f}};
			case 550:
				return {{980.5050659f, 1114.1198730f}};
			case 551:
				return {{980.5281982f, 1114.1198730f}};
			case 552:
				return {{980.9012451f, 1114.1198730f}};
			case 553:
				return {{980.9246216f, 1114.1198730f}};
			case 554:
				return {{981.2777100f, 1114.1198730f}};
			case 555:
				return {{981.3013306f, 1114.1198730f}};
			case 556:
				return {{981.3249512f, 1114.1198730f}};
			case 557:
				return {{981.7047729f, 1114.1198730f}};
			case 558:
				return {{981.7285767f, 1114.1198730f}};
			case 559:
				return {{981.7523804f, 1114.1198730f}};
			case 560:
				return {{982.1344604f, 1114.1198730f}};
			case 561:
				return {{982.1584473f, 1114.1198730f}};
			case 562:
				return {{982.1823730f, 1114.1198730f}};
			case 563:
				return {{982.5421753f, 1114.1198730f}};
			case 564:
				return {{982.5661621f, 1114.1198730f}};
			case 565:
				return {{982.5900879f, 1114.1198730f}};
			case 566:
				return {{982.9752197f, 1114.1198730f}};
			case 567:
				return {{982.9992676f, 1114.1198730f}};
			case 568:
				return {{983.3607178f, 1114.1198730f}};
			case 569:
				return {{983.7467041f, 1114.1198730f}};
			case 570:
				return {{984.1326294f, 1114.1198730f}};
			case 571:
				return {{984.1567383f, 1114.1198730f}};
			case 572:
				return {{984.1808472f, 1114.1198730f}};
			case 573:
				return {{984.5429077f, 1114.1198730f}};
			case 574:
				return {{984.5670166f, 1114.1198730f}};
			case 575:
				return {{984.5911255f, 1114.1198730f}};
			case 576:
				return {{984.9775391f, 1114.1198730f}};
			case 577:
				return {{985.0017090f, 1114.1198730f}};
			case 578:
				return {{985.0257568f, 1114.1198730f}};
			case 579:
				return {{985.4121094f, 1114.1198730f}};
			case 580:
				return {{985.4362793f, 1114.1198730f}};
			case 581:
				return {{985.4603271f, 1114.1198730f}};
			case 582:
				return {{985.8225098f, 1114.1198730f}};
			case 583:
				return {{985.8466187f, 1114.1198730f}};
			case 584:
				return {{986.2326660f, 1114.1198730f}};
			case 585:
				return {{986.2567749f, 1114.1198730f}};
			case 586:
				return {{986.2808838f, 1114.1198730f}};
			case 587:
				return {{986.6427612f, 1114.1198730f}};
			case 588:
				return {{986.6668701f, 1114.1198730f}};
			case 589:
				return {{986.6909790f, 1114.1198730f}};
			case 590:
				return {{987.0767822f, 1114.1198730f}};
			case 591:
				return {{987.1008911f, 1114.1198730f}};
			case 592:
				return {{987.1249390f, 1114.1198730f}};
			case 593:
				return {{987.5104980f, 1114.1198730f}};
			case 594:
				return {{987.5345459f, 1114.1198730f}};
			case 595:
				return {{987.5586548f, 1114.1198730f}};
			case 596:
				return {{987.9199829f, 1114.1198730f}};
			case 597:
				return {{987.9440308f, 1114.1198730f}};
			case 598:
				return {{987.9680786f, 1114.1198730f}};
			case 599:
				return {{988.3533325f, 1114.1199951f}};
			case 600:
				return {{988.3774414f, 1114.1198730f}};
			case 601:
				return {{989.1198120f, 1114.1198730f}};
			case 602:
				return {{989.1433716f, 1114.1198730f}};
			case 603:
				return {{989.5091553f, 1114.1198730f}};
			case 604:
				return {{989.5313110f, 1114.1198730f}};
			case 605:
				return {{989.8508301f, 1114.1198730f}};
			case 606:
				return {{989.8712769f, 1114.1198730f}};
			case 607:
				return {{989.8916016f, 1114.1198730f}};
			case 608:
				return {{990.2015381f, 1114.1198730f}};
			case 609:
				return {{990.2200317f, 1114.1198730f}};
			case 610:
				return {{990.4984131f, 1114.1198730f}};
			case 611:
				return {{990.5148315f, 1114.1198730f}};
			case 612:
				return {{990.7468262f, 1114.1198730f}};
			case 613:
				return {{990.7614136f, 1114.1198730f}};
			case 614:
				return {{990.9789429f, 1114.1198730f}};
			case 615:
				return {{990.9915771f, 1114.1198730f}};
			case 616:
				return {{991.0040894f, 1114.1198730f}};
			case 617:
				return {{991.1907349f, 1114.1198730f}};
			case 618:
				return {{991.2015381f, 1114.1198730f}};
			case 619:
				return {{991.2122192f, 1114.1198730f}};
			case 620:
				return {{991.3611450f, 1114.1198730f}};
			case 621:
				return {{991.3703613f, 1114.1198730f}};
			case 622:
				return {{991.5056763f, 1114.1198730f}};
			case 623:
				return {{991.5134277f, 1114.1198730f}};
			case 624:
				return {{991.5210571f, 1114.1198730f}};
			case 625:
				return {{991.6268311f, 1114.1198730f}};
			case 626:
				return {{991.6333008f, 1114.1198730f}};
			case 627:
				return {{991.6397095f, 1114.1198730f}};
			case 628:
				return {{991.8099365f, 1114.1198730f}};
			case 629:
				return {{991.8142700f, 1114.1198730f}};
			case 630:
				return {{991.8732910f, 1114.1198730f}};
			case 631:
				return {{991.8768921f, 1114.1198730f}};
			case 632:
				return {{991.8803711f, 1114.1198730f}};
			case 633:
				return {{991.9306030f, 1114.1198730f}};
			case 634:
				return {{991.9334717f, 1114.1198730f}};
			case 635:
				return {{991.9733276f, 1114.1198730f}};
			case 636:
				return {{991.9755859f, 1114.1198730f}};
			case 637:
				return {{991.9777832f, 1114.1198730f}};
			case 638:
				return {{992.0067749f, 1114.1198730f}};
			case 639:
				return {{992.0084229f, 1114.1198730f}};
			case 640:
				return {{992.0101929f, 1114.1198730f}};
			case 641:
				return {{992.0337524f, 1114.1198730f}};
			case 642:
				return {{992.0350342f, 1114.1198730f}};
			case 643:
				return {{992.0518799f, 1114.1198730f}};
			case 644:
				return {{992.0528564f, 1114.1198730f}};
			case 645:
				return {{992.0538330f, 1114.1198730f}};
			case 646:
				return {{992.0668945f, 1114.1198730f}};
			case 647:
				return {{992.0675659f, 1114.1198730f}};
			case 648:
				return {{992.0682373f, 1114.1198730f}};
			case 649:
				return {{992.0772705f, 1114.1198730f}};
			case 650:
				return {{992.0776978f, 1114.1198730f}};
			case 651:
				return {{992.0834961f, 1114.1198730f}};
			case 652:
				return {{992.0838013f, 1114.1198730f}};
			case 653:
				return {{992.0841064f, 1114.1198730f}};
			case 654:
				return {{992.0880127f, 1114.1198730f}};
			case 655:
				return {{992.0881348f, 1114.1198730f}};
			case 656:
				return {{992.0902100f, 1114.1198730f}};
			case 657:
				return {{992.0903320f, 1114.1198730f}};
			case 658:
				return {{992.0911255f, 1114.1198730f}};
			case 659:
				return {{992.0910645f, 1114.1198730f}};
			case 660:
				return {{992.0903931f, 1114.1198730f}};
			case 661:
				return {{992.0903320f, 1114.1198730f}};
			case 662:
				return {{992.0902100f, 1114.1198730f}};
			case 663:
				return {{992.0891113f, 1114.1198730f}};
			case 664:
				return {{992.0889893f, 1114.1198730f}};
			case 665:
				return {{992.0875244f, 1114.1198730f}};
			case 666:
				return {{992.0874634f, 1114.1198730f}};
			case 667:
				return {{992.0873413f, 1114.1198730f}};
			case 668:
				return {{992.0858154f, 1114.1198730f}};
			case 669:
				return {{992.0856323f, 1114.1198730f}};
			case 670:
				return {{992.0855713f, 1114.1198730f}};
			case 671:
				return {{992.0838013f, 1114.1198730f}};
			case 672:
				return {{992.0837402f, 1114.1198730f}};
			case 673:
				return {{992.0820312f, 1114.1198730f}};
			case 674:
				return {{992.0819702f, 1114.1198730f}};
			case 675:
				return {{992.0818481f, 1114.1198730f}};
			case 676:
				return {{992.0803223f, 1114.1198730f}};
			case 677:
				return {{992.0801392f, 1114.1198730f}};
			case 678:
				return {{992.0800781f, 1114.1198730f}};
			case 679:
				return {{992.0786133f, 1114.1198730f}};
			case 680:
				return {{992.0785522f, 1114.1198730f}};
			case 681:
				return {{992.0772705f, 1114.1198730f}};
			case 682:
				return {{992.0771484f, 1114.1198730f}};
			case 683:
				return {{992.0770874f, 1114.1198730f}};
			case 684:
				return {{992.0758667f, 1114.1198730f}};
			case 685:
				return {{992.0758667f, 1114.1198730f}};
			case 686:
				return {{992.0747070f, 1114.1198730f}};
			case 687:
				return {{992.0746460f, 1114.1198730f}};
			case 688:
				return {{992.0737305f, 1114.1198730f}};
			case 689:
				return {{992.0728760f, 1114.1198730f}};
			case 690:
				return {{992.0722046f, 1114.1198730f}};
			case 691:
				return {{992.0721436f, 1114.1198730f}};
			case 692:
				return {{992.0720825f, 1114.1198730f}};
			case 693:
				return {{992.0715332f, 1114.1198730f}};
			case 694:
				return {{992.0714722f, 1114.1198730f}};
			case 695:
				return {{992.0709839f, 1114.1198730f}};
			case 696:
				return {{992.0709839f, 1114.1198730f}};
			case 697:
				return {{992.0709839f, 1114.1198730f}};
			case 698:
				return {{992.0705566f, 1114.1198730f}};
			case 699:
				return {{992.0705566f, 1114.1198730f}};
			case 700:
				return {{992.0701904f, 1114.1198730f}};
			case 701:
				return {{992.0701904f, 1114.1198730f}};
			case 702:
				return {{992.0698242f, 1114.1198730f}};
			case 703:
				return {{992.0698242f, 1114.1198730f}};
			case 704:
				return {{992.0698242f, 1114.1198730f}};
			case 705:
				return {{992.0696411f, 1114.1198730f}};
			case 706:
				return {{992.0696411f, 1114.1198730f}};
			case 707:
				return {{992.0696411f, 1114.1198730f}};
			case 708:
				return {{992.0694580f, 1114.1198730f}};
			case 709:
				return {{992.0694580f, 1114.1198730f}};
			case 710:
				return {{992.0693359f, 1114.1198730f}};
			case 711:
				return {{992.0693359f, 1114.1198730f}};
			case 712:
				return {{992.0693359f, 1114.1198730f}};
			case 713:
				return {{992.0691528f, 1114.1198730f}};
			case 714:
				return {{992.0691528f, 1114.1198730f}};
			case 715:
				return {{992.0691528f, 1114.1198730f}};
			case 716:
				return {{992.0691528f, 1114.1198730f}};
			case 717:
				return {{992.0691528f, 1114.1198730f}};
			case 718:
				return {{992.0690918f, 1114.1198730f}};
			case 719:
				return {{992.0690308f, 1114.1198730f}};
			case 720:
				return {{992.0689697f, 1114.1198730f}};
			case 721:
				return {{992.0689697f, 1114.1198730f}};
			case 722:
				return {{992.0689697f, 1114.1198730f}};
			case 723:
				return {{992.0689697f, 1114.1198730f}};
			case 724:
				return {{992.0689697f, 1114.1198730f}};
			case 725:
				return {{992.0689697f, 1114.1198730f}};
			case 726:
				return {{992.0689697f, 1114.1198730f}};
			case 727:
				return {{992.0689697f, 1114.1198730f}};
			case 728:
				return {{992.0689697f, 1114.1198730f}};
			case 729:
				return {{992.0689697f, 1114.1198730f}};
			case 730:
				return {{992.0689697f, 1114.1198730f}};
			case 731:
				return {{992.0689697f, 1114.1198730f}};
			case 732:
				return {{992.0689697f, 1114.1198730f}};
			case 733:
				return {{992.0689697f, 1114.1198730f}};
			case 734:
				return {{992.0689697f, 1114.1198730f}};
			case 735:
				return {{992.0689697f, 1114.1198730f}};
			case 736:
				return {{992.0689697f, 1114.1198730f}};
			case 737:
				return {{992.0689697f, 1114.1198730f}};
			case 738:
				return {{992.0689697f, 1114.1198730f}};
			case 739:
				return {{992.0689697f, 1114.1198730f}};
			case 740:
				return {{992.0689697f, 1114.1198730f}};
			case 741:
				return {{992.0689697f, 1114.1198730f}};
			case 742:
				return {{992.0689697f, 1114.1198730f}};
			case 743:
				return {{992.0689697f, 1114.1198730f}};
			case 744:
				return {{992.0689697f, 1114.1198730f}};
			case 745:
				return {{992.0689697f, 1114.1198730f}};
			case 746:
				return {{992.0689697f, 1114.1198730f}};
			case 747:
				return {{992.0689697f, 1114.1198730f}};
			case 748:
				return {{992.0689697f, 1114.1198730f}};
			case 749:
				return {{992.0689697f, 1114.1198730f}};
			case 750:
				return {{992.0689697f, 1114.1198730f}};
			case 751:
				return {{992.0689697f, 1114.1198730f}};
			case 752:
				return {{992.0689697f, 1114.1198730f}};
			case 753:
				return {{992.0689697f, 1114.1198730f}};
			case 754:
				return {{992.0689697f, 1114.1198730f}};
			case 755:
				return {{992.0689697f, 1114.1198730f}};
			case 756:
				return {{992.0689697f, 1114.1198730f}};
			case 757:
				return {{992.0689697f, 1114.1198730f}};
			case 758:
				return {{992.0689697f, 1114.1198730f}};
			case 759:
				return {{992.0689697f, 1114.1198730f}};
			case 760:
				return {{992.0689697f, 1114.1198730f}};
			case 761:
				return {{992.0689697f, 1114.1198730f}};
			case 762:
				return {{992.0689697f, 1114.1198730f}};
			case 763:
				return {{992.0689697f, 1114.1198730f}};
			case 764:
				return {{992.0689697f, 1114.1198730f}};
			case 765:
				return {{992.0689697f, 1114.1198730f}};
			case 766:
				return {{992.0689697f, 1114.1198730f}};
			case 767:
				return {{992.0689697f, 1114.1198730f}};
			case 768:
				return {{992.0689697f, 1114.1198730f}};
			case 769:
				return {{992.0689697f, 1114.1198730f}};
			case 770:
				return {{992.0689697f, 1114.1198730f}};
			case 771:
				return {{992.0689697f, 1114.1198730f}};
			case 772:
				return {{992.0689697f, 1114.1198730f}};
			case 773:
				return {{992.0689697f, 1114.1198730f}};
			case 774:
				return {{992.0689697f, 1114.1198730f}};
			case 775:
				return {{992.0689697f, 1114.1198730f}};
			case 776:
				return {{992.0689697f, 1114.1198730f}};
			case 777:
				return {{992.0689697f, 1114.1198730f}};
			case 778:
				return {{992.0689697f, 1114.1198730f}};
			case 779:
				return {{992.0689697f, 1114.1198730f}};
			case 780:
				return {{992.0689697f, 1114.1198730f}};
			case 781:
				return {{992.0689697f, 1114.1198730f}};
			case 782:
				return {{992.0689697f, 1114.1198730f}};
			case 783:
				return {{992.0689697f, 1114.1198730f}};
			case 784:
				return {{992.0689697f, 1114.1198730f}};
			case 785:
				return {{992.0689697f, 1114.1198730f}};
			case 786:
				return {{992.0689697f, 1114.1198730f}};
			case 787:
				return {{992.0689697f, 1114.1198730f}};
			case 788:
				return {{992.0689697f, 1114.1198730f}};
			case 789:
				return {{992.0689697f, 1114.1198730f}};
			case 790:
				return {{992.0689697f, 1114.1198730f}};
			case 791:
				return {{992.0689697f, 1114.1198730f}};
			case 792:
				return {{992.0689697f, 1114.1198730f}};
			case 793:
				return {{992.0689697f, 1114.1198730f}};
			case 794:
				return {{992.0689697f, 1114.1198730f}};
			case 795:
				return {{992.0689697f, 1114.1198730f}};
			case 796:
				return {{992.0689697f, 1114.1198730f}};
			case 797:
				return {{992.0689697f, 1114.1198730f}};
			case 798:
				return {{992.0689697f, 1114.1198730f}};
			case 799:
				return {{992.0689697f, 1114.1198730f}};
			case 800:
				return {{992.0689697f, 1114.1198730f}};
			case 801:
				return {{992.0689697f, 1114.1198730f}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		assert(false); // Shouldn't be any unaccounted raycasts
		return std::nullopt;
	}
};
