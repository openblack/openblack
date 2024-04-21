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

class TiltUpPanLeftMockAction final: public MockAction
{
	static constexpr uint32_t k_Start1 = k_StabilizeFrames + 1;
	static constexpr uint32_t k_End1 = k_StabilizeFrames + k_InteractionFrames - 1;
	static constexpr uint32_t k_Start2 = k_StabilizeFrames + k_InteractionFrames + k_StabilizeFrames + 1;
	static constexpr uint32_t k_End2 = k_StabilizeFrames + k_InteractionFrames + k_StabilizeFrames + k_InteractionFrames - 1;

public:
	~TiltUpPanLeftMockAction() final = default;

	[[nodiscard]] bool GetBindable(openblack::input::BindableActionMap action) const final
	{
		using openblack::input::BindableActionMap;
		if (frameNumber >= k_Start1 && frameNumber <= k_End1)
		{
			return (static_cast<uint32_t>(action) & static_cast<uint32_t>(BindableActionMap::TILT_UP)) != 0;
		}

		if (frameNumber >= k_Start2 && frameNumber <= k_End2)
		{
			return (static_cast<uint32_t>(action) & static_cast<uint32_t>(BindableActionMap::ROTATE_LEFT)) != 0;
		}

		return false;
	}
};

// NOLINTBEGIN(bugprone-branch-clone, google-readability-function-size)
class TiltUpPanLeftMockDynamicsSystem: public MockDynamicsSystem
{
public:
	TiltUpPanLeftMockDynamicsSystem() = default;

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
				return {{0.0000000, 16.1538353}};
			case 1:
				return {{1000.0000000, -770.0559082}};
			case 2:
				return {{1000.0000000, -743.0767822}};
			case 3:
				return {{1000.0000000, -5.7276611}};
			case 4:
				return {{1000.0000000, 105.8948517}};
			case 5:
				return {{1000.0000000, 112.1076202}};
			case 6:
				return {{1000.0000000, 118.2447052}};
			case 7:
				return {{1000.0000000, 200.3244019}};
			case 8:
				return {{1000.0000000, 204.8554993}};
			case 9:
				return {{1000.0000000, 264.9228516}};
			case 10:
				return {{1000.0000000, 268.0856323}};
			case 11:
				return {{1000.0000000, 271.2250366}};
			case 12:
				return {{1000.0000000, 313.6578369}};
			case 13:
				return {{1000.0000000, 316.1343384}};
			case 14:
				return {{1000.0000000, 318.7072144}};
			case 15:
				return {{1000.0000000, 321.1528931}};
			case 16:
				return {{1000.0000000, 355.7194519}};
			case 17:
				return {{1000.0000000, 357.7429199}};
			case 18:
				return {{1000.0000000, 384.0479126}};
			case 19:
				return {{1000.0000000, 407.4533691}};
			case 20:
				return {{1000.0000000, 427.1724854}};
			case 21:
				return {{1000.0000000, 428.2545471}};
			case 22:
				return {{1000.0000000, 443.9299011}};
			case 23:
				return {{1000.0000000, 444.8811340}};
			case 24:
				return {{1000.0000000, 459.2140198}};
			case 25:
				return {{1000.0000000, 460.0471497}};
			case 26:
				return {{1000.0000000, 472.5143433}};
			case 27:
				return {{1000.0000000, 473.2416382}};
			case 28:
				return {{1000.0000000, 483.4656067}};
			case 29:
				return {{1000.0000000, 484.1685791}};
			case 30:
				return {{1000.0000000, 493.8484192}};
			case 31:
				return {{1000.0000000, 494.3926697}};
			case 32:
				return {{1000.0000000, 502.4442139}};
			case 33:
				return {{1000.0000000, 502.9736938}};
			case 34:
				return {{1000.0000000, 510.7427979}};
			case 35:
				return {{1000.0000000, 511.1937256}};
			case 36:
				return {{1000.0000000, 518.1301270}};
			case 37:
				return {{1000.0000000, 518.5700684}};
			case 38:
				return {{1000.0000000, 519.0095215}};
			case 39:
				return {{1000.0000000, 500.6634216}};
			case 40:
				return {{1000.0000000, 498.8476868}};
			case 41:
				return {{1000.0000000, 471.6894531}};
			case 42:
				return {{1000.0000000, 469.9794006}};
			case 43:
				return {{1000.0000000, 442.9660645}};
			case 44:
				return {{1000.0000000, 417.5871277}};
			case 45:
				return {{1000.0000000, 391.1619568}};
			case 46:
				return {{1000.0000000, 389.5305786}};
			case 47:
				return {{1000.0000000, 364.7833862}};
			case 48:
				return {{1000.0000000, 363.1747437}};
			case 49:
				return {{1000.0000000, 337.2171631}};
			case 50:
				return {{1000.0000000, 335.5335999}};
			case 51:
				return {{1000.0000000, 309.9683228}};
			case 52:
				return {{1000.0000000, 308.3560791}};
			case 53:
				return {{1000.0000000, 285.0237427}};
			case 54:
				return {{1000.0000000, 283.0931091}};
			case 55:
				return {{1000.0000000, 258.5480347}};
			case 56:
				return {{1000.0000000, 256.9194946}};
			case 57:
				return {{1000.0000000, 232.4271393}};
			case 58:
				return {{1000.0000000, 230.9935760}};
			case 59:
				return {{1000.0000000, 208.8545990}};
			case 60:
				return {{1000.0000000, 207.3721313}};
			case 61:
				return {{1000.0000000, 184.0393066}};
			case 62:
				return {{1000.0000000, 182.6068115}};
			case 63:
				return {{1000.0000000, 161.3394165}};
			case 64:
				return {{1000.0000000, 159.7380066}};
			case 65:
				return {{1000.0000000, 137.5003815}};
			case 66:
				return {{1000.0000000, 115.8879089}};
			case 67:
				return {{1000.0000000, 95.8544159}};
			case 68:
				return {{1000.0000000, 94.5362091}};
			case 69:
				return {{1000.0000000, 73.7548065}};
			case 70:
				return {{1000.0000000, 72.4786377}};
			case 71:
				return {{1000.0000000, 52.5038910}};
			case 72:
				return {{1000.0000000, 51.1960602}};
			case 73:
				return {{1000.0000000, 32.9644012}};
			case 74:
				return {{1000.0000000, 31.5235901}};
			case 75:
				return {{1000.0000000, 30.2701569}};
			case 76:
				return {{1000.0000000, 11.1421204}};
			case 77:
				return {{1000.0000000, 10.1613617}};
			case 78:
				return {{1000.0000000, -7.2264404}};
			case 79:
				return {{1000.0000000, -8.4018555}};
			case 80:
				return {{1000.0000000, -26.1702881}};
			case 81:
				return {{1000.0000000, -27.1201172}};
			case 82:
				return {{1000.0000000, -44.8266602}};
			case 83:
				return {{1000.0000000, -45.5584717}};
			case 84:
				return {{1000.0000000, -61.5065918}};
			case 85:
				return {{1000.0000000, -62.6777344}};
			case 86:
				return {{1000.0000000, -78.9976807}};
			case 87:
				return {{1000.0000000, -79.9364014}};
			case 88:
				return {{1000.0000000, -95.5395508}};
			case 89:
				return {{1000.0000000, -96.6873779}};
			case 90:
				return {{1000.0000000, -110.8873291}};
			case 91:
				return {{1000.0000000, -125.5928955}};
			case 92:
				return {{1000.0000000, -139.0494385}};
			case 93:
				return {{1000.0000000, -139.7036133}};
			case 94:
				return {{1000.0000000, -153.5233154}};
			case 95:
				return {{1000.0000000, -154.3769531}};
			case 96:
				return {{1000.0000000, -167.5012207}};
			case 97:
				return {{1000.0000000, -168.5506592}};
			case 98:
				return {{1000.0000000, -180.2821045}};
			case 99:
				return {{1000.0000000, -181.0770264}};
			case 100:
				return {{1000.0000000, -193.0888672}};
			case 101:
				return {{1000.0000000, -194.0817871}};
			case 102:
				return {{1000.0000000, -205.6433105}};
			case 103:
				return {{1000.0000000, -216.1108398}};
			case 104:
				return {{1000.0000000, -216.8082275}};
			case 105:
				return {{1000.0000000, -227.3505859}};
			case 106:
				return {{1000.0000000, -228.0187988}};
			case 107:
				return {{1000.0000000, -228.5657959}};
			case 108:
				return {{1000.0000000, -238.1468506}};
			case 109:
				return {{1000.0000000, -238.7722168}};
			case 110:
				return {{1000.0000000, -248.4827881}};
			case 111:
				return {{1000.0000000, -249.0815430}};
			case 112:
				return {{1000.0000000, -258.1691895}};
			case 113:
				return {{1000.0000000, -258.7281494}};
			case 114:
				return {{1000.0000000, -275.2501221}};
			case 115:
				return {{1000.0000000, -283.1724854}};
			case 116:
				return {{1000.0000000, -283.3957520}};
			case 117:
				return {{1000.0000000, -290.4317627}};
			case 118:
				return {{1000.0000000, -290.8327637}};
			case 119:
				return {{1000.0000000, -297.8603516}};
			case 120:
				return {{1000.0000000, -298.2362061}};
			case 121:
				return {{1000.0000000, -304.2756348}};
			case 122:
				return {{1000.0000000, -304.6058350}};
			case 123:
				return {{1000.0000000, -305.0563965}};
			case 124:
				return {{1000.0000000, -311.0228271}};
			case 125:
				return {{1000.0000000, -316.6230469}};
			case 126:
				return {{1000.0000000, -317.1135254}};
			case 127:
				return {{1000.0000000, -321.9949951}};
			case 128:
				return {{1000.0000000, -322.4465332}};
			case 129:
				return {{1000.0000000, -322.8072510}};
			case 130:
				return {{1000.0000000, -327.5804443}};
			case 131:
				return {{1000.0000000, -332.0192871}};
			case 132:
				return {{1000.0000000, -332.1802979}};
			case 133:
				return {{1000.0000000, -336.0573730}};
			case 134:
				return {{1000.0000000, -336.1817627}};
			case 135:
				return {{1000.0000000, -340.1765137}};
			case 136:
				return {{1000.0000000, -340.2766113}};
			case 137:
				return {{1000.0000000, -343.6551514}};
			case 138:
				return {{1000.0000000, -346.5415039}};
			case 139:
				return {{1000.0000000, -349.7819824}};
			case 140:
				return {{1000.0000000, -349.7995605}};
			case 141:
				return {{1000.0000000, -352.2539062}};
			case 142:
				return {{1000.0000000, -352.4711914}};
			case 143:
				return {{1000.0000000, -354.5693359}};
			case 144:
				return {{1000.0000000, -354.7683105}};
			case 145:
				return {{1000.0000000, -356.8011475}};
			case 146:
				return {{1000.0000000, -356.9757080}};
			case 147:
				return {{1000.0000000, -358.6204834}};
			case 148:
				return {{1000.0000000, -358.7729492}};
			case 149:
				return {{1000.0000000, -360.5307617}};
			case 150:
				return {{1000.0000000, -360.4355469}};
			case 151:
				return {{1000.0000000, -361.8718262}};
			case 152:
				return {{1000.0000000, -361.7562256}};
			case 153:
				return {{1000.0000000, -363.0778809}};
			case 154:
				return {{1000.0000000, -363.1750488}};
			case 155:
				return {{1000.0000000, -364.0666504}};
			case 156:
				return {{1000.0000000, -364.1520996}};
			case 157:
				return {{1000.0000000, -364.8118896}};
			case 158:
				return {{1000.0000000, -365.1131592}};
			case 159:
				return {{1000.0000000, -365.6785889}};
			case 160:
				return {{1000.0000000, -365.7379150}};
			case 161:
				return {{1000.0000000, -366.8975830}};
			case 162:
				return {{1000.0000000, -367.1383057}};
			case 163:
				return {{1000.0000000, -367.1704102}};
			case 164:
				return {{1000.0000000, -367.5058594}};
			case 165:
				return {{1000.0000000, -367.5330811}};
			case 166:
				return {{1000.0000000, -367.8172607}};
			case 167:
				return {{1000.0000000, -367.8402100}};
			case 168:
				return {{1000.0000000, -368.0611572}};
			case 169:
				return {{1000.0000000, -368.0804443}};
			case 170:
				return {{1000.0000000, -368.2398682}};
			case 171:
				return {{1000.0000000, -368.2490234}};
			case 172:
				return {{1000.0000000, -368.3774414}};
			case 173:
				return {{1000.0000000, -368.1584473}};
			case 174:
				return {{1000.0000000, -368.4852295}};
			case 175:
				return {{1000.0000000, -368.4898682}};
			case 176:
				return {{1000.0000000, -368.5522461}};
			case 177:
				return {{1000.0000000, -368.5556641}};
			case 178:
				return {{1000.0000000, -368.6008301}};
			case 179:
				return {{1000.0000000, -368.6030273}};
			case 180:
				return {{1000.0000000, -368.6293945}};
			case 181:
				return {{1000.0000000, -368.6307373}};
			case 182:
				return {{1000.0000000, -368.6463623}};
			case 183:
				return {{1000.0000000, -368.6523438}};
			case 184:
				return {{1000.0000000, -368.6516113}};
			case 185:
				return {{1000.0000000, -368.6513672}};
			case 186:
				return {{1000.0000000, -368.6455078}};
			case 187:
				return {{1000.0000000, -368.6450195}};
			case 188:
				return {{1000.0000000, -368.6356201}};
			case 189:
				return {{1000.0000000, -368.6350098}};
			case 190:
				return {{1000.0000000, -368.6240234}};
			case 191:
				return {{1000.0000000, -368.6234131}};
			case 192:
				return {{1000.0000000, -368.6105957}};
			case 193:
				return {{1000.0000000, -368.6097412}};
			case 194:
				return {{1000.0000000, -368.5974121}};
			case 195:
				return {{1000.0000000, -368.5965576}};
			case 196:
				return {{1000.0000000, -368.5834961}};
			case 197:
				return {{1000.0000000, -368.5827637}};
			case 198:
				return {{1000.0000000, -368.5701904}};
			case 199:
				return {{1000.0000000, -368.5695801}};
			case 200:
				return {{1000.0000000, -368.5584717}};
			case 201:
				return {{1000.0000000, -368.5578613}};
			case 202:
				return {{1000.0000000, -368.5469971}};
			case 203:
				return {{1000.0000000, -369.2239990}};
			case 204:
				return {{1000.0000000, -569.6903076}};
			case 205:
				return {{1000.0000000, -597.7822266}};
			case 206:
				return std::nullopt;
			case 207:
				return std::nullopt;
			case 208:
				return std::nullopt;
			case 209:
				return std::nullopt;
			case 210:
				return std::nullopt;
			case 211:
				return std::nullopt;
			case 212:
				return std::nullopt;
			case 213:
				return std::nullopt;
			case 214:
				return std::nullopt;
			case 215:
				return std::nullopt;
			case 216:
				return std::nullopt;
			case 217:
				return std::nullopt;
			case 218:
				return std::nullopt;
			case 219:
				return std::nullopt;
			case 220:
				return std::nullopt;
			case 221:
				return std::nullopt;
			case 222:
				return std::nullopt;
			case 223:
				return std::nullopt;
			case 224:
				return std::nullopt;
			case 225:
				return std::nullopt;
			case 226:
				return std::nullopt;
			case 227:
				return std::nullopt;
			case 228:
				return std::nullopt;
			case 229:
				return std::nullopt;
			case 230:
				return std::nullopt;
			case 231:
				return std::nullopt;
			case 232:
				return std::nullopt;
			case 233:
				return std::nullopt;
			case 234:
				return std::nullopt;
			case 235:
				return std::nullopt;
			case 236:
				return std::nullopt;
			case 237:
				return std::nullopt;
			case 238:
				return std::nullopt;
			case 239:
				return std::nullopt;
			case 240:
				return std::nullopt;
			case 241:
				return std::nullopt;
			case 242:
				return std::nullopt;
			case 243:
				return std::nullopt;
			case 244:
				return std::nullopt;
			case 245:
				return std::nullopt;
			case 246:
				return std::nullopt;
			case 247:
				return std::nullopt;
			case 248:
				return std::nullopt;
			case 249:
				return std::nullopt;
			case 250:
				return std::nullopt;
			case 251:
				return std::nullopt;
			case 252:
				return std::nullopt;
			case 253:
				return std::nullopt;
			case 254:
				return std::nullopt;
			case 255:
				return std::nullopt;
			case 256:
				return std::nullopt;
			case 257:
				return std::nullopt;
			case 258:
				return std::nullopt;
			case 259:
				return std::nullopt;
			case 260:
				return std::nullopt;
			case 261:
				return std::nullopt;
			case 262:
				return std::nullopt;
			case 263:
				return std::nullopt;
			case 264:
				return std::nullopt;
			case 265:
				return std::nullopt;
			case 266:
				return std::nullopt;
			case 267:
				return std::nullopt;
			case 268:
				return std::nullopt;
			case 269:
				return std::nullopt;
			case 270:
				return std::nullopt;
			case 271:
				return std::nullopt;
			case 272:
				return std::nullopt;
			case 273:
				return std::nullopt;
			case 274:
				return std::nullopt;
			case 275:
				return std::nullopt;
			case 276:
				return std::nullopt;
			case 277:
				return std::nullopt;
			case 278:
				return std::nullopt;
			case 279:
				return std::nullopt;
			case 280:
				return std::nullopt;
			case 281:
				return std::nullopt;
			case 282:
				return std::nullopt;
			case 283:
				return std::nullopt;
			case 284:
				return std::nullopt;
			case 285:
				return std::nullopt;
			case 286:
				return std::nullopt;
			case 287:
				return std::nullopt;
			case 288:
				return std::nullopt;
			case 289:
				return std::nullopt;
			case 290:
				return std::nullopt;
			case 291:
				return std::nullopt;
			case 292:
				return std::nullopt;
			case 293:
				return std::nullopt;
			case 294:
				return std::nullopt;
			case 295:
				return std::nullopt;
			case 296:
				return std::nullopt;
			case 297:
				return std::nullopt;
			case 298:
				return std::nullopt;
			case 299:
				return std::nullopt;
			case 300:
				return std::nullopt;
			case 301:
				return std::nullopt;
			case 302:
				return std::nullopt;
			case 303:
				return std::nullopt;
			case 304:
				return std::nullopt;
			case 305:
				return std::nullopt;
			case 306:
				return std::nullopt;
			case 307:
				return std::nullopt;
			case 308:
				return std::nullopt;
			case 309:
				return std::nullopt;
			case 310:
				return std::nullopt;
			case 311:
				return std::nullopt;
			case 312:
				return std::nullopt;
			case 313:
				return std::nullopt;
			case 314:
				return std::nullopt;
			case 315:
				return std::nullopt;
			case 316:
				return std::nullopt;
			case 317:
				return std::nullopt;
			case 318:
				return std::nullopt;
			case 319:
				return std::nullopt;
			case 320:
				return std::nullopt;
			case 321:
				return std::nullopt;
			case 322:
				return std::nullopt;
			case 323:
				return std::nullopt;
			case 324:
				return std::nullopt;
			case 325:
				return std::nullopt;
			case 326:
				return std::nullopt;
			case 327:
				return std::nullopt;
			case 328:
				return std::nullopt;
			case 329:
				return std::nullopt;
			case 330:
				return std::nullopt;
			case 331:
				return std::nullopt;
			case 332:
				return std::nullopt;
			case 333:
				return std::nullopt;
			case 334:
				return std::nullopt;
			case 335:
				return std::nullopt;
			case 336:
				return std::nullopt;
			case 337:
				return std::nullopt;
			case 338:
				return std::nullopt;
			case 339:
				return std::nullopt;
			case 340:
				return std::nullopt;
			case 341:
				return std::nullopt;
			case 342:
				return std::nullopt;
			case 343:
				return std::nullopt;
			case 344:
				return std::nullopt;
			case 345:
				return std::nullopt;
			case 346:
				return std::nullopt;
			case 347:
				return std::nullopt;
			case 348:
				return std::nullopt;
			case 349:
				return std::nullopt;
			case 350:
				return std::nullopt;
			case 351:
				return std::nullopt;
			case 352:
				return std::nullopt;
			case 353:
				return std::nullopt;
			case 354:
				return std::nullopt;
			case 355:
				return std::nullopt;
			case 356:
				return std::nullopt;
			case 357:
				return std::nullopt;
			case 358:
				return std::nullopt;
			case 359:
				return std::nullopt;
			case 360:
				return std::nullopt;
			case 361:
				return std::nullopt;
			case 362:
				return std::nullopt;
			case 363:
				return std::nullopt;
			case 364:
				return std::nullopt;
			case 365:
				return std::nullopt;
			case 366:
				return std::nullopt;
			case 367:
				return std::nullopt;
			case 368:
				return std::nullopt;
			case 369:
				return std::nullopt;
			case 370:
				return std::nullopt;
			case 371:
				return std::nullopt;
			case 372:
				return std::nullopt;
			case 373:
				return std::nullopt;
			case 374:
				return std::nullopt;
			case 375:
				return std::nullopt;
			case 376:
				return std::nullopt;
			case 377:
				return std::nullopt;
			case 378:
				return std::nullopt;
			case 379:
				return std::nullopt;
			case 380:
				return std::nullopt;
			case 381:
				return std::nullopt;
			case 382:
				return std::nullopt;
			case 383:
				return std::nullopt;
			case 384:
				return std::nullopt;
			case 385:
				return std::nullopt;
			case 386:
				return std::nullopt;
			case 387:
				return std::nullopt;
			case 388:
				return std::nullopt;
			case 389:
				return std::nullopt;
			case 390:
				return std::nullopt;
			case 391:
				return std::nullopt;
			case 392:
				return std::nullopt;
			case 393:
				return std::nullopt;
			case 394:
				return std::nullopt;
			case 395:
				return std::nullopt;
			case 396:
				return std::nullopt;
			case 397:
				return std::nullopt;
			case 398:
				return std::nullopt;
			case 399:
				return std::nullopt;
			case 400:
				return std::nullopt;
			case 401:
				return std::nullopt;
			case 402:
				return std::nullopt;
			case 403:
				return std::nullopt;
			case 404:
				return std::nullopt;
			case 405:
				return std::nullopt;
			case 406:
				return std::nullopt;
			case 407:
				return std::nullopt;
			case 408:
				return std::nullopt;
			case 409:
				return std::nullopt;
			case 410:
				return std::nullopt;
			case 411:
				return std::nullopt;
			case 412:
				return std::nullopt;
			case 413:
				return std::nullopt;
			case 414:
				return std::nullopt;
			case 415:
				return std::nullopt;
			case 416:
				return std::nullopt;
			case 417:
				return std::nullopt;
			case 418:
				return std::nullopt;
			case 419:
				return std::nullopt;
			case 420:
				return std::nullopt;
			case 421:
				return std::nullopt;
			case 422:
				return std::nullopt;
			case 423:
				return std::nullopt;
			case 424:
				return std::nullopt;
			case 425:
				return std::nullopt;
			case 426:
				return std::nullopt;
			case 427:
				return std::nullopt;
			case 428:
				return std::nullopt;
			case 429:
				return std::nullopt;
			case 430:
				return std::nullopt;
			case 431:
				return std::nullopt;
			case 432:
				return std::nullopt;
			case 433:
				return std::nullopt;
			case 434:
				return std::nullopt;
			case 435:
				return std::nullopt;
			case 436:
				return std::nullopt;
			case 437:
				return std::nullopt;
			case 438:
				return std::nullopt;
			case 439:
				return std::nullopt;
			case 440:
				return std::nullopt;
			case 441:
				return std::nullopt;
			case 442:
				return std::nullopt;
			case 443:
				return std::nullopt;
			case 444:
				return std::nullopt;
			case 445:
				return std::nullopt;
			case 446:
				return std::nullopt;
			case 447:
				return std::nullopt;
			case 448:
				return std::nullopt;
			case 449:
				return std::nullopt;
			case 450:
				return std::nullopt;
			case 451:
				return std::nullopt;
			case 452:
				return std::nullopt;
			case 453:
				return std::nullopt;
			case 454:
				return std::nullopt;
			case 455:
				return std::nullopt;
			case 456:
				return std::nullopt;
			case 457:
				return std::nullopt;
			case 458:
				return std::nullopt;
			case 459:
				return std::nullopt;
			case 460:
				return std::nullopt;
			case 461:
				return std::nullopt;
			case 462:
				return std::nullopt;
			case 463:
				return std::nullopt;
			case 464:
				return std::nullopt;
			case 465:
				return std::nullopt;
			case 466:
				return std::nullopt;
			case 467:
				return std::nullopt;
			case 468:
				return std::nullopt;
			case 469:
				return std::nullopt;
			case 470:
				return std::nullopt;
			case 471:
				return std::nullopt;
			case 472:
				return std::nullopt;
			case 473:
				return std::nullopt;
			case 474:
				return std::nullopt;
			case 475:
				return std::nullopt;
			case 476:
				return std::nullopt;
			case 477:
				return std::nullopt;
			case 478:
				return std::nullopt;
			case 479:
				return std::nullopt;
			case 480:
				return std::nullopt;
			case 481:
				return std::nullopt;
			case 482:
				return std::nullopt;
			case 483:
				return std::nullopt;
			case 484:
				return std::nullopt;
			case 485:
				return std::nullopt;
			case 486:
				return std::nullopt;
			case 487:
				return std::nullopt;
			case 488:
				return std::nullopt;
			case 489:
				return std::nullopt;
			case 490:
				return std::nullopt;
			case 491:
				return std::nullopt;
			case 492:
				return std::nullopt;
			case 493:
				return std::nullopt;
			case 494:
				return std::nullopt;
			case 495:
				return std::nullopt;
			case 496:
				return std::nullopt;
			case 497:
				return std::nullopt;
			case 498:
				return std::nullopt;
			case 499:
				return std::nullopt;
			case 500:
				return std::nullopt;
			case 501:
				return std::nullopt;
			case 502:
				return std::nullopt;
			case 503:
				return std::nullopt;
			case 504:
				return std::nullopt;
			case 505:
				return std::nullopt;
			case 506:
				return std::nullopt;
			case 507:
				return std::nullopt;
			case 508:
				return std::nullopt;
			case 509:
				return std::nullopt;
			case 510:
				return std::nullopt;
			case 511:
				return std::nullopt;
			case 512:
				return std::nullopt;
			case 513:
				return std::nullopt;
			case 514:
				return std::nullopt;
			case 515:
				return std::nullopt;
			case 516:
				return std::nullopt;
			case 517:
				return std::nullopt;
			case 518:
				return std::nullopt;
			case 519:
				return std::nullopt;
			case 520:
				return std::nullopt;
			case 521:
				return std::nullopt;
			case 522:
				return std::nullopt;
			case 523:
				return std::nullopt;
			case 524:
				return std::nullopt;
			case 525:
				return std::nullopt;
			case 526:
				return std::nullopt;
			case 527:
				return std::nullopt;
			case 528:
				return std::nullopt;
			case 529:
				return std::nullopt;
			case 530:
				return std::nullopt;
			case 531:
				return std::nullopt;
			case 532:
				return std::nullopt;
			case 533:
				return std::nullopt;
			case 534:
				return std::nullopt;
			case 535:
				return std::nullopt;
			case 536:
				return std::nullopt;
			case 537:
				return std::nullopt;
			case 538:
				return std::nullopt;
			case 539:
				return std::nullopt;
			case 540:
				return std::nullopt;
			case 541:
				return std::nullopt;
			case 542:
				return std::nullopt;
			case 543:
				return std::nullopt;
			case 544:
				return std::nullopt;
			case 545:
				return std::nullopt;
			case 546:
				return std::nullopt;
			case 547:
				return std::nullopt;
			case 548:
				return std::nullopt;
			case 549:
				return std::nullopt;
			case 550:
				return std::nullopt;
			case 551:
				return std::nullopt;
			case 552:
				return std::nullopt;
			case 553:
				return std::nullopt;
			case 554:
				return std::nullopt;
			case 555:
				return std::nullopt;
			case 556:
				return std::nullopt;
			case 557:
				return std::nullopt;
			case 558:
				return std::nullopt;
			case 559:
				return std::nullopt;
			case 560:
				return std::nullopt;
			case 561:
				return std::nullopt;
			case 562:
				return std::nullopt;
			case 563:
				return std::nullopt;
			case 564:
				return std::nullopt;
			case 565:
				return std::nullopt;
			case 566:
				return std::nullopt;
			case 567:
				return std::nullopt;
			case 568:
				return std::nullopt;
			case 569:
				return std::nullopt;
			case 570:
				return std::nullopt;
			case 571:
				return std::nullopt;
			case 572:
				return std::nullopt;
			case 573:
				return std::nullopt;
			case 574:
				return std::nullopt;
			case 575:
				return std::nullopt;
			case 576:
				return std::nullopt;
			case 577:
				return std::nullopt;
			case 578:
				return std::nullopt;
			case 579:
				return std::nullopt;
			case 580:
				return std::nullopt;
			case 581:
				return std::nullopt;
			case 582:
				return std::nullopt;
			case 583:
				return std::nullopt;
			case 584:
				return std::nullopt;
			case 585:
				return std::nullopt;
			case 586:
				return std::nullopt;
			case 587:
				return std::nullopt;
			case 588:
				return std::nullopt;
			case 589:
				return std::nullopt;
			case 590:
				return std::nullopt;
			case 591:
				return std::nullopt;
			case 592:
				return std::nullopt;
			case 593:
				return std::nullopt;
			case 594:
				return std::nullopt;
			case 595:
				return std::nullopt;
			case 596:
				return std::nullopt;
			case 597:
				return std::nullopt;
			case 598:
				return std::nullopt;
			case 599:
				return std::nullopt;
			case 600:
				return std::nullopt;
			case 601:
				return std::nullopt;
			case 602:
				return std::nullopt;
			case 603:
				return std::nullopt;
			case 604:
				return std::nullopt;
			case 605:
				return std::nullopt;
			case 606:
				return std::nullopt;
			case 607:
				return std::nullopt;
			case 608:
				return std::nullopt;
			case 609:
				return std::nullopt;
			case 610:
				return std::nullopt;
			case 611:
				return std::nullopt;
			case 612:
				return std::nullopt;
			case 613:
				return std::nullopt;
			case 614:
				return std::nullopt;
			case 615:
				return std::nullopt;
			case 616:
				return std::nullopt;
			case 617:
				return std::nullopt;
			case 618:
				return std::nullopt;
			case 619:
				return std::nullopt;
			case 620:
				return std::nullopt;
			case 621:
				return std::nullopt;
			case 622:
				return std::nullopt;
			case 623:
				return std::nullopt;
			case 624:
				return std::nullopt;
			case 625:
				return std::nullopt;
			case 626:
				return std::nullopt;
			case 627:
				return std::nullopt;
			case 628:
				return std::nullopt;
			case 629:
				return std::nullopt;
			case 630:
				return std::nullopt;
			case 631:
				return std::nullopt;
			case 632:
				return std::nullopt;
			case 633:
				return std::nullopt;
			case 634:
				return std::nullopt;
			case 635:
				return std::nullopt;
			case 636:
				return std::nullopt;
			case 637:
				return std::nullopt;
			case 638:
				return std::nullopt;
			case 639:
				return std::nullopt;
			case 640:
				return std::nullopt;
			case 641:
				return std::nullopt;
			case 642:
				return std::nullopt;
			case 643:
				return std::nullopt;
			case 644:
				return std::nullopt;
			case 645:
				return std::nullopt;
			case 646:
				return std::nullopt;
			case 647:
				return std::nullopt;
			case 648:
				return std::nullopt;
			case 649:
				return std::nullopt;
			case 650:
				return std::nullopt;
			case 651:
				return std::nullopt;
			case 652:
				return std::nullopt;
			case 653:
				return std::nullopt;
			case 654:
				return std::nullopt;
			case 655:
				return std::nullopt;
			case 656:
				return std::nullopt;
			case 657:
				return std::nullopt;
			case 658:
				return std::nullopt;
			case 659:
				return std::nullopt;
			case 660:
				return std::nullopt;
			case 661:
				return std::nullopt;
			case 662:
				return std::nullopt;
			case 663:
				return std::nullopt;
			case 664:
				return std::nullopt;
			case 665:
				return std::nullopt;
			case 666:
				return std::nullopt;
			case 667:
				return std::nullopt;
			case 668:
				return std::nullopt;
			case 669:
				return std::nullopt;
			case 670:
				return std::nullopt;
			case 671:
				return std::nullopt;
			case 672:
				return std::nullopt;
			case 673:
				return std::nullopt;
			case 674:
				return std::nullopt;
			case 675:
				return std::nullopt;
			case 676:
				return std::nullopt;
			case 677:
				return std::nullopt;
			case 678:
				return std::nullopt;
			case 679:
				return std::nullopt;
			case 680:
				return std::nullopt;
			case 681:
				return std::nullopt;
			case 682:
				return std::nullopt;
			case 683:
				return std::nullopt;
			case 684:
				return std::nullopt;
			case 685:
				return std::nullopt;
			case 686:
				return std::nullopt;
			case 687:
				return std::nullopt;
			case 688:
				return std::nullopt;
			case 689:
				return std::nullopt;
			case 690:
				return std::nullopt;
			case 691:
				return std::nullopt;
			case 692:
				return std::nullopt;
			case 693:
				return std::nullopt;
			case 694:
				return std::nullopt;
			case 695:
				return std::nullopt;
			case 696:
				return std::nullopt;
			case 697:
				return std::nullopt;
			case 698:
				return std::nullopt;
			case 699:
				return std::nullopt;
			case 700:
				return std::nullopt;
			case 701:
				return std::nullopt;
			case 702:
				return std::nullopt;
			case 703:
				return std::nullopt;
			case 704:
				return std::nullopt;
			case 705:
				return std::nullopt;
			case 706:
				return std::nullopt;
			case 707:
				return std::nullopt;
			case 708:
				return std::nullopt;
			case 709:
				return std::nullopt;
			case 710:
				return std::nullopt;
			case 711:
				return std::nullopt;
			case 712:
				return std::nullopt;
			case 713:
				return std::nullopt;
			case 714:
				return std::nullopt;
			case 715:
				return std::nullopt;
			case 716:
				return std::nullopt;
			case 717:
				return std::nullopt;
			case 718:
				return std::nullopt;
			case 719:
				return std::nullopt;
			case 720:
				return std::nullopt;
			case 721:
				return std::nullopt;
			case 722:
				return std::nullopt;
			case 723:
				return std::nullopt;
			case 724:
				return std::nullopt;
			case 725:
				return std::nullopt;
			case 726:
				return std::nullopt;
			case 727:
				return std::nullopt;
			case 728:
				return std::nullopt;
			case 729:
				return std::nullopt;
			case 730:
				return std::nullopt;
			case 731:
				return std::nullopt;
			case 732:
				return std::nullopt;
			case 733:
				return std::nullopt;
			case 734:
				return std::nullopt;
			case 735:
				return std::nullopt;
			case 736:
				return std::nullopt;
			case 737:
				return std::nullopt;
			case 738:
				return std::nullopt;
			case 739:
				return std::nullopt;
			case 740:
				return std::nullopt;
			case 741:
				return std::nullopt;
			case 742:
				return std::nullopt;
			case 743:
				return std::nullopt;
			case 744:
				return std::nullopt;
			case 745:
				return std::nullopt;
			case 746:
				return std::nullopt;
			case 747:
				return std::nullopt;
			case 748:
				return std::nullopt;
			case 749:
				return std::nullopt;
			case 750:
				return std::nullopt;
			case 751:
				return std::nullopt;
			case 752:
				return std::nullopt;
			case 753:
				return std::nullopt;
			case 754:
				return std::nullopt;
			case 755:
				return std::nullopt;
			case 756:
				return std::nullopt;
			case 757:
				return std::nullopt;
			case 758:
				return std::nullopt;
			case 759:
				return std::nullopt;
			case 760:
				return std::nullopt;
			case 761:
				return std::nullopt;
			case 762:
				return std::nullopt;
			case 763:
				return std::nullopt;
			case 764:
				return std::nullopt;
			case 765:
				return std::nullopt;
			case 766:
				return std::nullopt;
			case 767:
				return std::nullopt;
			case 768:
				return std::nullopt;
			case 769:
				return std::nullopt;
			case 770:
				return std::nullopt;
			case 771:
				return std::nullopt;
			case 772:
				return std::nullopt;
			case 773:
				return std::nullopt;
			case 774:
				return std::nullopt;
			case 775:
				return std::nullopt;
			case 776:
				return std::nullopt;
			case 777:
				return std::nullopt;
			case 778:
				return std::nullopt;
			case 779:
				return std::nullopt;
			case 780:
				return std::nullopt;
			case 781:
				return std::nullopt;
			case 782:
				return std::nullopt;
			case 783:
				return std::nullopt;
			case 784:
				return std::nullopt;
			case 785:
				return std::nullopt;
			case 786:
				return std::nullopt;
			case 787:
				return std::nullopt;
			case 788:
				return std::nullopt;
			case 789:
				return std::nullopt;
			case 790:
				return std::nullopt;
			case 791:
				return std::nullopt;
			case 792:
				return std::nullopt;
			case 793:
				return std::nullopt;
			case 794:
				return std::nullopt;
			case 795:
				return std::nullopt;
			case 796:
				return std::nullopt;
			case 797:
				return std::nullopt;
			case 798:
				return std::nullopt;
			case 799:
				return std::nullopt;
			case 800:
				return std::nullopt;
			case 801:
				return std::nullopt;
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
				return std::nullopt;
			case 502:
				return std::nullopt;
			case 579:
				return std::nullopt;
			case 582:
				return std::nullopt;
			case 590:
				return std::nullopt;
			case 601:
				return std::nullopt;
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
				return std::nullopt;
			case 502:
				return std::nullopt;
			case 579:
				return std::nullopt;
			case 582:
				return std::nullopt;
			case 590:
				return std::nullopt;
			case 601:
				return std::nullopt;
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
				return std::nullopt;
			case 502:
				return std::nullopt;
			case 579:
				return std::nullopt;
			case 582:
				return std::nullopt;
			case 590:
				return std::nullopt;
			case 601:
				return std::nullopt;
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
				return std::nullopt;
			case 502:
				return std::nullopt;
			case 579:
				return std::nullopt;
			case 582:
				return std::nullopt;
			case 590:
				return std::nullopt;
			case 601:
				return std::nullopt;
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
				return {{1000.0000000f, 1110.8963623}};
			case 301:
				return std::nullopt;
			case 502:
				return std::nullopt;
			case 579:
				return std::nullopt;
			case 582:
				return std::nullopt;
			case 590:
				return std::nullopt;
			case 601:
				return std::nullopt;
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
				return {{1000.0000000f, 1112.4274902}};
			case 301:
				return std::nullopt;
			case 502:
				return std::nullopt;
			case 579:
				return std::nullopt;
			case 582:
				return std::nullopt;
			case 590:
				return std::nullopt;
			case 601:
				return std::nullopt;
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
				return {{1000.0000000f, 1113.4934082}};
			case 301:
				return std::nullopt;
			case 502:
				return std::nullopt;
			case 579:
				return std::nullopt;
			case 582:
				return std::nullopt;
			case 590:
				return std::nullopt;
			case 601:
				return std::nullopt;
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
				return {{995.9793701, 1114.1164551}};
			case 205:
				return {{995.9790649, 1114.1162109}};
			case 206:
				return {{995.9602051, 1114.0809326}};
			case 207:
				return {{995.9481812, 1114.0587158}};
			case 208:
				return {{995.9475098, 1114.0577393}};
			case 209:
				return {{995.9467163, 1114.0566406}};
			case 210:
				return {{995.9371338, 1114.0390625}};
			case 211:
				return {{995.9365845, 1114.0382080}};
			case 212:
				return {{995.9288330, 1114.0227051}};
			case 213:
				return {{995.9283447, 1114.0220947}};
			case 214:
				return {{995.9221191, 1114.0109863}};
			case 215:
				return {{995.9216309, 1114.0104980}};
			case 216:
				return {{995.9155884, 1113.9986572}};
			case 217:
				return {{995.9152222, 1113.9980469}};
			case 218:
				return {{995.9094849, 1113.9879150}};
			case 219:
				return {{995.9091797, 1113.9873047}};
			case 220:
				return {{995.9038086, 1113.9777832}};
			case 221:
				return {{995.9033813, 1113.9772949}};
			case 222:
				return {{995.8975220, 1113.9655762}};
			case 223:
				return {{995.8971558, 1113.9650879}};
			case 224:
				return {{995.8911133, 1113.9539795}};
			case 225:
				return {{995.8906250, 1113.9533691}};
			case 226:
				return {{995.8834839, 1113.9396973}};
			case 227:
				return {{995.8829346, 1113.9390869}};
			case 228:
				return {{995.8745728, 1113.9230957}};
			case 229:
				return {{995.8739624, 1113.9222412}};
			case 230:
				return {{995.8521729, 1113.8828125}};
			case 231:
				return {{995.8367920, 1113.8542480}};
			case 232:
				return {{995.8356934, 1113.8526611}};
			case 233:
				return {{995.8345947, 1113.8510742}};
			case 234:
				return {{995.8157959, 1113.8167725}};
			case 235:
				return {{995.7900391, 1113.7687988}};
			case 236:
				return {{995.7882080, 1113.7661133}};
			case 237:
				return {{995.7559204, 1113.7067871}};
			case 238:
				return {{995.7534180, 1113.7031250}};
			case 239:
				return {{995.7068481, 1113.6181641}};
			case 240:
				return {{995.7033691, 1113.6123047}};
			case 241:
				return {{995.6386719, 1113.4943848}};
			case 242:
				return {{995.6339111, 1113.4859619}};
			case 243:
				return {{995.5488281, 1113.3322754}};
			case 244:
				return {{995.5421143, 1113.3201904}};
			case 245:
				return {{995.4120483, 1113.0864258}};
			case 246:
				return {{995.4022217, 1113.0690918}};
			case 247:
				return {{995.2087402, 1112.7238770}};
			case 248:
				return {{995.1939697, 1112.6987305}};
			case 249:
				return {{994.9188843, 1112.2116699}};
			case 250:
				return {{994.8963013, 1112.1721191}};
			case 251:
				return {{994.4315186, 1111.3586426}};
			case 252:
				return {{994.3945312, 1111.2946777}};
			case 253:
				return {{994.3563232, 1111.2288818}};
			case 254:
				return {{992.1979370, 1107.5391846}};
			case 255:
				return {{989.3889771, 1102.8294678}};
			case 256:
				return {{989.1252441, 1102.3916016}};
			case 257:
				return {{982.1777954, 1090.8796387}};
			case 258:
				return {{981.3657227, 1089.5391846}};
			case 259:
				return {{980.4753418, 1088.0666504}};
			case 260:
				return {{921.4790039, 991.0484619}};
			case 261:
				return {{904.2413940, 962.7258301}};
			case 262:
				return std::nullopt;
			case 263:
				return std::nullopt;
			case 264:
				return std::nullopt;
			case 265:
				return std::nullopt;
			case 266:
				return std::nullopt;
			case 267:
				return std::nullopt;
			case 268:
				return std::nullopt;
			case 269:
				return std::nullopt;
			case 270:
				return std::nullopt;
			case 271:
				return std::nullopt;
			case 272:
				return std::nullopt;
			case 273:
				return std::nullopt;
			case 274:
				return std::nullopt;
			case 275:
				return std::nullopt;
			case 276:
				return std::nullopt;
			case 277:
				return std::nullopt;
			case 278:
				return std::nullopt;
			case 279:
				return std::nullopt;
			case 280:
				return std::nullopt;
			case 281:
				return std::nullopt;
			case 282:
				return std::nullopt;
			case 283:
				return std::nullopt;
			case 284:
				return std::nullopt;
			case 285:
				return std::nullopt;
			case 286:
				return std::nullopt;
			case 287:
				return std::nullopt;
			case 288:
				return std::nullopt;
			case 289:
				return std::nullopt;
			case 290:
				return std::nullopt;
			case 291:
				return std::nullopt;
			case 292:
				return std::nullopt;
			case 293:
				return std::nullopt;
			case 294:
				return std::nullopt;
			case 295:
				return std::nullopt;
			case 296:
				return std::nullopt;
			case 297:
				return std::nullopt;
			case 298:
				return std::nullopt;
			case 299:
				return std::nullopt;
			case 300:
				return std::nullopt;
			case 301:
				return std::nullopt;
			case 302:
				return std::nullopt;
			case 303:
				return std::nullopt;
			case 304:
				return std::nullopt;
			case 305:
				return std::nullopt;
			case 306:
				return std::nullopt;
			case 307:
				return std::nullopt;
			case 308:
				return std::nullopt;
			case 309:
				return std::nullopt;
			case 310:
				return std::nullopt;
			case 311:
				return std::nullopt;
			case 312:
				return std::nullopt;
			case 313:
				return std::nullopt;
			case 314:
				return std::nullopt;
			case 315:
				return std::nullopt;
			case 316:
				return std::nullopt;
			case 317:
				return std::nullopt;
			case 318:
				return std::nullopt;
			case 319:
				return std::nullopt;
			case 320:
				return std::nullopt;
			case 321:
				return std::nullopt;
			case 322:
				return std::nullopt;
			case 323:
				return std::nullopt;
			case 324:
				return std::nullopt;
			case 325:
				return std::nullopt;
			case 326:
				return std::nullopt;
			case 327:
				return std::nullopt;
			case 328:
				return std::nullopt;
			case 329:
				return std::nullopt;
			case 330:
				return std::nullopt;
			case 331:
				return std::nullopt;
			case 332:
				return std::nullopt;
			case 333:
				return std::nullopt;
			case 334:
				return std::nullopt;
			case 335:
				return std::nullopt;
			case 336:
				return std::nullopt;
			case 337:
				return std::nullopt;
			case 338:
				return std::nullopt;
			case 339:
				return std::nullopt;
			case 340:
				return std::nullopt;
			case 341:
				return std::nullopt;
			case 342:
				return std::nullopt;
			case 343:
				return std::nullopt;
			case 344:
				return std::nullopt;
			case 345:
				return std::nullopt;
			case 346:
				return std::nullopt;
			case 347:
				return std::nullopt;
			case 348:
				return std::nullopt;
			case 349:
				return std::nullopt;
			case 350:
				return std::nullopt;
			case 351:
				return std::nullopt;
			case 352:
				return std::nullopt;
			case 353:
				return std::nullopt;
			case 354:
				return std::nullopt;
			case 355:
				return std::nullopt;
			case 356:
				return std::nullopt;
			case 357:
				return std::nullopt;
			case 358:
				return std::nullopt;
			case 359:
				return std::nullopt;
			case 360:
				return std::nullopt;
			case 361:
				return std::nullopt;
			case 362:
				return std::nullopt;
			case 363:
				return std::nullopt;
			case 364:
				return std::nullopt;
			case 365:
				return std::nullopt;
			case 366:
				return std::nullopt;
			case 367:
				return std::nullopt;
			case 368:
				return std::nullopt;
			case 369:
				return std::nullopt;
			case 370:
				return std::nullopt;
			case 371:
				return std::nullopt;
			case 372:
				return std::nullopt;
			case 373:
				return std::nullopt;
			case 374:
				return std::nullopt;
			case 375:
				return std::nullopt;
			case 376:
				return std::nullopt;
			case 377:
				return std::nullopt;
			case 378:
				return std::nullopt;
			case 379:
				return std::nullopt;
			case 380:
				return std::nullopt;
			case 381:
				return std::nullopt;
			case 382:
				return std::nullopt;
			case 383:
				return std::nullopt;
			case 384:
				return std::nullopt;
			case 385:
				return std::nullopt;
			case 386:
				return std::nullopt;
			case 387:
				return std::nullopt;
			case 388:
				return std::nullopt;
			case 389:
				return std::nullopt;
			case 390:
				return std::nullopt;
			case 391:
				return std::nullopt;
			case 392:
				return std::nullopt;
			case 393:
				return std::nullopt;
			case 394:
				return std::nullopt;
			case 395:
				return std::nullopt;
			case 396:
				return std::nullopt;
			case 397:
				return std::nullopt;
			case 398:
				return std::nullopt;
			case 399:
				return std::nullopt;
			case 400:
				return std::nullopt;
			case 401:
				return std::nullopt;
			case 402:
				return std::nullopt;
			case 403:
				return std::nullopt;
			case 404:
				return std::nullopt;
			case 405:
				return std::nullopt;
			case 406:
				return std::nullopt;
			case 407:
				return std::nullopt;
			case 408:
				return std::nullopt;
			case 409:
				return std::nullopt;
			case 410:
				return std::nullopt;
			case 411:
				return std::nullopt;
			case 412:
				return std::nullopt;
			case 413:
				return std::nullopt;
			case 414:
				return std::nullopt;
			case 415:
				return std::nullopt;
			case 416:
				return std::nullopt;
			case 417:
				return std::nullopt;
			case 418:
				return std::nullopt;
			case 419:
				return std::nullopt;
			case 420:
				return std::nullopt;
			case 421:
				return std::nullopt;
			case 422:
				return std::nullopt;
			case 423:
				return std::nullopt;
			case 424:
				return std::nullopt;
			case 425:
				return std::nullopt;
			case 426:
				return std::nullopt;
			case 427:
				return std::nullopt;
			case 428:
				return std::nullopt;
			case 429:
				return std::nullopt;
			case 430:
				return std::nullopt;
			case 431:
				return std::nullopt;
			case 432:
				return std::nullopt;
			case 433:
				return std::nullopt;
			case 434:
				return std::nullopt;
			case 435:
				return std::nullopt;
			case 436:
				return std::nullopt;
			case 437:
				return std::nullopt;
			case 438:
				return std::nullopt;
			case 439:
				return std::nullopt;
			case 440:
				return std::nullopt;
			case 441:
				return std::nullopt;
			case 442:
				return std::nullopt;
			case 443:
				return std::nullopt;
			case 444:
				return std::nullopt;
			case 445:
				return std::nullopt;
			case 446:
				return std::nullopt;
			case 447:
				return std::nullopt;
			case 448:
				return std::nullopt;
			case 449:
				return std::nullopt;
			case 450:
				return std::nullopt;
			case 451:
				return std::nullopt;
			case 452:
				return std::nullopt;
			case 453:
				return std::nullopt;
			case 454:
				return std::nullopt;
			case 455:
				return std::nullopt;
			case 456:
				return std::nullopt;
			case 457:
				return std::nullopt;
			case 458:
				return std::nullopt;
			case 459:
				return std::nullopt;
			case 460:
				return std::nullopt;
			case 461:
				return std::nullopt;
			case 462:
				return std::nullopt;
			case 463:
				return std::nullopt;
			case 464:
				return std::nullopt;
			case 465:
				return std::nullopt;
			case 466:
				return std::nullopt;
			case 467:
				return std::nullopt;
			case 468:
				return std::nullopt;
			case 469:
				return std::nullopt;
			case 470:
				return std::nullopt;
			case 471:
				return std::nullopt;
			case 472:
				return std::nullopt;
			case 473:
				return std::nullopt;
			case 474:
				return std::nullopt;
			case 475:
				return std::nullopt;
			case 476:
				return std::nullopt;
			case 477:
				return std::nullopt;
			case 478:
				return std::nullopt;
			case 479:
				return std::nullopt;
			case 480:
				return std::nullopt;
			case 481:
				return std::nullopt;
			case 482:
				return std::nullopt;
			case 483:
				return std::nullopt;
			case 484:
				return std::nullopt;
			case 485:
				return std::nullopt;
			case 486:
				return std::nullopt;
			case 487:
				return std::nullopt;
			case 488:
				return std::nullopt;
			case 489:
				return std::nullopt;
			case 490:
				return std::nullopt;
			case 491:
				return std::nullopt;
			case 492:
				return std::nullopt;
			case 493:
				return std::nullopt;
			case 494:
				return std::nullopt;
			case 495:
				return std::nullopt;
			case 496:
				return std::nullopt;
			case 497:
				return std::nullopt;
			case 498:
				return std::nullopt;
			case 499:
				return std::nullopt;
			case 500:
				return std::nullopt;
			case 501:
				return std::nullopt;
			case 502:
				return std::nullopt;
			case 503:
				return std::nullopt;
			case 504:
				return std::nullopt;
			case 505:
				return std::nullopt;
			case 506:
				return std::nullopt;
			case 507:
				return std::nullopt;
			case 508:
				return std::nullopt;
			case 509:
				return std::nullopt;
			case 510:
				return std::nullopt;
			case 511:
				return std::nullopt;
			case 512:
				return std::nullopt;
			case 513:
				return std::nullopt;
			case 514:
				return std::nullopt;
			case 515:
				return std::nullopt;
			case 516:
				return std::nullopt;
			case 517:
				return std::nullopt;
			case 518:
				return std::nullopt;
			case 519:
				return std::nullopt;
			case 520:
				return std::nullopt;
			case 521:
				return std::nullopt;
			case 522:
				return std::nullopt;
			case 523:
				return std::nullopt;
			case 524:
				return std::nullopt;
			case 525:
				return std::nullopt;
			case 526:
				return std::nullopt;
			case 527:
				return std::nullopt;
			case 528:
				return std::nullopt;
			case 529:
				return std::nullopt;
			case 530:
				return std::nullopt;
			case 531:
				return std::nullopt;
			case 532:
				return std::nullopt;
			case 533:
				return std::nullopt;
			case 534:
				return std::nullopt;
			case 535:
				return std::nullopt;
			case 536:
				return std::nullopt;
			case 537:
				return std::nullopt;
			case 538:
				return std::nullopt;
			case 539:
				return std::nullopt;
			case 540:
				return std::nullopt;
			case 541:
				return std::nullopt;
			case 542:
				return std::nullopt;
			case 543:
				return std::nullopt;
			case 544:
				return std::nullopt;
			case 545:
				return std::nullopt;
			case 546:
				return std::nullopt;
			case 547:
				return std::nullopt;
			case 548:
				return std::nullopt;
			case 549:
				return std::nullopt;
			case 550:
				return std::nullopt;
			case 551:
				return std::nullopt;
			case 552:
				return std::nullopt;
			case 553:
				return std::nullopt;
			case 554:
				return std::nullopt;
			case 555:
				return std::nullopt;
			case 556:
				return std::nullopt;
			case 557:
				return std::nullopt;
			case 558:
				return std::nullopt;
			case 559:
				return std::nullopt;
			case 560:
				return std::nullopt;
			case 561:
				return std::nullopt;
			case 562:
				return std::nullopt;
			case 563:
				return std::nullopt;
			case 564:
				return std::nullopt;
			case 565:
				return std::nullopt;
			case 566:
				return std::nullopt;
			case 567:
				return std::nullopt;
			case 568:
				return std::nullopt;
			case 569:
				return std::nullopt;
			case 570:
				return std::nullopt;
			case 571:
				return std::nullopt;
			case 572:
				return std::nullopt;
			case 573:
				return std::nullopt;
			case 574:
				return std::nullopt;
			case 575:
				return std::nullopt;
			case 576:
				return std::nullopt;
			case 577:
				return std::nullopt;
			case 578:
				return std::nullopt;
			case 579:
				return std::nullopt;
			case 580:
				return std::nullopt;
			case 581:
				return std::nullopt;
			case 582:
				return std::nullopt;
			case 583:
				return std::nullopt;
			case 584:
				return std::nullopt;
			case 585:
				return std::nullopt;
			case 586:
				return std::nullopt;
			case 587:
				return std::nullopt;
			case 588:
				return std::nullopt;
			case 589:
				return std::nullopt;
			case 590:
				return std::nullopt;
			case 591:
				return std::nullopt;
			case 592:
				return std::nullopt;
			case 593:
				return std::nullopt;
			case 594:
				return std::nullopt;
			case 595:
				return std::nullopt;
			case 596:
				return std::nullopt;
			case 597:
				return std::nullopt;
			case 598:
				return std::nullopt;
			case 599:
				return std::nullopt;
			case 600:
				return std::nullopt;
			case 601:
				return std::nullopt;
			case 602:
				return std::nullopt;
			case 603:
				return std::nullopt;
			case 604:
				return std::nullopt;
			case 605:
				return std::nullopt;
			case 606:
				return std::nullopt;
			case 607:
				return std::nullopt;
			case 608:
				return std::nullopt;
			case 609:
				return std::nullopt;
			case 610:
				return std::nullopt;
			case 611:
				return std::nullopt;
			case 612:
				return std::nullopt;
			case 613:
				return std::nullopt;
			case 614:
				return std::nullopt;
			case 615:
				return std::nullopt;
			case 616:
				return std::nullopt;
			case 617:
				return std::nullopt;
			case 618:
				return std::nullopt;
			case 619:
				return std::nullopt;
			case 620:
				return std::nullopt;
			case 621:
				return std::nullopt;
			case 622:
				return std::nullopt;
			case 623:
				return std::nullopt;
			case 624:
				return std::nullopt;
			case 625:
				return std::nullopt;
			case 626:
				return std::nullopt;
			case 627:
				return std::nullopt;
			case 628:
				return std::nullopt;
			case 629:
				return std::nullopt;
			case 630:
				return std::nullopt;
			case 631:
				return std::nullopt;
			case 632:
				return std::nullopt;
			case 633:
				return std::nullopt;
			case 634:
				return std::nullopt;
			case 635:
				return std::nullopt;
			case 636:
				return std::nullopt;
			case 637:
				return std::nullopt;
			case 638:
				return std::nullopt;
			case 639:
				return std::nullopt;
			case 640:
				return std::nullopt;
			case 641:
				return std::nullopt;
			case 642:
				return std::nullopt;
			case 643:
				return std::nullopt;
			case 644:
				return std::nullopt;
			case 645:
				return std::nullopt;
			case 646:
				return std::nullopt;
			case 647:
				return std::nullopt;
			case 648:
				return std::nullopt;
			case 649:
				return std::nullopt;
			case 650:
				return std::nullopt;
			case 651:
				return std::nullopt;
			case 652:
				return std::nullopt;
			case 653:
				return std::nullopt;
			case 654:
				return std::nullopt;
			case 655:
				return std::nullopt;
			case 656:
				return std::nullopt;
			case 657:
				return std::nullopt;
			case 658:
				return std::nullopt;
			case 659:
				return std::nullopt;
			case 660:
				return std::nullopt;
			case 661:
				return std::nullopt;
			case 662:
				return std::nullopt;
			case 663:
				return std::nullopt;
			case 664:
				return std::nullopt;
			case 665:
				return std::nullopt;
			case 666:
				return std::nullopt;
			case 667:
				return std::nullopt;
			case 668:
				return std::nullopt;
			case 669:
				return std::nullopt;
			case 670:
				return std::nullopt;
			case 671:
				return std::nullopt;
			case 672:
				return std::nullopt;
			case 673:
				return std::nullopt;
			case 674:
				return std::nullopt;
			case 675:
				return std::nullopt;
			case 676:
				return std::nullopt;
			case 677:
				return std::nullopt;
			case 678:
				return std::nullopt;
			case 679:
				return std::nullopt;
			case 680:
				return std::nullopt;
			case 681:
				return std::nullopt;
			case 682:
				return std::nullopt;
			case 683:
				return std::nullopt;
			case 684:
				return std::nullopt;
			case 685:
				return std::nullopt;
			case 686:
				return std::nullopt;
			case 687:
				return std::nullopt;
			case 688:
				return std::nullopt;
			case 689:
				return std::nullopt;
			case 690:
				return std::nullopt;
			case 691:
				return std::nullopt;
			case 692:
				return std::nullopt;
			case 693:
				return std::nullopt;
			case 694:
				return std::nullopt;
			case 695:
				return std::nullopt;
			case 696:
				return std::nullopt;
			case 697:
				return std::nullopt;
			case 698:
				return std::nullopt;
			case 699:
				return std::nullopt;
			case 700:
				return std::nullopt;
			case 701:
				return std::nullopt;
			case 702:
				return std::nullopt;
			case 703:
				return std::nullopt;
			case 704:
				return std::nullopt;
			case 705:
				return std::nullopt;
			case 706:
				return std::nullopt;
			case 707:
				return std::nullopt;
			case 708:
				return std::nullopt;
			case 709:
				return std::nullopt;
			case 710:
				return std::nullopt;
			case 711:
				return std::nullopt;
			case 712:
				return std::nullopt;
			case 713:
				return std::nullopt;
			case 714:
				return std::nullopt;
			case 715:
				return std::nullopt;
			case 716:
				return std::nullopt;
			case 717:
				return std::nullopt;
			case 718:
				return std::nullopt;
			case 719:
				return std::nullopt;
			case 720:
				return std::nullopt;
			case 721:
				return std::nullopt;
			case 722:
				return std::nullopt;
			case 723:
				return std::nullopt;
			case 724:
				return std::nullopt;
			case 725:
				return std::nullopt;
			case 726:
				return std::nullopt;
			case 727:
				return std::nullopt;
			case 728:
				return std::nullopt;
			case 729:
				return std::nullopt;
			case 730:
				return std::nullopt;
			case 731:
				return std::nullopt;
			case 732:
				return std::nullopt;
			case 733:
				return std::nullopt;
			case 734:
				return std::nullopt;
			case 735:
				return std::nullopt;
			case 736:
				return std::nullopt;
			case 737:
				return std::nullopt;
			case 738:
				return std::nullopt;
			case 739:
				return std::nullopt;
			case 740:
				return std::nullopt;
			case 741:
				return std::nullopt;
			case 742:
				return std::nullopt;
			case 743:
				return std::nullopt;
			case 744:
				return std::nullopt;
			case 745:
				return std::nullopt;
			case 746:
				return std::nullopt;
			case 747:
				return std::nullopt;
			case 748:
				return std::nullopt;
			case 749:
				return std::nullopt;
			case 750:
				return std::nullopt;
			case 751:
				return std::nullopt;
			case 752:
				return std::nullopt;
			case 753:
				return std::nullopt;
			case 754:
				return std::nullopt;
			case 755:
				return std::nullopt;
			case 756:
				return std::nullopt;
			case 757:
				return std::nullopt;
			case 758:
				return std::nullopt;
			case 759:
				return std::nullopt;
			case 760:
				return std::nullopt;
			case 761:
				return std::nullopt;
			case 762:
				return std::nullopt;
			case 763:
				return std::nullopt;
			case 764:
				return std::nullopt;
			case 765:
				return std::nullopt;
			case 766:
				return std::nullopt;
			case 767:
				return std::nullopt;
			case 768:
				return std::nullopt;
			case 769:
				return std::nullopt;
			case 770:
				return std::nullopt;
			case 771:
				return std::nullopt;
			case 772:
				return std::nullopt;
			case 773:
				return std::nullopt;
			case 774:
				return std::nullopt;
			case 775:
				return std::nullopt;
			case 776:
				return std::nullopt;
			case 777:
				return std::nullopt;
			case 778:
				return std::nullopt;
			case 779:
				return std::nullopt;
			case 780:
				return std::nullopt;
			case 781:
				return std::nullopt;
			case 782:
				return std::nullopt;
			case 783:
				return std::nullopt;
			case 784:
				return std::nullopt;
			case 785:
				return std::nullopt;
			case 786:
				return std::nullopt;
			case 787:
				return std::nullopt;
			case 788:
				return std::nullopt;
			case 789:
				return std::nullopt;
			case 790:
				return std::nullopt;
			case 791:
				return std::nullopt;
			case 792:
				return std::nullopt;
			case 793:
				return std::nullopt;
			case 794:
				return std::nullopt;
			case 795:
				return std::nullopt;
			case 796:
				return std::nullopt;
			case 797:
				return std::nullopt;
			case 798:
				return std::nullopt;
			case 799:
				return std::nullopt;
			case 800:
				return std::nullopt;
			case 801:
				return std::nullopt;
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		assert(false); // Shouldn't be any unaccounted raycasts
		return std::nullopt;
	}
};
// NOLINTEND(bugprone-branch-clone, google-readability-function-size)
