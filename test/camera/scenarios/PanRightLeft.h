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

class PanRightLeftMockAction final: public MockAction
{
public:
	static constexpr uint32_t k_Start1 = k_StabilizeFrames + 1;
	static constexpr uint32_t k_End1 = k_StabilizeFrames + k_InteractionFrames - 1;
	static constexpr uint32_t k_Start2 = k_StabilizeFrames + k_InteractionFrames + k_StabilizeFrames + 1;
	static constexpr uint32_t k_End2 = k_StabilizeFrames + k_InteractionFrames + k_StabilizeFrames + k_InteractionFrames - 1;

	~PanRightLeftMockAction() final = default;

	[[nodiscard]] bool GetBindable(openblack::input::BindableActionMap action) const final
	{
		using openblack::input::BindableActionMap;
		if (frameNumber >= k_Start1 && frameNumber <= k_End1)
		{
			return (static_cast<uint32_t>(action) & (static_cast<uint32_t>(BindableActionMap::ROTATE_ON) |
			                                         static_cast<uint32_t>(BindableActionMap::MOVE_LEFT))) != 0;
		}

		if (frameNumber >= k_Start2 && frameNumber <= k_End2)
		{
			return (static_cast<uint32_t>(action) & (static_cast<uint32_t>(BindableActionMap::ROTATE_ON) |
			                                         static_cast<uint32_t>(BindableActionMap::MOVE_RIGHT))) != 0;
		}

		return false;
	}
};

class PanRightLeftMockDynamicsSystem: public MockDynamicsSystem
{
public:
	PanRightLeftMockDynamicsSystem() = default;

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
				return {{0.0, 16.1538353}};
			case 1:
				return {{1000.0, -770.0559082}};
			case 2:
				return {{1000.0, -743.0767822}};
			case 3:
				return {{1000.0, -5.7276611}};
			case 4:
				return {{1000.0, 105.8948517}};
			case 5:
				return {{1000.0, 112.1076202}};
			case 6:
				return {{1000.0, 118.2447052}};
			case 7:
				return {{1000.0, 200.3244019}};
			case 8:
				return {{1000.0, 204.8554993}};
			case 9:
				return {{1000.0, 264.9228516}};
			case 10:
				return {{1000.0, 268.0856323}};
			case 11:
				return {{1000.0, 271.2250366}};
			case 12:
				return {{1000.0, 313.6578369}};
			case 13:
				return {{1000.0, 316.1343384}};
			case 14:
				return {{1000.0, 318.7072144}};
			case 15:
				return {{1000.0, 321.1528931}};
			case 16:
				return {{1000.0, 355.7194519}};
			case 17:
				return {{1000.0, 357.7429199}};
			case 18:
				return {{1000.0, 384.0479126}};
			case 19:
				return {{1000.0, 407.4533691}};
			case 20:
				return {{1000.0, 427.1724854}};
			case 21:
				return {{1000.0, 428.2545471}};
			case 22:
				return {{1000.0, 443.9299011}};
			case 23:
				return {{1000.0, 444.8811340}};
			case 24:
				return {{1000.0, 459.2140198}};
			case 25:
				return {{1000.0, 460.0471497}};
			case 26:
				return {{1000.0, 472.5143433}};
			case 27:
				return {{1000.0, 473.2416382}};
			case 28:
				return {{1000.0, 483.4656067}};
			case 29:
				return {{1000.0, 484.1685791}};
			case 30:
				return {{1000.0, 493.8484192}};
			case 31:
				return {{1000.0, 494.3926697}};
			case 32:
				return {{1000.0, 502.4442139}};
			case 33:
				return {{1000.0, 502.9736938}};
			case 34:
				return {{1000.0, 510.7427979}};
			case 35:
				return {{1000.0, 511.1937256}};
			case 36:
				return {{1000.0, 518.1301270}};
			case 37:
				return {{1000.0, 518.5700684}};
			case 38:
				return {{1000.0, 519.0095215}};
			case 39:
				return {{1000.0, 500.6634216}};
			case 40:
				return {{1000.0, 498.8476868}};
			case 41:
				return {{1000.0, 471.6894531}};
			case 42:
				return {{1000.0, 469.9794006}};
			case 43:
				return {{1000.0, 442.9660645}};
			case 44:
				return {{1000.0, 417.5871277}};
			case 45:
				return {{1000.0, 391.1619568}};
			case 46:
				return {{1000.0, 389.5305786}};
			case 47:
				return {{1000.0, 364.7833862}};
			case 48:
				return {{1000.0, 363.1747437}};
			case 49:
				return {{1000.0, 337.2171631}};
			case 50:
				return {{1000.0, 335.5335999}};
			case 51:
				return {{1000.0, 309.9683228}};
			case 52:
				return {{1000.0, 308.3560791}};
			case 53:
				return {{1000.0, 285.0237427}};
			case 54:
				return {{1000.0, 283.0931091}};
			case 55:
				return {{1000.0, 258.5480347}};
			case 56:
				return {{1000.0, 256.9194946}};
			case 57:
				return {{1000.0, 232.4271393}};
			case 58:
				return {{1000.0, 230.9935760}};
			case 59:
				return {{1000.0, 208.8545990}};
			case 60:
				return {{1000.0, 207.3721313}};
			case 61:
				return {{1000.0, 184.0393066}};
			case 62:
				return {{1000.0, 182.6068115}};
			case 63:
				return {{1000.0, 161.3394165}};
			case 64:
				return {{1000.0, 159.7380066}};
			case 65:
				return {{1000.0, 137.5003815}};
			case 66:
				return {{1000.0, 115.8879089}};
			case 67:
				return {{1000.0, 95.8544159}};
			case 68:
				return {{1000.0, 94.5362091}};
			case 69:
				return {{1000.0, 73.7548065}};
			case 70:
				return {{1000.0, 72.4786377}};
			case 71:
				return {{1000.0, 52.5038910}};
			case 72:
				return {{1000.0, 51.1960602}};
			case 73:
				return {{1000.0, 32.9644012}};
			case 74:
				return {{1000.0, 31.5235901}};
			case 75:
				return {{1000.0, 30.2701569}};
			case 76:
				return {{1000.0, 11.1421204}};
			case 77:
				return {{1000.0, 10.1613617}};
			case 78:
				return {{1000.0, -7.2264404}};
			case 79:
				return {{1000.0, -8.4018555}};
			case 80:
				return {{1000.0, -26.1702881}};
			case 81:
				return {{1000.0, -27.1201172}};
			case 82:
				return {{1000.0, -44.8266602}};
			case 83:
				return {{1000.0, -45.5584717}};
			case 84:
				return {{1000.0, -61.5065918}};
			case 85:
				return {{1000.0, -62.6777344}};
			case 86:
				return {{1000.0, -78.9976807}};
			case 87:
				return {{1000.0, -79.9364014}};
			case 88:
				return {{1000.0, -95.5395508}};
			case 89:
				return {{1000.0, -96.6873779}};
			case 90:
				return {{1000.0, -110.8873291}};
			case 91:
				return {{1000.0, -125.5928955}};
			case 92:
				return {{1000.0, -139.0494385}};
			case 93:
				return {{1000.0, -139.7036133}};
			case 94:
				return {{1000.0, -153.5233154}};
			case 95:
				return {{1000.0, -154.3769531}};
			case 96:
				return {{1000.0, -167.5012207}};
			case 97:
				return {{1000.0, -168.5506592}};
			case 98:
				return {{1000.0, -180.2821045}};
			case 99:
				return {{1000.0, -181.0770264}};
			case 100:
				return {{1000.0, -193.0888672}};
			case 101:
				return {{1000.0, -194.0817871}};
			case 102:
				return {{1000.0, -205.6433105}};
			case 103:
				return {{1000.0, -216.1108398}};
			case 104:
				return {{1000.0, -216.8082275}};
			case 105:
				return {{1000.0, -227.3505859}};
			case 106:
				return {{1000.0, -228.0187988}};
			case 107:
				return {{1000.0, -228.5657959}};
			case 108:
				return {{1000.0, -238.1468506}};
			case 109:
				return {{1000.0, -238.7722168}};
			case 110:
				return {{1000.0, -248.4827881}};
			case 111:
				return {{1000.0, -249.0815430}};
			case 112:
				return {{1000.0, -258.1691895}};
			case 113:
				return {{1000.0, -258.7281494}};
			case 114:
				return {{1000.0, -275.2501221}};
			case 115:
				return {{1000.0, -283.1724854}};
			case 116:
				return {{1000.0, -283.3957520}};
			case 117:
				return {{1000.0, -290.4317627}};
			case 118:
				return {{1000.0, -290.8327637}};
			case 119:
				return {{1000.0, -297.8603516}};
			case 120:
				return {{1000.0, -298.2362061}};
			case 121:
				return {{1000.0, -304.2756348}};
			case 122:
				return {{1000.0, -304.6058350}};
			case 123:
				return {{1000.0, -305.0563965}};
			case 124:
				return {{1000.0, -311.0228271}};
			case 125:
				return {{1000.0, -316.6230469}};
			case 126:
				return {{1000.0, -317.1135254}};
			case 127:
				return {{1000.0, -321.9949951}};
			case 128:
				return {{1000.0, -322.4465332}};
			case 129:
				return {{1000.0, -322.8072510}};
			case 130:
				return {{1000.0, -327.5804443}};
			case 131:
				return {{1000.0, -332.0192871}};
			case 132:
				return {{1000.0, -332.1802979}};
			case 133:
				return {{1000.0, -336.0573730}};
			case 134:
				return {{1000.0, -336.1817627}};
			case 135:
				return {{1000.0, -340.1765137}};
			case 136:
				return {{1000.0, -340.2766113}};
			case 137:
				return {{1000.0, -343.6551514}};
			case 138:
				return {{1000.0, -346.5415039}};
			case 139:
				return {{1000.0, -349.7819824}};
			case 140:
				return {{1000.0, -349.7995605}};
			case 141:
				return {{1000.0, -352.2539062}};
			case 142:
				return {{1000.0, -352.4711914}};
			case 143:
				return {{1000.0, -354.5693359}};
			case 144:
				return {{1000.0, -354.7683105}};
			case 145:
				return {{1000.0, -356.8011475}};
			case 146:
				return {{1000.0, -356.9757080}};
			case 147:
				return {{1000.0, -358.6204834}};
			case 148:
				return {{1000.0, -358.7729492}};
			case 149:
				return {{1000.0, -360.5307617}};
			case 150:
				return {{1000.0, -360.4355469}};
			case 151:
				return {{1000.0, -361.8718262}};
			case 152:
				return {{1000.0, -361.7562256}};
			case 153:
				return {{1000.0, -363.0778809}};
			case 154:
				return {{1000.0, -363.1750488}};
			case 155:
				return {{1000.0, -364.0666504}};
			case 156:
				return {{1000.0, -364.1520996}};
			case 157:
				return {{1000.0, -364.8118896}};
			case 158:
				return {{1000.0, -365.1131592}};
			case 159:
				return {{1000.0, -365.6785889}};
			case 160:
				return {{1000.0, -365.7379150}};
			case 161:
				return {{1000.0, -366.8975830}};
			case 162:
				return {{1000.0, -367.1383057}};
			case 163:
				return {{1000.0, -367.1704102}};
			case 164:
				return {{1000.0, -367.5058594}};
			case 165:
				return {{1000.0, -367.5330811}};
			case 166:
				return {{1000.0, -367.8172607}};
			case 167:
				return {{1000.0, -367.8402100}};
			case 168:
				return {{1000.0, -368.0611572}};
			case 169:
				return {{1000.0, -368.0804443}};
			case 170:
				return {{1000.0, -368.2398682}};
			case 171:
				return {{1000.0, -368.2490234}};
			case 172:
				return {{1000.0, -368.3774414}};
			case 173:
				return {{1000.0, -368.1584473}};
			case 174:
				return {{1000.0, -368.4852295}};
			case 175:
				return {{1000.0, -368.4898682}};
			case 176:
				return {{1000.0, -368.5522461}};
			case 177:
				return {{1000.0, -368.5556641}};
			case 178:
				return {{1000.0, -368.6008301}};
			case 179:
				return {{1000.0, -368.6030273}};
			case 180:
				return {{1000.0, -368.6293945}};
			case 181:
				return {{1000.0, -368.6307373}};
			case 182:
				return {{1000.0, -368.6463623}};
			case 183:
				return {{1000.0, -368.6523438}};
			case 184:
				return {{1000.0, -368.6516113}};
			case 185:
				return {{1000.0, -368.6513672}};
			case 186:
				return {{1000.0, -368.6455078}};
			case 187:
				return {{1000.0, -368.6450195}};
			case 188:
				return {{1000.0, -368.6356201}};
			case 189:
				return {{1000.0, -368.6350098}};
			case 190:
				return {{1000.0, -368.6240234}};
			case 191:
				return {{1000.0, -368.6234131}};
			case 192:
				return {{1000.0, -368.6105957}};
			case 193:
				return {{1000.0, -368.6097412}};
			case 194:
				return {{1000.0, -368.5974121}};
			case 195:
				return {{1000.0, -368.5965576}};
			case 196:
				return {{1000.0, -368.5834961}};
			case 197:
				return {{1000.0, -368.5827637}};
			case 198:
				return {{1000.0, -368.5701904}};
			case 199:
				return {{1000.0, -368.5695801}};
			case 200:
				return {{1000.0, -368.5584717}};
			case 201:
				return {{1000.0, -368.5578613}};
			case 202:
				return {{1000.0, -368.5469971}};
			case 203:
				return {{1000.0, -368.5463867}};
			case 204:
				return {{999.3082275, -368.5368652}};
			case 205:
				return {{999.1926270, -368.5362549}};
			case 206:
				return {{992.9683228, -368.2961426}};
			case 207:
				return {{987.0891113, -368.2918701}};
			case 208:
				return {{986.6282349, -368.2917480}};
			case 209:
				return {{986.1674805, -368.2916260}};
			case 210:
				return {{976.4907227, -367.6065674}};
			case 211:
				return {{975.7992554, -367.6065674}};
			case 212:
				return {{963.0244751, -366.7009277}};
			case 213:
				return {{962.2189941, -366.7012939}};
			case 214:
				return {{947.3830566, -365.5678711}};
			case 215:
				return {{946.3472290, -365.5686035}};
			case 216:
				return {{927.8547974, -363.7524414}};
			case 217:
				return {{926.5888672, -363.7534180}};
			case 218:
				return {{905.4747314, -361.7132568}};
			case 219:
				return {{903.9788208, -361.4824219}};
			case 220:
				return {{881.9891968, -358.7617188}};
			case 221:
				return {{880.4957886, -358.7635498}};
			case 222:
				return {{854.9809570, -355.3597412}};
			case 223:
				return {{853.3739014, -355.1298828}};
			case 224:
				return {{827.7839966, -351.2725830}};
			case 225:
				return {{826.0632324, -351.0434570}};
			case 226:
				return {{797.3037109, -346.0441895}};
			case 227:
				return {{795.3536377, -345.8156738}};
			case 228:
				return {{765.2640381, -340.1381836}};
			case 229:
				return {{763.4318848, -339.9105225}};
			case 230:
				return {{702.4551392, -326.9423828}};
			case 231:
				return {{670.3325195, -319.2030029}};
			case 232:
				return {{668.2731934, -318.7460938}};
			case 233:
				return {{666.2139893, -318.2890625}};
			case 234:
				return {{635.3792725, -310.0999756}};
			case 235:
				return {{602.1131592, -300.5296631}};
			case 236:
				return {{599.9405518, -299.8432617}};
			case 237:
				return {{568.4085693, -290.0509033}};
			case 238:
				return {{566.3522949, -289.3652344}};
			case 239:
				return {{532.5555420, -277.9793701}};
			case 240:
				return {{530.3845215, -277.2945557}};
			case 241:
				return {{496.5329285, -265.2105713}};
			case 242:
				return {{494.3622131, -264.2956543}};
			case 243:
				return {{462.6911621, -251.9853516}};
			case 244:
				return {{460.5206909, -251.0710449}};
			case 245:
				return {{426.9188843, -236.9365234}};
			case 246:
				return {{424.7486572, -236.0229492}};
			case 247:
				return {{391.3856812, -220.9793701}};
			case 248:
				return {{389.2156372, -220.0665283}};
			case 249:
				return {{358.1632690, -205.2249756}};
			case 250:
				return {{356.1083374, -204.3127441}};
			case 251:
				return {{323.2282715, -187.6450195}};
			case 252:
				return {{321.1730957, -186.5025635}};
			case 253:
				return {{319.1179199, -185.3602295}};
			case 254:
				return {{256.9004517, -150.8436279}};
			case 255:
				return {{225.7268066, -132.0753174}};
			case 256:
				return {{223.7853394, -130.9339600}};
			case 257:
				return {{194.8928223, -112.4333496}};
			case 258:
				return {{193.0664673, -111.0617676}};
			case 259:
				return {{191.1247559, -109.6903076}};
			case 260:
				return {{160.6202393, -89.3165283}};
			case 261:
				return {{158.7932739, -87.9454346}};
			case 262:
				return {{128.9831543, -66.2424316}};
			case 263:
				return {{127.1558838, -64.8718262}};
			case 264:
				return {{99.6079102, -44.2603760}};
			case 265:
				return {{70.8208008, -20.9542236}};
			case 266:
				return {{69.1079712, -19.5843506}};
			case 267:
				return {{42.6389275, 2.7700644}};
			case 268:
				return {{40.7811356, 4.3410416}};
			case 269:
				return {{13.3643608, 28.7596588}};
			case 270:
				return {{11.6500902, 30.3306351}};
			case 271:
				return {{-15.0815430, 55.8112793}};
			case 272:
				return {{-16.7956543, 57.4106445}};
			case 273:
				return {{-41.5140991, 81.7022705}};
			case 274:
				return {{-43.1134033, 83.3012695}};
			case 275:
				return {{-44.7127686, 84.9002686}};
			case 276:
				return {{-70.0754395, 111.6353149}};
			case 277:
				return {{-71.6752319, 113.2340088}};
			case 278:
				return {{-73.2752075, 115.0633545}};
			case 279:
				return {{-97.9533081, 142.4837036}};
			case 280:
				return {{-120.3455200, 168.7619629}};
			case 281:
				return {{-143.6529541, 197.3250122}};
			case 282:
				return {{-145.0234375, 199.1533203}};
			case 283:
				return {{-146.5092163, 200.9815674}};
			case 284:
				return {{-167.5308838, 228.1716919}};
			case 285:
				return {{-168.9017944, 229.9996338}};
			case 286:
				return {{-170.2727051, 231.8275146}};
			case 287:
				return {{-191.9808960, 261.7628174}};
			case 288:
				return {{-193.4182129, 263.4625854}};
			case 289:
				return {{-194.7897949, 265.2904053}};
			case 290:
				return {{-215.5866089, 295.6859741}};
			case 291:
				return {{-216.8434448, 297.7443237}};
			case 292:
				return {{-218.1001587, 299.5717773}};
			case 293:
				return {{-236.8405762, 328.5961304}};
			case 294:
				return {{-237.9824829, 330.4232178}};
			case 295:
				return {{-257.1801758, 361.9624634}};
			case 296:
				return {{-258.3226318, 364.0202026}};
			case 297:
				return {{-259.4650269, 365.8469238}};
			case 298:
				return {{-277.6331177, 397.8424683}};
			case 299:
				return {{-278.7761841, 399.8998413}};
			case 300:
				return {{-279.9191284, 401.9572144}};
			case 301:
				return {{-296.1450806, 432.3524170}};
			case 302:
				return {{-297.1731567, 434.4095459}};
			case 303:
				return {{-313.5977783, 466.5361328}};
			case 304:
				return {{-314.5114136, 468.3631592}};
			case 305:
				return {{-315.5404053, 470.4210205}};
			case 306:
				return {{-329.5396118, 499.2656860}};
			case 307:
				return {{-330.3389282, 501.3251953}};
			case 308:
				return {{-343.8623657, 530.3310547}};
			case 309:
				return {{-356.2064819, 557.1384277}};
			case 310:
				return {{-356.8928833, 558.7409058}};
			case 311:
				return {{-367.0788574, 581.4656372}};
			case 312:
				return {{-367.5354004, 583.0708618}};
			case 313:
				return {{-368.1074829, 584.4446411}};
			case 314:
				return {{-377.6011963, 605.9844360}};
			case 315:
				return {{-378.0588989, 607.3612061}};
			case 316:
				return {{-378.7265625, 608.6624756}};
			case 317:
				return {{-386.8627930, 627.4929199}};
			case 318:
				return {{-387.2059326, 628.6409912}};
			case 319:
				return {{-393.6521606, 644.1310425}};
			case 320:
				return {{-393.9962769, 645.2814331}};
			case 321:
				return {{-394.4370728, 646.1298828}};
			case 322:
				return {{-400.2049561, 660.2677002}};
			case 323:
				return {{-400.5500488, 661.1884766}};
			case 324:
				return {{-400.7789307, 662.1092529}};
			case 325:
				return {{-405.4067993, 673.2781372}};
			case 326:
				return {{-405.6364136, 673.9683838}};
			case 327:
				return {{-405.8662109, 674.6588135}};
			case 328:
				return {{-409.8261719, 684.7592163}};
			case 329:
				return {{-410.0563965, 685.2185059}};
			case 330:
				return {{-410.1705933, 685.9105225}};
			case 331:
				return {{-413.5615845, 694.1802979}};
			case 332:
				return {{-413.6761475, 694.6409912}};
			case 333:
				return {{-413.7907715, 695.1016846}};
			case 334:
				return {{-416.1781006, 701.4295654}};
			case 335:
				return {{-416.3936157, 701.8295898}};
			case 336:
				return {{-416.5086060, 702.2913208}};
			case 337:
				return {{-418.5523682, 707.7015381}};
			case 338:
				return {{-418.6676025, 708.1641235}};
			case 339:
				return {{-418.7828979, 708.3939819}};
			case 340:
				return {{-420.2655640, 712.7144165}};
			case 341:
				return {{-420.2647705, 712.9449463}};
			case 342:
				return {{-421.4025879, 716.3460083}};
			case 343:
				return {{-422.4263306, 719.0538940}};
			case 344:
				return {{-423.2201538, 721.3039551}};
			case 345:
				return {{-423.2197876, 721.3027344}};
			case 346:
				return {{-423.2194824, 721.5345459}};
			case 347:
				return {{-423.6802368, 723.1488037}};
			case 348:
				return {{-423.7963867, 723.1478271}};
			case 349:
				return {{-424.0260010, 724.5333252}};
			case 350:
				return {{-424.1422729, 724.5325928}};
			case 351:
				return {{-424.1420898, 724.5319824}};
			case 352:
				return {{-424.3728638, 725.4558105}};
			case 353:
				return {{-424.3727417, 725.4553223}};
			case 354:
				return {{-424.4877930, 726.1480713}};
			case 355:
				return {{-424.4877319, 726.1477051}};
			case 356:
				return {{-424.4876709, 726.1473389}};
			case 357:
				return {{-424.6035156, 726.6093750}};
			case 358:
				return {{-424.6035156, 726.6091309}};
			case 359:
				return {{-424.7199097, 726.8399658}};
			case 360:
				return {{-424.7199097, 726.8398438}};
			case 361:
				return {{-424.7199097, 726.8397217}};
			case 362:
				return {{-424.5017700, 726.8966064}};
			case 363:
				return {{-424.5017700, 726.8966064}};
			case 364:
				return {{-424.5017700, 726.8966064}};
			case 365:
				return {{-424.5017700, 726.8966064}};
			case 366:
				return {{-424.5022583, 726.8964844}};
			case 367:
				return {{-424.5023193, 726.8964844}};
			case 368:
				return {{-424.5029907, 726.8969727}};
			case 369:
				return {{-424.5030518, 726.8969727}};
			case 370:
				return {{-424.5037231, 726.6649170}};
			case 371:
				return {{-424.5037842, 726.6650391}};
			case 372:
				return {{-424.5038452, 726.6651611}};
			case 373:
				return {{-424.1698608, 726.7243652}};
			case 374:
				return {{-424.1706543, 726.4927979}};
			case 375:
				return {{-424.1714478, 726.4942627}};
			case 376:
				return {{-424.1714478, 726.2613525}};
			case 377:
				return {{-424.1715088, 726.2614746}};
			case 378:
				return {{-424.0557251, 726.2628174}};
			case 379:
				return {{-424.0557861, 726.2629395}};
			case 380:
				return {{-424.0558472, 726.2630615}};
			case 381:
				return {{-424.0565186, 726.0314331}};
			case 382:
				return {{-424.0565796, 726.0315552}};
			case 383:
				return {{-424.0566406, 726.0316772}};
			case 384:
				return {{-424.0571899, 726.0328979}};
			case 385:
				return {{-424.0571899, 725.8000488}};
			case 386:
				return {{-424.0572510, 725.8001709}};
			case 387:
				return {{-423.9413452, 725.8013916}};
			case 388:
				return {{-423.9414062, 725.8015137}};
			case 389:
				return {{-423.9418945, 725.5696411}};
			case 390:
				return {{-423.9418945, 725.5697632}};
			case 391:
				return {{-423.9418945, 725.5698853}};
			case 392:
				return {{-423.7241211, 725.6289062}};
			case 393:
				return {{-423.7241211, 725.6289062}};
			case 394:
				return {{-423.7241211, 725.6289062}};
			case 395:
				return {{-423.6081543, 725.6297607}};
			case 396:
				return {{-423.6081543, 725.6297607}};
			case 397:
				return {{-423.6081543, 725.6297607}};
			case 398:
				return {{-423.6083984, 725.3975220}};
			case 399:
				return {{-423.6083984, 725.3975220}};
			case 400:
				return {{-423.6086426, 725.3981323}};
			case 401:
				return {{-423.6086426, 725.3981323}};
			case 402:
				return {{-423.6086426, 725.3981323}};
			case 403:
				return {{-423.6088257, 725.3986206}};
			case 404:
				return {{-423.6088257, 725.3986206}};
			case 405:
				return {{-423.6088257, 725.3986206}};
			case 406:
				return {{-423.6090088, 725.1660156}};
			case 407:
				return {{-423.6091309, 725.1663818}};
			case 408:
				return {{-423.6092529, 725.1667480}};
			case 409:
				return {{-423.6092529, 725.1667480}};
			case 410:
				return {{-423.6092529, 725.1667480}};
			case 411:
				return {{-423.6093140, 725.1669922}};
			case 412:
				return {{-423.6093140, 725.1669922}};
			case 413:
				return {{-423.6093140, 725.1669922}};
			case 414:
				return {{-423.6093750, 725.1672363}};
			case 415:
				return {{-423.6093750, 725.1672363}};
			case 416:
				return {{-423.6093750, 725.1672363}};
			case 417:
				return {{-423.6094360, 725.1673584}};
			case 418:
				return {{-423.6094360, 725.1673584}};
			case 419:
				return {{-423.6094360, 725.1673584}};
			case 420:
				return {{-423.6094971, 725.1674805}};
			case 421:
				return {{-423.6094971, 725.1674805}};
			case 422:
				return {{-423.6095581, 725.1676025}};
			case 423:
				return {{-423.6095581, 725.1676025}};
			case 424:
				return {{-423.6095581, 725.1676025}};
			case 425:
				return {{-423.6095581, 725.1677246}};
			case 426:
				return {{-423.6095581, 725.1677246}};
			case 427:
				return {{-423.6095581, 725.1677246}};
			case 428:
				return {{-423.6095581, 725.1677246}};
			case 429:
				return {{-423.6095581, 725.1677246}};
			case 430:
				return {{-423.6095581, 725.1677246}};
			case 431:
				return {{-423.4931030, 725.1677246}};
			case 432:
				return {{-423.4931030, 725.1677246}};
			case 433:
				return {{-423.4931030, 725.1677246}};
			case 434:
				return {{-423.4931030, 725.1677246}};
			case 435:
				return {{-423.4931030, 725.1677246}};
			case 436:
				return {{-423.4931030, 725.1677246}};
			case 437:
				return {{-423.4931030, 725.1677246}};
			case 438:
				return {{-423.4931030, 725.1677246}};
			case 439:
				return {{-423.4931030, 725.1677246}};
			case 440:
				return {{-423.4931030, 725.1677246}};
			case 441:
				return {{-423.4931030, 725.1677246}};
			case 442:
				return {{-423.4931030, 725.1677246}};
			case 443:
				return {{-423.4931030, 725.1677246}};
			case 444:
				return {{-423.4931030, 725.1677246}};
			case 445:
				return {{-423.4931030, 725.1677246}};
			case 446:
				return {{-423.4931030, 725.1677246}};
			case 447:
				return {{-423.4931030, 725.1677246}};
			case 448:
				return {{-423.4931030, 725.1677246}};
			case 449:
				return {{-423.4931030, 725.1677246}};
			case 450:
				return {{-423.4931030, 725.1677246}};
			case 451:
				return {{-423.4931030, 725.1677246}};
			case 452:
				return {{-423.4931030, 725.1677246}};
			case 453:
				return {{-423.4931030, 725.1677246}};
			case 454:
				return {{-423.4931030, 725.1677246}};
			case 455:
				return {{-423.4931030, 725.1677246}};
			case 456:
				return {{-423.4931030, 725.1677246}};
			case 457:
				return {{-423.4931030, 725.1677246}};
			case 458:
				return {{-423.4931030, 725.1677246}};
			case 459:
				return {{-423.4931030, 725.1677246}};
			case 460:
				return {{-423.4931030, 725.1677246}};
			case 461:
				return {{-423.4931030, 725.1677246}};
			case 462:
				return {{-423.4931030, 725.1677246}};
			case 463:
				return {{-423.4931030, 725.1677246}};
			case 464:
				return {{-423.4931030, 725.1677246}};
			case 465:
				return {{-423.4931030, 725.1677246}};
			case 466:
				return {{-423.4931030, 725.1677246}};
			case 467:
				return {{-423.4931030, 725.1677246}};
			case 468:
				return {{-423.4931030, 725.1677246}};
			case 469:
				return {{-423.4931030, 725.1677246}};
			case 470:
				return {{-423.4931030, 725.1677246}};
			case 471:
				return {{-423.4931030, 725.1677246}};
			case 472:
				return {{-423.4931030, 725.1677246}};
			case 473:
				return {{-423.4931030, 725.1677246}};
			case 474:
				return {{-423.4931030, 725.1677246}};
			case 475:
				return {{-423.4931030, 725.1677246}};
			case 476:
				return {{-423.4931030, 725.1677246}};
			case 477:
				return {{-423.4931030, 725.1677246}};
			case 478:
				return {{-423.4931030, 725.1677246}};
			case 479:
				return {{-423.4931030, 725.1677246}};
			case 480:
				return {{-423.4931030, 725.1677246}};
			case 481:
				return {{-423.4931030, 725.1677246}};
			case 482:
				return {{-423.4931030, 725.1677246}};
			case 483:
				return {{-423.4931030, 725.1677246}};
			case 484:
				return {{-423.4931030, 725.1677246}};
			case 485:
				return {{-423.4931030, 725.1677246}};
			case 486:
				return {{-423.4931030, 725.1677246}};
			case 487:
				return {{-423.4931030, 725.1677246}};
			case 488:
				return {{-423.4931030, 725.1677246}};
			case 489:
				return {{-423.4931030, 725.1677246}};
			case 490:
				return {{-423.4931030, 725.1677246}};
			case 491:
				return {{-423.4931030, 725.1677246}};
			case 492:
				return {{-423.4931030, 725.1677246}};
			case 493:
				return {{-423.4931030, 725.1677246}};
			case 494:
				return {{-423.4931030, 725.1677246}};
			case 495:
				return {{-423.4931030, 725.1677246}};
			case 496:
				return {{-423.4931030, 725.1677246}};
			case 497:
				return {{-423.4931030, 725.1677246}};
			case 498:
				return {{-423.4931030, 725.1677246}};
			case 499:
				return {{-423.4931030, 725.1677246}};
			case 500:
				return {{-423.6095581, 725.1677246}};
			case 501:
				return {{-423.6095581, 725.1677246}};
			case 502:
				return {{-423.6095581, 725.1677246}};
			case 503:
				return {{-423.4931641, 725.1680908}};
			case 504:
				return {{-423.0333252, 723.0928955}};
			case 505:
				return {{-422.9171753, 722.8612671}};
			case 506:
				return {{-422.9176025, 722.6296387}};
			case 507:
				return {{-422.9180298, 722.6311035}};
			case 508:
				return {{-421.8814697, 718.7099609}};
			case 509:
				return {{-421.7659302, 718.4803467}};
			case 510:
				return {{-419.4887085, 711.9137573}};
			case 511:
				return {{-419.3739624, 711.2202759}};
			case 512:
				return {{-416.6250610, 702.2918701}};
			case 513:
				return {{-416.3945923, 701.6003418}};
			case 514:
				return {{-416.1640015, 700.9089355}};
			case 515:
				return {{-415.7167969, 700.2796021}};
			case 516:
				return {{-411.6039429, 687.2690430}};
			case 517:
				return {{-411.2580566, 686.5798340}};
			case 518:
				return {{-405.6826172, 670.9466553}};
			case 519:
				return {{-405.3376465, 669.7944946}};
			case 520:
				return {{-404.9926147, 668.8750000}};
			case 521:
				return {{-398.6190796, 651.6506348}};
			case 522:
				return {{-398.2750244, 650.5007324}};
			case 523:
				return {{-397.8148804, 649.3509521}};
			case 524:
				return {{-389.7442017, 628.7731934}};
			case 525:
				return {{-389.2850952, 627.3934326}};
			case 526:
				return {{-388.8259888, 626.0136719}};
			case 527:
				return {{-380.1945190, 604.7800293}};
			case 528:
				return {{-379.6205444, 603.1703491}};
			case 529:
				return {{-378.8359375, 601.6376953}};
			case 530:
				return {{-368.7077637, 577.1479492}};
			case 531:
				return {{-367.8103027, 575.6213989}};
			case 532:
				return {{-367.2375488, 574.0144653}};
			case 533:
				return {{-355.6552124, 548.0385742}};
			case 534:
				return {{-354.9677124, 546.4335327}};
			case 535:
				return {{-354.2803955, 544.5969849}};
			case 536:
				return {{-328.7350464, 491.3011475}};
			case 537:
				return {{-327.9339600, 489.4679565}};
			case 538:
				return {{-313.5975342, 461.1282959}};
			case 539:
				return {{-312.7975464, 459.2963867}};
			case 540:
				return {{-311.6823730, 457.5629883}};
			case 541:
				return {{-297.1329346, 430.1679688}};
			case 542:
				return {{-296.1030273, 428.3374634}};
			case 543:
				return {{-295.1887207, 426.5070190}};
			case 544:
				return {{-278.4797363, 397.0824585}};
			case 545:
				return {{-277.4510498, 395.2529907}};
			case 546:
				return {{-276.4223022, 393.1925049}};
			case 547:
				return {{-259.7289429, 365.3966675}};
			case 548:
				return {{-258.5857544, 363.5681152}};
			case 549:
				return {{-257.4428101, 361.7396240}};
			case 550:
				return {{-238.7824707, 331.7955933}};
			case 551:
				return {{-237.6401978, 329.7368164}};
			case 552:
				return {{-217.9624634, 300.0416260}};
			case 553:
				return {{-216.7058105, 298.2141113}};
			case 554:
				return {{-197.5637207, 270.4667358}};
			case 555:
				return {{-196.0111084, 268.7662354}};
			case 556:
				return {{-194.7553101, 266.7084351}};
			case 557:
				return {{-173.3378296, 237.3753052}};
			case 558:
				return {{-171.9674683, 235.5482178}};
			case 559:
				return {{-170.5972900, 233.7211914}};
			case 560:
				return {{-148.4472046, 204.7180786}};
			case 561:
				return {{-146.9035034, 203.0276489}};
			case 562:
				return {{-145.5340576, 201.4312744}};
			case 563:
				return {{-123.8424683, 174.7163086}};
			case 564:
				return {{-122.3580322, 172.8890381}};
			case 565:
				return {{-120.9890137, 171.0617676}};
			case 566:
				return {{-97.0138550, 143.2062988}};
			case 567:
				return {{-95.5299683, 141.3786621}};
			case 568:
				return {{-72.4690552, 115.8064575}};
			case 569:
				return {{-47.1237183, 88.8619385}};
			case 570:
				return {{-21.0932007, 62.8336182}};
			case 571:
				return {{-19.4952393, 61.2354736}};
			case 572:
				return {{-17.7821045, 59.6373291}};
			case 573:
				return {{7.0523272, 35.4723663}};
			case 574:
				return {{8.6218185, 34.0455475}};
			case 575:
				return {{10.4785328, 32.4759102}};
			case 576:
				return {{37.7422180, 7.6363873}};
			case 577:
				return {{39.4556503, 6.0661173}};
			case 578:
				return {{41.1685028, 4.7833538}};
			case 579:
				return {{69.2730103, -19.2476807}};
			case 580:
				return {{71.1008301, -20.6170654}};
			case 581:
				return {{72.8132324, -22.2172852}};
			case 582:
				return {{99.7648926, -43.6842041}};
			case 583:
				return {{101.5922852, -45.2849121}};
			case 584:
				return {{130.8104858, -67.6149902}};
			case 585:
				return {{132.6378174, -68.9855957}};
			case 586:
				return {{134.4650879, -70.3562012}};
			case 587:
				return {{162.5634155, -90.6878662}};
			case 588:
				return {{164.5056763, -91.8281250}};
			case 589:
				return {{166.3325806, -93.1992188}};
			case 590:
				return {{196.8288574, -113.7564697}};
			case 591:
				return {{198.6491089, -115.3134766}};
			case 592:
				return {{200.5913086, -116.6853027}};
			case 593:
				return {{231.6635742, -136.3317871}};
			case 594:
				return {{233.6054077, -137.4732666}};
			case 595:
				return {{235.5473022, -138.8458252}};
			case 596:
				return {{265.1340942, -156.4359131}};
			case 597:
				return {{267.0756836, -157.5781250}};
			case 598:
				return {{269.1325684, -158.7203369}};
			case 599:
				return {{301.1267700, -176.9635010}};
			case 600:
				return {{303.0683594, -178.1064453}};
			case 601:
				return {{366.2507935, -210.3223877}};
			case 602:
				return {{368.2116699, -211.6663818}};
			case 603:
				return {{399.7741089, -226.4897461}};
			case 604:
				return {{401.6263428, -227.6066895}};
			case 605:
				return {{429.2578125, -240.3281250}};
			case 606:
				return {{431.0879517, -241.2440186}};
			case 607:
				return {{432.8026733, -241.9289551}};
			case 608:
				return {{459.8411865, -254.1583252}};
			case 609:
				return {{461.4432373, -254.8446045}};
			case 610:
				return {{485.8117676, -265.6970215}};
			case 611:
				return {{487.1856079, -266.1535645}};
			case 612:
				return {{507.6619263, -274.7510986}};
			case 613:
				return {{508.9636230, -275.4190674}};
			case 614:
				return {{528.1407471, -283.5548096}};
			case 615:
				return {{529.2885132, -284.0137939}};
			case 616:
				return {{530.4364014, -284.2408447}};
			case 617:
				return {{547.0093994, -291.2448730}};
			case 618:
				return {{548.0438232, -291.4729004}};
			case 619:
				return {{548.8934326, -292.1464844}};
			case 620:
				return {{562.2331543, -297.3398438}};
			case 621:
				return {{563.0377197, -297.8010254}};
			case 622:
				return {{575.1970825, -302.3284912}};
			case 623:
				return {{575.8223877, -302.7731934}};
			case 624:
				return {{576.6289062, -303.0029297}};
			case 625:
				return {{586.0415039, -306.8507080}};
			case 626:
				return {{586.7333374, -306.8483887}};
			case 627:
				return {{587.3089600, -307.0787354}};
			case 628:
				return {{602.7194824, -312.9729004}};
			case 629:
				return {{603.0646362, -312.9714355}};
			case 630:
				return {{608.4770508, -315.0159912}};
			case 631:
				return {{608.8231201, -315.0147705}};
			case 632:
				return {{609.1691895, -315.2463379}};
			case 633:
				return {{613.6613159, -316.8295898}};
			case 634:
				return {{614.0081177, -317.0616455}};
			case 635:
				return {{617.6392822, -318.1986084}};
			case 636:
				return {{617.8701172, -318.1978760}};
			case 637:
				return {{618.1010742, -318.1971436}};
			case 638:
				return {{620.6949463, -319.3382568}};
			case 639:
				return {{620.9263916, -319.3377686}};
			case 640:
				return {{621.0412598, -319.3372803}};
			case 641:
				return {{623.1748047, -320.0146484}};
			case 642:
				return {{623.2900391, -320.2473145}};
			case 643:
				return {{624.9053955, -320.4754639}};
			case 644:
				return {{625.0209961, -320.7082520}};
			case 645:
				return {{625.1365967, -320.7080078}};
			case 646:
				return {{626.2893066, -320.9375000}};
			case 647:
				return {{626.4051514, -320.9373779}};
			case 648:
				return {{626.4044189, -320.9372559}};
			case 649:
				return {{627.3277588, -321.1679688}};
			case 650:
				return {{627.3272705, -321.1678467}};
			case 651:
				return {{627.9041748, -321.3996582}};
			case 652:
				return {{627.9038086, -321.3995361}};
			case 653:
				return {{628.0200806, -321.3995361}};
			case 654:
				return {{628.3657227, -321.3989258}};
			case 655:
				return {{628.3655396, -321.3989258}};
			case 656:
				return {{628.5963135, -321.6318359}};
			case 657:
				return {{628.5961914, -321.6318359}};
			case 658:
				return {{628.7686768, -321.4134521}};
			case 659:
				return {{628.7684937, -321.4139404}};
			case 660:
				return {{628.6523438, -321.4145508}};
			case 661:
				return {{628.6524048, -321.4145508}};
			case 662:
				return {{628.6524658, -321.4145508}};
			case 663:
				return {{628.5368652, -321.4154053}};
			case 664:
				return {{628.5369873, -321.1822510}};
			case 665:
				return {{628.4216309, -321.1829834}};
			case 666:
				return {{628.4217529, -321.1829834}};
			case 667:
				return {{628.4217529, -321.1829834}};
			case 668:
				return {{628.3635254, -320.9653320}};
			case 669:
				return {{628.3635254, -320.9653320}};
			case 670:
				return {{628.3636475, -320.9653320}};
			case 671:
				return {{628.1320801, -320.9660645}};
			case 672:
				return {{628.1322021, -320.9660645}};
			case 673:
				return {{628.0172119, -320.9667969}};
			case 674:
				return {{628.0172729, -320.9667969}};
			case 675:
				return {{628.0173340, -320.9667969}};
			case 676:
				return {{627.9022217, -320.9674072}};
			case 677:
				return {{627.9022217, -320.9674072}};
			case 678:
				return {{627.7858276, -320.9674072}};
			case 679:
				return {{627.6706543, -320.7349854}};
			case 680:
				return {{627.6707764, -320.7349854}};
			case 681:
				return {{627.5554199, -320.7355957}};
			case 682:
				return {{627.5554810, -320.7355957}};
			case 683:
				return {{627.5555420, -320.7355957}};
			case 684:
				return {{627.4401855, -320.7360840}};
			case 685:
				return {{627.4401855, -320.7360840}};
			case 686:
				return {{627.3817749, -320.5181885}};
			case 687:
				return {{627.3818359, -320.5181885}};
			case 688:
				return {{627.2661743, -320.5185547}};
			case 689:
				return {{627.2669678, -320.5189209}};
			case 690:
				return {{627.1511230, -320.5192871}};
			case 691:
				return {{627.1512451, -320.5192871}};
			case 692:
				return {{627.1512451, -320.5192871}};
			case 693:
				return {{627.1518555, -320.5195312}};
			case 694:
				return {{627.1518555, -320.5195312}};
			case 695:
				return {{627.0358887, -320.5197754}};
			case 696:
				return {{627.0358887, -320.5197754}};
			case 697:
				return {{627.0358887, -320.5197754}};
			case 698:
				return {{627.0362549, -320.5198975}};
			case 699:
				return {{627.0362549, -320.5198975}};
			case 700:
				return {{626.9201050, -320.5200195}};
			case 701:
				return {{626.9201050, -320.5200195}};
			case 702:
				return {{626.9203491, -320.5201416}};
			case 703:
				return {{626.9203491, -320.5201416}};
			case 704:
				return {{626.9203491, -320.5201416}};
			case 705:
				return {{626.9205322, -320.5202637}};
			case 706:
				return {{626.9205322, -320.5202637}};
			case 707:
				return {{626.9205322, -320.5202637}};
			case 708:
				return {{626.9207153, -320.2874756}};
			case 709:
				return {{626.9207153, -320.2874756}};
			case 710:
				return {{626.9208374, -320.2875977}};
			case 711:
				return {{626.9208374, -320.2875977}};
			case 712:
				return {{626.9208374, -320.2875977}};
			case 713:
				return {{626.9209595, -320.2877197}};
			case 714:
				return {{626.9209595, -320.2877197}};
			case 715:
				return {{626.9209595, -320.2877197}};
			case 716:
				return {{626.9210815, -320.2877197}};
			case 717:
				return {{626.9210815, -320.2877197}};
			case 718:
				return {{626.9211426, -320.2877197}};
			case 719:
				return {{626.8047485, -320.2877197}};
			case 720:
				return {{626.8048096, -320.2877197}};
			case 721:
				return {{626.8048096, -320.2877197}};
			case 722:
				return {{626.8048096, -320.2877197}};
			case 723:
				return {{626.8048706, -320.2877197}};
			case 724:
				return {{626.8048706, -320.2877197}};
			case 725:
				return {{626.8048706, -320.2877197}};
			case 726:
				return {{626.8048706, -320.2877197}};
			case 727:
				return {{626.8048706, -320.2877197}};
			case 728:
				return {{626.8048706, -320.2877197}};
			case 729:
				return {{626.8048706, -320.2877197}};
			case 730:
				return {{626.8048706, -320.2877197}};
			case 731:
				return {{626.8048706, -320.2877197}};
			case 732:
				return {{626.8048706, -320.2877197}};
			case 733:
				return {{626.8048706, -320.2877197}};
			case 734:
				return {{626.8048706, -320.2877197}};
			case 735:
				return {{626.8048706, -320.2877197}};
			case 736:
				return {{626.8048706, -320.2877197}};
			case 737:
				return {{626.8048706, -320.2877197}};
			case 738:
				return {{626.8048706, -320.2877197}};
			case 739:
				return {{626.8048706, -320.2877197}};
			case 740:
				return {{626.8048706, -320.2877197}};
			case 741:
				return {{626.8048706, -320.2877197}};
			case 742:
				return {{626.8048706, -320.2877197}};
			case 743:
				return {{626.8048706, -320.2877197}};
			case 744:
				return {{626.8048706, -320.2877197}};
			case 745:
				return {{626.8048706, -320.2877197}};
			case 746:
				return {{626.8048706, -320.2877197}};
			case 747:
				return {{626.8048706, -320.2877197}};
			case 748:
				return {{626.8048706, -320.2877197}};
			case 749:
				return {{626.8048706, -320.2877197}};
			case 750:
				return {{626.8048706, -320.2877197}};
			case 751:
				return {{626.8048706, -320.2877197}};
			case 752:
				return {{626.8048706, -320.2877197}};
			case 753:
				return {{626.8048706, -320.2877197}};
			case 754:
				return {{626.8048706, -320.2877197}};
			case 755:
				return {{626.8048706, -320.2877197}};
			case 756:
				return {{626.8048706, -320.2877197}};
			case 757:
				return {{626.8048706, -320.2877197}};
			case 758:
				return {{626.8048706, -320.2877197}};
			case 759:
				return {{626.8048706, -320.2877197}};
			case 760:
				return {{626.8048706, -320.2877197}};
			case 761:
				return {{626.8048706, -320.2877197}};
			case 762:
				return {{626.8048706, -320.2877197}};
			case 763:
				return {{626.8048706, -320.2877197}};
			case 764:
				return {{626.8048706, -320.2877197}};
			case 765:
				return {{626.8048706, -320.2877197}};
			case 766:
				return {{626.8048706, -320.2877197}};
			case 767:
				return {{626.8048706, -320.2877197}};
			case 768:
				return {{626.8048706, -320.2877197}};
			case 769:
				return {{626.8048706, -320.2877197}};
			case 770:
				return {{626.8048706, -320.2877197}};
			case 771:
				return {{626.8048706, -320.2877197}};
			case 772:
				return {{626.8048706, -320.2877197}};
			case 773:
				return {{626.8048706, -320.2877197}};
			case 774:
				return {{626.8048706, -320.2877197}};
			case 775:
				return {{626.8048706, -320.2877197}};
			case 776:
				return {{626.8048706, -320.2877197}};
			case 777:
				return {{626.8048706, -320.2877197}};
			case 778:
				return {{626.8048706, -320.2877197}};
			case 779:
				return {{626.8048706, -320.2877197}};
			case 780:
				return {{626.8048706, -320.2877197}};
			case 781:
				return {{626.8048706, -320.2877197}};
			case 782:
				return {{626.8048706, -320.2877197}};
			case 783:
				return {{626.8048706, -320.2877197}};
			case 784:
				return {{626.8048706, -320.2877197}};
			case 785:
				return {{626.8048706, -320.2877197}};
			case 786:
				return {{626.8048706, -320.2877197}};
			case 787:
				return {{626.8048706, -320.2877197}};
			case 788:
				return {{626.8048706, -320.2877197}};
			case 789:
				return {{626.8048706, -320.2877197}};
			case 790:
				return {{626.8048706, -320.2877197}};
			case 791:
				return {{626.8048706, -320.2877197}};
			case 792:
				return {{626.8048706, -320.2877197}};
			case 793:
				return {{626.8048706, -320.2877197}};
			case 794:
				return {{626.8048706, -320.2877197}};
			case 795:
				return {{626.8048706, -320.2877197}};
			case 796:
				return {{626.8048706, -320.2877197}};
			case 797:
				return {{626.8048706, -320.2877197}};
			case 798:
				return {{626.8048706, -320.2877197}};
			case 799:
				return {{626.8048706, -320.2877197}};
			case 800:
				return {{626.8048706, -320.2877197}};
			case 801:
				return {{626.8048706, -320.2877197}};
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
				return {{973.3985596, 1091.1276855}};
			case 502:
				return {{971.1582031, 1097.2403564}};
			case 601:
				return {{986.9957886, 1077.9326172}};
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
				return {{993.2854614, 1101.4479980}};
			case 502:
				return {{992.8182373, 1103.0178223}};
			case 601:
				return {{996.7180176, 1098.1177979}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == k_ScreenCentreLine[11])
		{
			switch (frameNumber)
			{
			case 0:
				return {{0.0, -0.9229774}};
			case 202:
				return {{1000.0, 1104.8607178}};
			case 301:
				return {{999.8155518, 1104.8398438}};
			case 502:
				return {{999.9276733, 1104.9146729}};
			case 601:
				return {{999.9104004, 1104.7470703}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == k_ScreenCentreLine[12])
		{
			switch (frameNumber)
			{
			case 0:
				return {{0.0, -4.5307636}};
			case 202:
				return {{1000.0, 1108.6668701}};
			case 301:
				return {{1003.1954956, 1106.5926514}};
			case 502:
				return {{1003.6074219, 1105.8981934}};
			case 601:
				return {{1001.5633545, 1108.1765137}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == k_ScreenCentreLine[13])
		{
			switch (frameNumber)
			{
			case 0:
				return {{0.0, -7.4373074}};
			case 202:
				return {{1000.0, 1110.8963623}};
			case 301:
				return {{1005.1740723, 1107.6207275}};
			case 502:
				return {{1005.7613525, 1106.4724121}};
			case 601:
				return {{1002.5313721, 1110.1845703}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == k_ScreenCentreLine[14])
		{
			switch (frameNumber)
			{
			case 0:
				return {{0.0, -9.9486294}};
			case 202:
				return {{1000.0, 1112.4274902}};
			case 301:
				return {{1006.5321045, 1108.3244629}};
			case 502:
				return {{1007.2391357, 1106.8659668}};
			case 601:
				return {{1003.1950684, 1111.5629883}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == k_ScreenCentreLine[15])
		{
			switch (frameNumber)
			{
			case 0:
				return {{0.0, -12.0339508}};
			case 202:
				return {{1000.0, 1113.4934082}};
			case 301:
				return {{1007.4783325, 1108.8148193}};
			case 502:
				return {{1008.2691040, 1107.1403809}};
			case 601:
				return {{1003.6572266, 1112.5234375}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == k_MockMousePos)
		{
			switch (frameNumber)
			{
			case 0:
				return {{15.8141842, -13.4212151}};
			case 1:
				return {{998.6565552, 1118.0327148}};
			case 2:
				return {{998.6566772, 1118.0330811}};
			case 3:
				return {{998.6575928, 1118.0344238}};
			case 4:
				return {{998.6578369, 1118.0350342}};
			case 5:
				return {{998.6578979, 1118.0350342}};
			case 6:
				return {{998.6578979, 1118.0351562}};
			case 7:
				return {{998.6581421, 1118.0354004}};
			case 8:
				return {{998.6581421, 1118.0354004}};
			case 9:
				return {{998.6583862, 1118.0360107}};
			case 10:
				return {{998.6583862, 1118.0361328}};
			case 11:
				return {{998.6583862, 1118.0361328}};
			case 12:
				return {{998.6585083, 1118.0363770}};
			case 13:
				return {{998.6585083, 1118.0363770}};
			case 14:
				return {{998.6585083, 1118.0363770}};
			case 15:
				return {{998.6585083, 1118.0363770}};
			case 16:
				return {{998.6586914, 1118.0366211}};
			case 17:
				return {{998.6586914, 1118.0366211}};
			case 18:
				return {{998.6588135, 1118.0368652}};
			case 19:
				return {{998.6589355, 1118.0369873}};
			case 20:
				return {{998.6591187, 1118.0371094}};
			case 21:
				return {{998.6591187, 1118.0371094}};
			case 22:
				return {{998.6591187, 1118.0375977}};
			case 23:
				return {{998.6591187, 1118.0375977}};
			case 24:
				return {{998.6593018, 1118.0377197}};
			case 25:
				return {{998.6593018, 1118.0377197}};
			case 26:
				return {{998.6593628, 1118.0378418}};
			case 27:
				return {{998.6593628, 1118.0378418}};
			case 28:
				return {{998.6593628, 1118.0378418}};
			case 29:
				return {{998.6594238, 1118.0379639}};
			case 30:
				return {{998.6594238, 1118.0380859}};
			case 31:
				return {{998.6594238, 1118.0380859}};
			case 32:
				return {{998.6595459, 1118.0380859}};
			case 33:
				return {{998.6595459, 1118.0380859}};
			case 34:
				return {{998.6596069, 1118.0382080}};
			case 35:
				return {{998.6596069, 1118.0382080}};
			case 36:
				return {{998.6596680, 1118.0382080}};
			case 37:
				return {{998.6596680, 1118.0382080}};
			case 38:
				return {{998.6596680, 1118.0382080}};
			case 39:
				return {{998.6050415, 1117.9583740}};
			case 40:
				return {{998.6004639, 1117.9519043}};
			case 41:
				return {{998.5250854, 1117.8411865}};
			case 42:
				return {{998.5202026, 1117.8342285}};
			case 43:
				return {{998.4447632, 1117.7239990}};
			case 44:
				return {{998.3740845, 1117.6204834}};
			case 45:
				return {{998.2989502, 1117.5107422}};
			case 46:
				return {{998.2943726, 1117.5034180}};
			case 47:
				return {{998.2241821, 1117.4010010}};
			case 48:
				return {{998.2192993, 1117.3937988}};
			case 49:
				return {{998.1444702, 1117.2852783}};
			case 50:
				return {{998.1400146, 1117.2783203}};
			case 51:
				return {{998.0661011, 1117.1699219}};
			case 52:
				return {{998.0614624, 1117.1633301}};
			case 53:
				return {{997.9931030, 1117.0632324}};
			case 54:
				return {{997.9883423, 1117.0559082}};
			case 55:
				return {{997.9157104, 1116.9499512}};
			case 56:
				return {{997.9113770, 1116.9432373}};
			case 57:
				return {{997.8395996, 1116.8388672}};
			case 58:
				return {{997.8350830, 1116.8316650}};
			case 59:
				return {{997.7686157, 1116.7346191}};
			case 60:
				return {{997.7645264, 1116.7280273}};
			case 61:
				return {{997.6946411, 1116.6263428}};
			case 62:
				return {{997.6901855, 1116.6201172}};
			case 63:
				return {{997.6260376, 1116.5261230}};
			case 64:
				return {{997.6213379, 1116.5197754}};
			case 65:
				return {{997.5543213, 1116.4202881}};
			case 66:
				return {{997.4876709, 1116.3229980}};
			case 67:
				return {{997.4268799, 1116.2340088}};
			case 68:
				return {{997.4226074, 1116.2282715}};
			case 69:
				return {{997.3584595, 1116.1345215}};
			case 70:
				return {{997.3546753, 1116.1284180}};
			case 71:
				return {{997.2918701, 1116.0371094}};
			case 72:
				return {{997.2882690, 1116.0312500}};
			case 73:
				return {{997.2311401, 1115.9484863}};
			case 74:
				return {{997.2274780, 1115.9427490}};
			case 75:
				return {{997.2233887, 1115.9372559}};
			case 76:
				return {{997.1643066, 1115.8500977}};
			case 77:
				return {{997.1604004, 1115.8443604}};
			case 78:
				return {{997.1060791, 1115.7656250}};
			case 79:
				return {{997.1025391, 1115.7595215}};
			case 80:
				return {{997.0460205, 1115.6776123}};
			case 81:
				return {{997.0431519, 1115.6727295}};
			case 82:
				return {{996.9878540, 1115.5917969}};
			case 83:
				return {{996.9848022, 1115.5877686}};
			case 84:
				return {{996.9347534, 1115.5148926}};
			case 85:
				return {{996.9316406, 1115.5096436}};
			case 86:
				return {{996.8798828, 1115.4344482}};
			case 87:
				return {{996.8767700, 1115.4294434}};
			case 88:
				return {{996.8270874, 1115.3565674}};
			case 89:
				return {{996.8242188, 1115.3520508}};
			case 90:
				return {{996.7781982, 1115.2857666}};
			case 91:
				return {{996.7322388, 1115.2180176}};
			case 92:
				return {{996.6903687, 1115.1567383}};
			case 93:
				return {{996.6872559, 1115.1528320}};
			case 94:
				return {{996.6439209, 1115.0891113}};
			case 95:
				return {{996.6413574, 1115.0844727}};
			case 96:
				return {{996.5997314, 1115.0242920}};
			case 97:
				return {{996.5974121, 1115.0205078}};
			case 98:
				return {{996.5593872, 1114.9663086}};
			case 99:
				return {{996.5569458, 1114.9620361}};
			case 100:
				return {{996.5190430, 1114.9064941}};
			case 101:
				return {{996.5166626, 1114.9024658}};
			case 102:
				return {{996.4803467, 1114.8493652}};
			case 103:
				return {{996.4471436, 1114.8009033}};
			case 104:
				return {{996.4453735, 1114.7979736}};
			case 105:
				return {{996.4117432, 1114.7495117}};
			case 106:
				return {{996.4096069, 1114.7465820}};
			case 107:
				return {{996.4075928, 1114.7440186}};
			case 108:
				return {{996.3780518, 1114.7004395}};
			case 109:
				return {{996.3764648, 1114.6977539}};
			case 110:
				return {{996.3460693, 1114.6536865}};
			case 111:
				return {{996.3442383, 1114.6506348}};
			case 112:
				return {{996.3157959, 1114.6094971}};
			case 113:
				return {{996.3142090, 1114.6070557}};
			case 114:
				return {{996.2634277, 1114.5322266}};
			case 115:
				return {{996.2390137, 1114.4976807}};
			case 116:
				return {{996.2378540, 1114.4953613}};
			case 117:
				return {{996.2164307, 1114.4639893}};
			case 118:
				return {{996.2153320, 1114.4620361}};
			case 119:
				return {{996.1939087, 1114.4304199}};
			case 120:
				return {{996.1929932, 1114.4294434}};
			case 121:
				return {{996.1745605, 1114.4014893}};
			case 122:
				return {{996.1728516, 1114.3999023}};
			case 123:
				return {{996.1718140, 1114.3980713}};
			case 124:
				return {{996.1535034, 1114.3715820}};
			case 125:
				return {{996.1361694, 1114.3464355}};
			case 126:
				return {{996.1347656, 1114.3447266}};
			case 127:
				return {{996.1205444, 1114.3227539}};
			case 128:
				return {{996.1194458, 1114.3221436}};
			case 129:
				return {{996.1185303, 1114.3204346}};
			case 130:
				return {{996.1038818, 1114.2991943}};
			case 131:
				return {{996.0902100, 1114.2783203}};
			case 132:
				return {{996.0897217, 1114.2772217}};
			case 133:
				return {{996.0779419, 1114.2603760}};
			case 134:
				return {{996.0769653, 1114.2600098}};
			case 135:
				return {{996.0654297, 1114.2429199}};
			case 136:
				return {{996.0649414, 1114.2418213}};
			case 137:
				return {{996.0555420, 1114.2282715}};
			case 138:
				return {{996.0457764, 1114.2138672}};
			case 139:
				return {{996.0378418, 1114.2017822}};
			case 140:
				return {{996.0368652, 1114.2006836}};
			case 141:
				return {{996.0293579, 1114.1906738}};
			case 142:
				return {{996.0292969, 1114.1895752}};
			case 143:
				return {{996.0226440, 1114.1788330}};
			case 144:
				return {{996.0218506, 1114.1791992}};
			case 145:
				return {{996.0155640, 1114.1700439}};
			case 146:
				return {{996.0155029, 1114.1693115}};
			case 147:
				return {{996.0106812, 1114.1623535}};
			case 148:
				return {{996.0104370, 1114.1616211}};
			case 149:
				return {{996.0059204, 1114.1550293}};
			case 150:
				return {{996.0051270, 1114.1542969}};
			case 151:
				return {{996.0014648, 1114.1489258}};
			case 152:
				return {{996.0012817, 1114.1485596}};
			case 153:
				return {{995.9979248, 1114.1435547}};
			case 154:
				return {{995.9971924, 1114.1424561}};
			case 155:
				return {{995.9947510, 1114.1383057}};
			case 156:
				return {{995.9942017, 1114.1379395}};
			case 157:
				return {{995.9913330, 1114.1345215}};
			case 158:
				return {{995.9914551, 1114.1347656}};
			case 159:
				return {{995.9893799, 1114.1317139}};
			case 160:
				return {{995.9888916, 1114.1317139}};
			case 161:
				return {{995.9858398, 1114.1260986}};
			case 162:
				return {{995.9853516, 1114.1248779}};
			case 163:
				return {{995.9848633, 1114.1246338}};
			case 164:
				return {{995.9840088, 1114.1234131}};
			case 165:
				return {{995.9839478, 1114.1235352}};
			case 166:
				return {{995.9832764, 1114.1224365}};
			case 167:
				return {{995.9831543, 1114.1213379}};
			case 168:
				return {{995.9825439, 1114.1206055}};
			case 169:
				return {{995.9827881, 1114.1210938}};
			case 170:
				return {{995.9824219, 1114.1206055}};
			case 171:
				return {{995.9821167, 1114.1203613}};
			case 172:
				return {{995.9817505, 1114.1198730}};
			case 173:
				return {{995.9819336, 1114.1199951}};
			case 174:
				return {{995.9816895, 1114.1196289}};
			case 175:
				return {{995.9813232, 1114.1198730}};
			case 176:
				return {{995.9811401, 1114.1196289}};
			case 177:
				return {{995.9811401, 1114.1197510}};
			case 178:
				return {{995.9810791, 1114.1196289}};
			case 179:
				return {{995.9812012, 1114.1191406}};
			case 180:
				return {{995.9811401, 1114.1190186}};
			case 181:
				return {{995.9811401, 1114.1191406}};
			case 182:
				return {{995.9811401, 1114.1190186}};
			case 183:
				return {{995.9811401, 1114.1190186}};
			case 184:
				return {{995.9811401, 1114.1191406}};
			case 185:
				return {{995.9811401, 1114.1191406}};
			case 186:
				return {{995.9811401, 1114.1191406}};
			case 187:
				return {{995.9811401, 1114.1191406}};
			case 188:
				return {{995.9811401, 1114.1191406}};
			case 189:
				return {{995.9811401, 1114.1191406}};
			case 190:
				return {{995.9812012, 1114.1191406}};
			case 191:
				return {{995.9811401, 1114.1191406}};
			case 192:
				return {{995.9812012, 1114.1191406}};
			case 193:
				return {{995.9812012, 1114.1191406}};
			case 194:
				return {{995.9812012, 1114.1191406}};
			case 195:
				return {{995.9812012, 1114.1191406}};
			case 196:
				return {{995.9812012, 1114.1191406}};
			case 197:
				return {{995.9811401, 1114.1197510}};
			case 198:
				return {{995.9811401, 1114.1197510}};
			case 199:
				return {{995.9811401, 1114.1197510}};
			case 200:
				return {{995.9811401, 1114.1197510}};
			case 201:
				return {{995.9811401, 1114.1197510}};
			case 202:
				return {{995.9812012, 1114.1197510}};
			case 203:
				return {{995.9811401, 1114.1197510}};
			case 204:
				return {{995.9854736, 1114.1215820}};
			case 205:
				return {{995.9862671, 1114.1214600}};
			case 206:
				return {{996.0256958, 1114.1379395}};
			case 207:
				return {{996.0620728, 1114.1513672}};
			case 208:
				return {{996.0653076, 1114.1533203}};
			case 209:
				return {{996.0679932, 1114.1538086}};
			case 210:
				return {{996.1286011, 1114.1754150}};
			case 211:
				return {{996.1328735, 1114.1767578}};
			case 212:
				return {{996.2128296, 1114.2043457}};
			case 213:
				return {{996.2183838, 1114.2065430}};
			case 214:
				return {{996.3117676, 1114.2377930}};
			case 215:
				return {{996.3186035, 1114.2397461}};
			case 216:
				return {{996.4361572, 1114.2772217}};
			case 217:
				return {{996.4441528, 1114.2795410}};
			case 218:
				return {{996.5788574, 1114.3200684}};
			case 219:
				return {{996.5874023, 1114.3223877}};
			case 220:
				return {{996.7287598, 1114.3629150}};
			case 221:
				return {{996.7380371, 1114.3662109}};
			case 222:
				return {{996.9031982, 1114.4102783}};
			case 223:
				return {{996.9136963, 1114.4138184}};
			case 224:
				return {{997.0795898, 1114.4559326}};
			case 225:
				return {{997.0905762, 1114.4584961}};
			case 226:
				return {{997.2797241, 1114.5045166}};
			case 227:
				return {{997.2916260, 1114.5075684}};
			case 228:
				return {{997.4907227, 1114.5518799}};
			case 229:
				return {{997.5034180, 1114.5549316}};
			case 230:
				return {{997.9119873, 1114.6363525}};
			case 231:
				return {{998.1299438, 1114.6745605}};
			case 232:
				return {{998.1442871, 1114.6762695}};
			case 233:
				return {{998.1582031, 1114.6796875}};
			case 234:
				return {{998.3697510, 1114.7115479}};
			case 235:
				return {{998.6010742, 1114.7429199}};
			case 236:
				return {{998.6154785, 1114.7451172}};
			case 237:
				return {{998.8360596, 1114.7692871}};
			case 238:
				return {{998.8507690, 1114.7703857}};
			case 239:
				return {{999.0902100, 1114.7923584}};
			case 240:
				return {{999.1053467, 1114.7934570}};
			case 241:
				return {{999.3481445, 1114.8094482}};
			case 242:
				return {{999.3628540, 1114.8103027}};
			case 243:
				return {{999.5932617, 1114.8204346}};
			case 244:
				return {{999.6079102, 1114.8210449}};
			case 245:
				return {{999.8552246, 1114.8256836}};
			case 246:
				return {{999.8703003, 1114.8262939}};
			case 247:
				return {{1000.1185303, 1114.8256836}};
			case 248:
				return {{1000.1342163, 1114.8256836}};
			case 249:
				return {{1000.3675537, 1114.8190918}};
			case 250:
				return {{1000.3829346, 1114.8190918}};
			case 251:
				return {{1000.6323242, 1114.8055420}};
			case 252:
				return {{1000.6484375, 1114.8044434}};
			case 253:
				return {{1000.6635132, 1114.8032227}};
			case 254:
				return {{1001.1448975, 1114.7607422}};
			case 255:
				return {{1001.3909302, 1114.7312012}};
			case 256:
				return {{1001.4061890, 1114.7286377}};
			case 257:
				return {{1001.6370850, 1114.6939697}};
			case 258:
				return {{1001.6519165, 1114.6922607}};
			case 259:
				return {{1001.6669922, 1114.6890869}};
			case 260:
				return {{1001.9129028, 1114.6448975}};
			case 261:
				return {{1001.9281616, 1114.6424561}};
			case 262:
				return {{1002.1728516, 1114.5908203}};
			case 263:
				return {{1002.1882324, 1114.5878906}};
			case 264:
				return {{1002.4157715, 1114.5327148}};
			case 265:
				return {{1002.6581421, 1114.4694824}};
			case 266:
				return {{1002.6734619, 1114.4654541}};
			case 267:
				return {{1002.8991699, 1114.4000244}};
			case 268:
				return {{1002.9135742, 1114.3948975}};
			case 269:
				return {{1003.1525879, 1114.3183594}};
			case 270:
				return {{1003.1671753, 1114.3138428}};
			case 271:
				return {{1003.4038696, 1114.2308350}};
			case 272:
				return {{1003.4179688, 1114.2259521}};
			case 273:
				return {{1003.6379395, 1114.1427002}};
			case 274:
				return {{1003.6522827, 1114.1368408}};
			case 275:
				return {{1003.6664429, 1114.1309814}};
			case 276:
				return {{1003.8980103, 1114.0357666}};
			case 277:
				return {{1003.9123535, 1114.0299072}};
			case 278:
				return {{1003.9271240, 1114.0230713}};
			case 279:
				return {{1004.1555176, 1113.9218750}};
			case 280:
				return {{1004.3670044, 1113.8206787}};
			case 281:
				return {{1004.5901489, 1113.7083740}};
			case 282:
				return {{1004.6040039, 1113.7009277}};
			case 283:
				return {{1004.6175537, 1113.6934814}};
			case 284:
				return {{1004.8238525, 1113.5825195}};
			case 285:
				return {{1004.8374023, 1113.5747070}};
			case 286:
				return {{1004.8509521, 1113.5667725}};
			case 287:
				return {{1005.0671997, 1113.4420166}};
			case 288:
				return {{1005.0806885, 1113.4337158}};
			case 289:
				return {{1005.0943604, 1113.4254150}};
			case 290:
				return {{1005.3064575, 1113.2945557}};
			case 291:
				return {{1005.3199463, 1113.2863770}};
			case 292:
				return {{1005.3330078, 1113.2779541}};
			case 293:
				return {{1005.5292969, 1113.1495361}};
			case 294:
				return {{1005.5418701, 1113.1409912}};
			case 295:
				return {{1005.7473145, 1112.9990234}};
			case 296:
				return {{1005.7593384, 1112.9902344}};
			case 297:
				return {{1005.7728271, 1112.9809570}};
			case 298:
				return {{1005.9736328, 1112.8332520}};
			case 299:
				return {{1005.9860840, 1112.8232422}};
			case 300:
				return {{1005.9980469, 1112.8140869}};
			case 301:
				return {{1006.1826172, 1112.6712646}};
			case 302:
				return {{1006.1947021, 1112.6610107}};
			case 303:
				return {{1006.3850098, 1112.5058594}};
			case 304:
				return {{1006.3968506, 1112.4964600}};
			case 305:
				return {{1006.4085693, 1112.4864502}};
			case 306:
				return {{1006.5775146, 1112.3447266}};
			case 307:
				return {{1006.5878906, 1112.3353271}};
			case 308:
				return {{1006.7563477, 1112.1909180}};
			case 309:
				return {{1006.9101562, 1112.0570068}};
			case 310:
				return {{1006.9192505, 1112.0487061}};
			case 311:
				return {{1007.0505371, 1111.9327393}};
			case 312:
				return {{1007.0587158, 1111.9260254}};
			case 313:
				return {{1007.0669556, 1111.9187012}};
			case 314:
				return {{1007.1905518, 1111.8074951}};
			case 315:
				return {{1007.1978760, 1111.8007812}};
			case 316:
				return {{1007.2045898, 1111.7941895}};
			case 317:
				return {{1007.3123779, 1111.6971436}};
			case 318:
				return {{1007.3181763, 1111.6916504}};
			case 319:
				return {{1007.4063721, 1111.6113281}};
			case 320:
				return {{1007.4120483, 1111.6071777}};
			case 321:
				return {{1007.4168701, 1111.6015625}};
			case 322:
				return {{1007.4973145, 1111.5280762}};
			case 323:
				return {{1007.5018921, 1111.5233154}};
			case 324:
				return {{1007.5062256, 1111.5195312}};
			case 325:
				return {{1007.5694580, 1111.4610596}};
			case 326:
				return {{1007.5733032, 1111.4564209}};
			case 327:
				return {{1007.5766602, 1111.4533691}};
			case 328:
				return {{1007.6327515, 1111.4002686}};
			case 329:
				return {{1007.6357422, 1111.3972168}};
			case 330:
				return {{1007.6385498, 1111.3944092}};
			case 331:
				return {{1007.6846313, 1111.3511963}};
			case 332:
				return {{1007.6872559, 1111.3483887}};
			case 333:
				return {{1007.6895142, 1111.3463135}};
			case 334:
				return {{1007.7246094, 1111.3126221}};
			case 335:
				return {{1007.7261353, 1111.3107910}};
			case 336:
				return {{1007.7282104, 1111.3088379}};
			case 337:
				return {{1007.7579346, 1111.2790527}};
			case 338:
				return {{1007.7596436, 1111.2769775}};
			case 339:
				return {{1007.7612915, 1111.2757568}};
			case 340:
				return {{1007.7841187, 1111.2523193}};
			case 341:
				return {{1007.7855835, 1111.2506104}};
			case 342:
				return {{1007.8027954, 1111.2337646}};
			case 343:
				return {{1007.8170776, 1111.2177734}};
			case 344:
				return {{1007.8274536, 1111.2067871}};
			case 345:
				return {{1007.8278198, 1111.2062988}};
			case 346:
				return {{1007.8282471, 1111.2049561}};
			case 347:
				return {{1007.8368530, 1111.1965332}};
			case 348:
				return {{1007.8372192, 1111.1962891}};
			case 349:
				return {{1007.8426514, 1111.1892090}};
			case 350:
				return {{1007.8427734, 1111.1884766}};
			case 351:
				return {{1007.8435059, 1111.1890869}};
			case 352:
				return {{1007.8469849, 1111.1839600}};
			case 353:
				return {{1007.8475342, 1111.1835938}};
			case 354:
				return {{1007.8496704, 1111.1801758}};
			case 355:
				return {{1007.8497314, 1111.1796875}};
			case 356:
				return {{1007.8497925, 1111.1799316}};
			case 357:
				return {{1007.8515625, 1111.1771240}};
			case 358:
				return {{1007.8516235, 1111.1779785}};
			case 359:
				return {{1007.8518066, 1111.1767578}};
			case 360:
				return {{1007.8518066, 1111.1762695}};
			case 361:
				return {{1007.8518066, 1111.1760254}};
			case 362:
				return {{1007.8519287, 1111.1750488}};
			case 363:
				return {{1007.8519287, 1111.1750488}};
			case 364:
				return {{1007.8519287, 1111.1750488}};
			case 365:
				return {{1007.8519287, 1111.1750488}};
			case 366:
				return {{1007.8510132, 1111.1748047}};
			case 367:
				return {{1007.8515015, 1111.1748047}};
			case 368:
				return {{1007.8507080, 1111.1757812}};
			case 369:
				return {{1007.8502808, 1111.1757812}};
			case 370:
				return {{1007.8494873, 1111.1757812}};
			case 371:
				return {{1007.8493652, 1111.1760254}};
			case 372:
				return {{1007.8493652, 1111.1760254}};
			case 373:
				return {{1007.8481445, 1111.1772461}};
			case 374:
				return {{1007.8467407, 1111.1779785}};
			case 375:
				return {{1007.8464355, 1111.1782227}};
			case 376:
				return {{1007.8463745, 1111.1778564}};
			case 377:
				return {{1007.8458252, 1111.1784668}};
			case 378:
				return {{1007.8450928, 1111.1787109}};
			case 379:
				return {{1007.8449707, 1111.1788330}};
			case 380:
				return {{1007.8449707, 1111.1789551}};
			case 381:
				return {{1007.8443604, 1111.1791992}};
			case 382:
				return {{1007.8442383, 1111.1794434}};
			case 383:
				return {{1007.8442383, 1111.1799316}};
			case 384:
				return {{1007.8430786, 1111.1805420}};
			case 385:
				return {{1007.8429565, 1111.1811523}};
			case 386:
				return {{1007.8433838, 1111.1809082}};
			case 387:
				return {{1007.8424072, 1111.1815186}};
			case 388:
				return {{1007.8428955, 1111.1816406}};
			case 389:
				return {{1007.8417358, 1111.1815186}};
			case 390:
				return {{1007.8417969, 1111.1817627}};
			case 391:
				return {{1007.8417358, 1111.1823730}};
			case 392:
				return {{1007.8414307, 1111.1827393}};
			case 393:
				return {{1007.8414307, 1111.1827393}};
			case 394:
				return {{1007.8414307, 1111.1827393}};
			case 395:
				return {{1007.8400269, 1111.1829834}};
			case 396:
				return {{1007.8400269, 1111.1829834}};
			case 397:
				return {{1007.8400269, 1111.1829834}};
			case 398:
				return {{1007.8397217, 1111.1831055}};
			case 399:
				return {{1007.8397217, 1111.1831055}};
			case 400:
				return {{1007.8399048, 1111.1838379}};
			case 401:
				return {{1007.8399048, 1111.1838379}};
			case 402:
				return {{1007.8399048, 1111.1838379}};
			case 403:
				return {{1007.8397217, 1111.1837158}};
			case 404:
				return {{1007.8397217, 1111.1837158}};
			case 405:
				return {{1007.8397217, 1111.1837158}};
			case 406:
				return {{1007.8390503, 1111.1840820}};
			case 407:
				return {{1007.8388672, 1111.1843262}};
			case 408:
				return {{1007.8382568, 1111.1846924}};
			case 409:
				return {{1007.8382568, 1111.1846924}};
			case 410:
				return {{1007.8382568, 1111.1846924}};
			case 411:
				return {{1007.8387451, 1111.1849365}};
			case 412:
				return {{1007.8387451, 1111.1849365}};
			case 413:
				return {{1007.8387451, 1111.1849365}};
			case 414:
				return {{1007.8386841, 1111.1851807}};
			case 415:
				return {{1007.8386841, 1111.1845703}};
			case 416:
				return {{1007.8386841, 1111.1845703}};
			case 417:
				return {{1007.8386230, 1111.1842041}};
			case 418:
				return {{1007.8386230, 1111.1842041}};
			case 419:
				return {{1007.8386230, 1111.1842041}};
			case 420:
				return {{1007.8385010, 1111.1843262}};
			case 421:
				return {{1007.8385010, 1111.1843262}};
			case 422:
				return {{1007.8380127, 1111.1850586}};
			case 423:
				return {{1007.8380127, 1111.1850586}};
			case 424:
				return {{1007.8380127, 1111.1850586}};
			case 425:
				return {{1007.8380737, 1111.1846924}};
			case 426:
				return {{1007.8380737, 1111.1846924}};
			case 427:
				return {{1007.8380737, 1111.1846924}};
			case 428:
				return {{1007.8380737, 1111.1846924}};
			case 429:
				return {{1007.8380737, 1111.1846924}};
			case 430:
				return {{1007.8380737, 1111.1846924}};
			case 431:
				return {{1007.8380737, 1111.1846924}};
			case 432:
				return {{1007.8380737, 1111.1846924}};
			case 433:
				return {{1007.8380737, 1111.1846924}};
			case 434:
				return {{1007.8380737, 1111.1846924}};
			case 435:
				return {{1007.8380737, 1111.1846924}};
			case 436:
				return {{1007.8380737, 1111.1846924}};
			case 437:
				return {{1007.8380737, 1111.1846924}};
			case 438:
				return {{1007.8380737, 1111.1846924}};
			case 439:
				return {{1007.8380737, 1111.1846924}};
			case 440:
				return {{1007.8380737, 1111.1846924}};
			case 441:
				return {{1007.8380737, 1111.1846924}};
			case 442:
				return {{1007.8380737, 1111.1846924}};
			case 443:
				return {{1007.8380737, 1111.1846924}};
			case 444:
				return {{1007.8380737, 1111.1846924}};
			case 445:
				return {{1007.8380737, 1111.1846924}};
			case 446:
				return {{1007.8380737, 1111.1846924}};
			case 447:
				return {{1007.8380737, 1111.1846924}};
			case 448:
				return {{1007.8380737, 1111.1846924}};
			case 449:
				return {{1007.8380737, 1111.1846924}};
			case 450:
				return {{1007.8380737, 1111.1846924}};
			case 451:
				return {{1007.8380737, 1111.1846924}};
			case 452:
				return {{1007.8380737, 1111.1846924}};
			case 453:
				return {{1007.8380737, 1111.1846924}};
			case 454:
				return {{1007.8380737, 1111.1846924}};
			case 455:
				return {{1007.8380737, 1111.1846924}};
			case 456:
				return {{1007.8380737, 1111.1846924}};
			case 457:
				return {{1007.8380737, 1111.1846924}};
			case 458:
				return {{1007.8380737, 1111.1846924}};
			case 459:
				return {{1007.8380737, 1111.1846924}};
			case 460:
				return {{1007.8380737, 1111.1846924}};
			case 461:
				return {{1007.8380737, 1111.1846924}};
			case 462:
				return {{1007.8380737, 1111.1846924}};
			case 463:
				return {{1007.8380737, 1111.1846924}};
			case 464:
				return {{1007.8380737, 1111.1846924}};
			case 465:
				return {{1007.8380737, 1111.1846924}};
			case 466:
				return {{1007.8380737, 1111.1846924}};
			case 467:
				return {{1007.8380737, 1111.1846924}};
			case 468:
				return {{1007.8380737, 1111.1846924}};
			case 469:
				return {{1007.8380737, 1111.1846924}};
			case 470:
				return {{1007.8380737, 1111.1846924}};
			case 471:
				return {{1007.8380737, 1111.1846924}};
			case 472:
				return {{1007.8380737, 1111.1846924}};
			case 473:
				return {{1007.8380737, 1111.1846924}};
			case 474:
				return {{1007.8380737, 1111.1846924}};
			case 475:
				return {{1007.8380737, 1111.1846924}};
			case 476:
				return {{1007.8380737, 1111.1846924}};
			case 477:
				return {{1007.8380737, 1111.1846924}};
			case 478:
				return {{1007.8380737, 1111.1846924}};
			case 479:
				return {{1007.8380737, 1111.1846924}};
			case 480:
				return {{1007.8380737, 1111.1846924}};
			case 481:
				return {{1007.8380737, 1111.1846924}};
			case 482:
				return {{1007.8380737, 1111.1846924}};
			case 483:
				return {{1007.8380737, 1111.1846924}};
			case 484:
				return {{1007.8380737, 1111.1846924}};
			case 485:
				return {{1007.8380737, 1111.1846924}};
			case 486:
				return {{1007.8380737, 1111.1846924}};
			case 487:
				return {{1007.8380737, 1111.1846924}};
			case 488:
				return {{1007.8380737, 1111.1846924}};
			case 489:
				return {{1007.8380737, 1111.1846924}};
			case 490:
				return {{1007.8380737, 1111.1846924}};
			case 491:
				return {{1007.8380737, 1111.1846924}};
			case 492:
				return {{1007.8380737, 1111.1846924}};
			case 493:
				return {{1007.8380737, 1111.1846924}};
			case 494:
				return {{1007.8380737, 1111.1846924}};
			case 495:
				return {{1007.8380737, 1111.1846924}};
			case 496:
				return {{1007.8380737, 1111.1846924}};
			case 497:
				return {{1007.8380737, 1111.1846924}};
			case 498:
				return {{1007.8380737, 1111.1846924}};
			case 499:
				return {{1007.8380737, 1111.1846924}};
			case 500:
				return {{1007.8380737, 1111.1846924}};
			case 501:
				return {{1007.8380737, 1111.1846924}};
			case 502:
				return {{1007.8380737, 1111.1846924}};
			case 503:
				return {{1007.8384399, 1111.1850586}};
			case 504:
				return {{1007.8289795, 1111.1958008}};
			case 505:
				return {{1007.8281250, 1111.1976318}};
			case 506:
				return {{1007.8276367, 1111.1979980}};
			case 507:
				return {{1007.8267822, 1111.1988525}};
			case 508:
				return {{1007.8087769, 1111.2197266}};
			case 509:
				return {{1007.8077393, 1111.2214355}};
			case 510:
				return {{1007.7755127, 1111.2573242}};
			case 511:
				return {{1007.7728271, 1111.2602539}};
			case 512:
				return {{1007.7287598, 1111.3093262}};
			case 513:
				return {{1007.7252197, 1111.3123779}};
			case 514:
				return {{1007.7218628, 1111.3164062}};
			case 515:
				return {{1007.7182007, 1111.3201904}};
			case 516:
				return {{1007.6532593, 1111.3890381}};
			case 517:
				return {{1007.6489868, 1111.3941650}};
			case 518:
				return {{1007.5692749, 1111.4768066}};
			case 519:
				return {{1007.5632324, 1111.4818115}};
			case 520:
				return {{1007.5581055, 1111.4880371}};
			case 521:
				return {{1007.4683838, 1111.5781250}};
			case 522:
				return {{1007.4615479, 1111.5847168}};
			case 523:
				return {{1007.4552002, 1111.5908203}};
			case 524:
				return {{1007.3450928, 1111.6986084}};
			case 525:
				return {{1007.3379517, 1111.7060547}};
			case 526:
				return {{1007.3302612, 1111.7136230}};
			case 527:
				return {{1007.2143555, 1111.8236084}};
			case 528:
				return {{1007.2058716, 1111.8311768}};
			case 529:
				return {{1007.1979370, 1111.8388672}};
			case 530:
				return {{1007.0617065, 1111.9638672}};
			case 531:
				return {{1007.0523682, 1111.9718018}};
			case 532:
				return {{1007.0435791, 1111.9794922}};
			case 533:
				return {{1006.8957520, 1112.1113281}};
			case 534:
				return {{1006.8863525, 1112.1192627}};
			case 535:
				return {{1006.8768311, 1112.1281738}};
			case 536:
				return {{1006.5637817, 1112.3908691}};
			case 537:
				return {{1006.5538940, 1112.4000244}};
			case 538:
				return {{1006.3827515, 1112.5377197}};
			case 539:
				return {{1006.3720093, 1112.5458984}};
			case 540:
				return {{1006.3613281, 1112.5555420}};
			case 541:
				return {{1006.1921387, 1112.6850586}};
			case 542:
				return {{1006.1813354, 1112.6933594}};
			case 543:
				return {{1006.1693115, 1112.7022705}};
			case 544:
				return {{1005.9817505, 1112.8392334}};
			case 545:
				return {{1005.9697266, 1112.8483887}};
			case 546:
				return {{1005.9580078, 1112.8570557}};
			case 547:
				return {{1005.7756958, 1112.9844971}};
			case 548:
				return {{1005.7630005, 1112.9936523}};
			case 549:
				return {{1005.7512817, 1113.0017090}};
			case 550:
				return {{1005.5505981, 1113.1357422}};
			case 551:
				return {{1005.5379639, 1113.1437988}};
			case 552:
				return {{1005.3319092, 1113.2741699}};
			case 553:
				return {{1005.3191528, 1113.2817383}};
			case 554:
				return {{1005.1225586, 1113.4010010}};
			case 555:
				return {{1005.1091309, 1113.4089355}};
			case 556:
				return {{1005.0953369, 1113.4167480}};
			case 557:
				return {{1004.8810425, 1113.5393066}};
			case 558:
				return {{1004.8676758, 1113.5472412}};
			case 559:
				return {{1004.8542480, 1113.5549316}};
			case 560:
				return {{1004.6356201, 1113.6723633}};
			case 561:
				return {{1004.6222534, 1113.6802979}};
			case 562:
				return {{1004.6078491, 1113.6871338}};
			case 563:
				return {{1004.4006348, 1113.7924805}};
			case 564:
				return {{1004.3862305, 1113.7995605}};
			case 565:
				return {{1004.3718872, 1113.8066406}};
			case 566:
				return {{1004.1467896, 1113.9133301}};
			case 567:
				return {{1004.1329346, 1113.9199219}};
			case 568:
				return {{1003.9191895, 1114.0153809}};
			case 569:
				return {{1003.6888428, 1114.1119385}};
			case 570:
				return {{1003.4564209, 1114.2023926}};
			case 571:
				return {{1003.4418945, 1114.2078857}};
			case 572:
				return {{1003.4270020, 1114.2133789}};
			case 573:
				return {{1003.2070312, 1114.2922363}};
			case 574:
				return {{1003.1921387, 1114.2972412}};
			case 575:
				return {{1003.1777344, 1114.3023682}};
			case 576:
				return {{1002.9405518, 1114.3801270}};
			case 577:
				return {{1002.9252930, 1114.3847656}};
			case 578:
				return {{1002.9104004, 1114.3905029}};
			case 579:
				return {{1002.6714478, 1114.4609375}};
			case 580:
				return {{1002.6562500, 1114.4650879}};
			case 581:
				return {{1002.6410522, 1114.4700928}};
			case 582:
				return {{1002.4150391, 1114.5305176}};
			case 583:
				return {{1002.4004517, 1114.5340576}};
			case 584:
				return {{1002.1575928, 1114.5927734}};
			case 585:
				return {{1002.1423340, 1114.5952148}};
			case 586:
				return {{1002.1273193, 1114.5993652}};
			case 587:
				return {{1001.8986816, 1114.6479492}};
			case 588:
				return {{1001.8834229, 1114.6505127}};
			case 589:
				return {{1001.8682251, 1114.6536865}};
			case 590:
				return {{1001.6224365, 1114.6979980}};
			case 591:
				return {{1001.6073608, 1114.7006836}};
			case 592:
				return {{1001.5923462, 1114.7038574}};
			case 593:
				return {{1001.3455200, 1114.7419434}};
			case 594:
				return {{1001.3296509, 1114.7427979}};
			case 595:
				return {{1001.3143311, 1114.7449951}};
			case 596:
				return {{1001.0827637, 1114.7740479}};
			case 597:
				return {{1001.0677490, 1114.7761230}};
			case 598:
				return {{1001.0520020, 1114.7775879}};
			case 599:
				return {{1000.8041992, 1114.8022461}};
			case 600:
				return {{1000.7883301, 1114.8032227}};
			case 601:
				return {{1000.3091431, 1114.8314209}};
			case 602:
				return {{1000.2941895, 1114.8325195}};
			case 603:
				return {{1000.0582275, 1114.8403320}};
			case 604:
				return {{1000.0443115, 1114.8406982}};
			case 605:
				return {{999.8395996, 1114.8457031}};
			case 606:
				return {{999.8267212, 1114.8464355}};
			case 607:
				return {{999.8137817, 1114.8466797}};
			case 608:
				return {{999.6163940, 1114.8513184}};
			case 609:
				return {{999.6040039, 1114.8513184}};
			case 610:
				return {{999.4280396, 1114.8536377}};
			case 611:
				return {{999.4172363, 1114.8536377}};
			case 612:
				return {{999.2713623, 1114.8562012}};
			case 613:
				return {{999.2623291, 1114.8557129}};
			case 614:
				return {{999.1259766, 1114.8574219}};
			case 615:
				return {{999.1174316, 1114.8576660}};
			case 616:
				return {{999.1100464, 1114.8577881}};
			case 617:
				return {{998.9929810, 1114.8582764}};
			case 618:
				return {{998.9861450, 1114.8575439}};
			case 619:
				return {{998.9797974, 1114.8575439}};
			case 620:
				return {{998.8861084, 1114.8579102}};
			case 621:
				return {{998.8806152, 1114.8576660}};
			case 622:
				return {{998.7966309, 1114.8576660}};
			case 623:
				return {{998.7912598, 1114.8576660}};
			case 624:
				return {{998.7865601, 1114.8579102}};
			case 625:
				return {{998.7211914, 1114.8562012}};
			case 626:
				return {{998.7169800, 1114.8569336}};
			case 627:
				return {{998.7128906, 1114.8563232}};
			case 628:
				return {{998.6065674, 1114.8542480}};
			case 629:
				return {{998.6041870, 1114.8543701}};
			case 630:
				return {{998.5671997, 1114.8530273}};
			case 631:
				return {{998.5650635, 1114.8530273}};
			case 632:
				return {{998.5625000, 1114.8525391}};
			case 633:
				return {{998.5311279, 1114.8519287}};
			case 634:
				return {{998.5291138, 1114.8516846}};
			case 635:
				return {{998.5039673, 1114.8496094}};
			case 636:
				return {{998.5028687, 1114.8491211}};
			case 637:
				return {{998.5015869, 1114.8503418}};
			case 638:
				return {{998.4829102, 1114.8483887}};
			case 639:
				return {{998.4818115, 1114.8485107}};
			case 640:
				return {{998.4808350, 1114.8477783}};
			case 641:
				return {{998.4659424, 1114.8472900}};
			case 642:
				return {{998.4648438, 1114.8464355}};
			case 643:
				return {{998.4539795, 1114.8457031}};
			case 644:
				return {{998.4536133, 1114.8459473}};
			case 645:
				return {{998.4526978, 1114.8450928}};
			case 646:
				return {{998.4441528, 1114.8438721}};
			case 647:
				return {{998.4439697, 1114.8444824}};
			case 648:
				return {{998.4433594, 1114.8447266}};
			case 649:
				return {{998.4368896, 1114.8430176}};
			case 650:
				return {{998.4368896, 1114.8431396}};
			case 651:
				return {{998.4328613, 1114.8426514}};
			case 652:
				return {{998.4331665, 1114.8427734}};
			case 653:
				return {{998.4328613, 1114.8415527}};
			case 654:
				return {{998.4296265, 1114.8415527}};
			case 655:
				return {{998.4299316, 1114.8415527}};
			case 656:
				return {{998.4282227, 1114.8405762}};
			case 657:
				return {{998.4281006, 1114.8405762}};
			case 658:
				return {{998.4277344, 1114.8403320}};
			case 659:
				return {{998.4276123, 1114.8403320}};
			case 660:
				return {{998.4272461, 1114.8397217}};
			case 661:
				return {{998.4273682, 1114.8397217}};
			case 662:
				return {{998.4274292, 1114.8397217}};
			case 663:
				return {{998.4283447, 1114.8385010}};
			case 664:
				return {{998.4284058, 1114.8386230}};
			case 665:
				return {{998.4290771, 1114.8389893}};
			case 666:
				return {{998.4291992, 1114.8389893}};
			case 667:
				return {{998.4292603, 1114.8389893}};
			case 668:
				return {{998.4304810, 1114.8381348}};
			case 669:
				return {{998.4299316, 1114.8381348}};
			case 670:
				return {{998.4300537, 1114.8381348}};
			case 671:
				return {{998.4304199, 1114.8386230}};
			case 672:
				return {{998.4310303, 1114.8386230}};
			case 673:
				return {{998.4320068, 1114.8386230}};
			case 674:
				return {{998.4320068, 1114.8386230}};
			case 675:
				return {{998.4320068, 1114.8386230}};
			case 676:
				return {{998.4328613, 1114.8378906}};
			case 677:
				return {{998.4329224, 1114.8378906}};
			case 678:
				return {{998.4334717, 1114.8378906}};
			case 679:
				return {{998.4343262, 1114.8383789}};
			case 680:
				return {{998.4338379, 1114.8383789}};
			case 681:
				return {{998.4349365, 1114.8383789}};
			case 682:
				return {{998.4349976, 1114.8385010}};
			case 683:
				return {{998.4350586, 1114.8383789}};
			case 684:
				return {{998.4350586, 1114.8383789}};
			case 685:
				return {{998.4356079, 1114.8383789}};
			case 686:
				return {{998.4360352, 1114.8374023}};
			case 687:
				return {{998.4361572, 1114.8374023}};
			case 688:
				return {{998.4368896, 1114.8382568}};
			case 689:
				return {{998.4371338, 1114.8377686}};
			case 690:
				return {{998.4378052, 1114.8374023}};
			case 691:
				return {{998.4378662, 1114.8374023}};
			case 692:
				return {{998.4384155, 1114.8374023}};
			case 693:
				return {{998.4379883, 1114.8378906}};
			case 694:
				return {{998.4380493, 1114.8378906}};
			case 695:
				return {{998.4384766, 1114.8381348}};
			case 696:
				return {{998.4384766, 1114.8381348}};
			case 697:
				return {{998.4384766, 1114.8381348}};
			case 698:
				return {{998.4388428, 1114.8375244}};
			case 699:
				return {{998.4388428, 1114.8375244}};
			case 700:
				return {{998.4386597, 1114.8378906}};
			case 701:
				return {{998.4386597, 1114.8378906}};
			case 702:
				return {{998.4393311, 1114.8372803}};
			case 703:
				return {{998.4393311, 1114.8372803}};
			case 704:
				return {{998.4393311, 1114.8372803}};
			case 705:
				return {{998.4394531, 1114.8371582}};
			case 706:
				return {{998.4394531, 1114.8371582}};
			case 707:
				return {{998.4394531, 1114.8371582}};
			case 708:
				return {{998.4392700, 1114.8375244}};
			case 709:
				return {{998.4396973, 1114.8380127}};
			case 710:
				return {{998.4393311, 1114.8378906}};
			case 711:
				return {{998.4393311, 1114.8378906}};
			case 712:
				return {{998.4393311, 1114.8378906}};
			case 713:
				return {{998.4399414, 1114.8377686}};
			case 714:
				return {{998.4399414, 1114.8377686}};
			case 715:
				return {{998.4399414, 1114.8377686}};
			case 716:
				return {{998.4395752, 1114.8377686}};
			case 717:
				return {{998.4395752, 1114.8377686}};
			case 718:
				return {{998.4396362, 1114.8377686}};
			case 719:
				return {{998.4401855, 1114.8377686}};
			case 720:
				return {{998.4402466, 1114.8377686}};
			case 721:
				return {{998.4402466, 1114.8377686}};
			case 722:
				return {{998.4402466, 1114.8377686}};
			case 723:
				return {{998.4403076, 1114.8377686}};
			case 724:
				return {{998.4403076, 1114.8377686}};
			case 725:
				return {{998.4403076, 1114.8377686}};
			case 726:
				return {{998.4403076, 1114.8377686}};
			case 727:
				return {{998.4403076, 1114.8377686}};
			case 728:
				return {{998.4403076, 1114.8377686}};
			case 729:
				return {{998.4403076, 1114.8377686}};
			case 730:
				return {{998.4403076, 1114.8377686}};
			case 731:
				return {{998.4403076, 1114.8377686}};
			case 732:
				return {{998.4403076, 1114.8377686}};
			case 733:
				return {{998.4403076, 1114.8377686}};
			case 734:
				return {{998.4403076, 1114.8377686}};
			case 735:
				return {{998.4403076, 1114.8377686}};
			case 736:
				return {{998.4403076, 1114.8377686}};
			case 737:
				return {{998.4403076, 1114.8377686}};
			case 738:
				return {{998.4403076, 1114.8377686}};
			case 739:
				return {{998.4403076, 1114.8377686}};
			case 740:
				return {{998.4403076, 1114.8377686}};
			case 741:
				return {{998.4403076, 1114.8377686}};
			case 742:
				return {{998.4403076, 1114.8377686}};
			case 743:
				return {{998.4403076, 1114.8377686}};
			case 744:
				return {{998.4403076, 1114.8377686}};
			case 745:
				return {{998.4403076, 1114.8377686}};
			case 746:
				return {{998.4403076, 1114.8377686}};
			case 747:
				return {{998.4403076, 1114.8377686}};
			case 748:
				return {{998.4403076, 1114.8377686}};
			case 749:
				return {{998.4403076, 1114.8377686}};
			case 750:
				return {{998.4403076, 1114.8377686}};
			case 751:
				return {{998.4403076, 1114.8377686}};
			case 752:
				return {{998.4403076, 1114.8377686}};
			case 753:
				return {{998.4403076, 1114.8377686}};
			case 754:
				return {{998.4403076, 1114.8377686}};
			case 755:
				return {{998.4403076, 1114.8377686}};
			case 756:
				return {{998.4403076, 1114.8377686}};
			case 757:
				return {{998.4403076, 1114.8377686}};
			case 758:
				return {{998.4403076, 1114.8377686}};
			case 759:
				return {{998.4403076, 1114.8377686}};
			case 760:
				return {{998.4403076, 1114.8377686}};
			case 761:
				return {{998.4403076, 1114.8377686}};
			case 762:
				return {{998.4403076, 1114.8377686}};
			case 763:
				return {{998.4403076, 1114.8377686}};
			case 764:
				return {{998.4403076, 1114.8377686}};
			case 765:
				return {{998.4403076, 1114.8377686}};
			case 766:
				return {{998.4403076, 1114.8377686}};
			case 767:
				return {{998.4403076, 1114.8377686}};
			case 768:
				return {{998.4403076, 1114.8377686}};
			case 769:
				return {{998.4403076, 1114.8377686}};
			case 770:
				return {{998.4403076, 1114.8377686}};
			case 771:
				return {{998.4403076, 1114.8377686}};
			case 772:
				return {{998.4403076, 1114.8377686}};
			case 773:
				return {{998.4403076, 1114.8377686}};
			case 774:
				return {{998.4403076, 1114.8377686}};
			case 775:
				return {{998.4403076, 1114.8377686}};
			case 776:
				return {{998.4403076, 1114.8377686}};
			case 777:
				return {{998.4403076, 1114.8377686}};
			case 778:
				return {{998.4403076, 1114.8377686}};
			case 779:
				return {{998.4403076, 1114.8377686}};
			case 780:
				return {{998.4403076, 1114.8377686}};
			case 781:
				return {{998.4403076, 1114.8377686}};
			case 782:
				return {{998.4403076, 1114.8377686}};
			case 783:
				return {{998.4403076, 1114.8377686}};
			case 784:
				return {{998.4403076, 1114.8377686}};
			case 785:
				return {{998.4403076, 1114.8377686}};
			case 786:
				return {{998.4403076, 1114.8377686}};
			case 787:
				return {{998.4403076, 1114.8377686}};
			case 788:
				return {{998.4403076, 1114.8377686}};
			case 789:
				return {{998.4403076, 1114.8377686}};
			case 790:
				return {{998.4403076, 1114.8377686}};
			case 791:
				return {{998.4403076, 1114.8377686}};
			case 792:
				return {{998.4403076, 1114.8377686}};
			case 793:
				return {{998.4403076, 1114.8377686}};
			case 794:
				return {{998.4403076, 1114.8377686}};
			case 795:
				return {{998.4403076, 1114.8377686}};
			case 796:
				return {{998.4403076, 1114.8377686}};
			case 797:
				return {{998.4403076, 1114.8377686}};
			case 798:
				return {{998.4403076, 1114.8377686}};
			case 799:
				return {{998.4403076, 1114.8377686}};
			case 800:
				return {{998.4403076, 1114.8377686}};
			case 801:
				return {{998.4403076, 1114.8377686}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		assert(false); // Shouldn't be any unaccounted raycasts
		return std::nullopt;
	}
};
