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

class TiltUpDownMockAction final: public MockAction
{
	static constexpr uint32_t k_Start1 = k_StabilizeFrames + 1;
	static constexpr uint32_t k_End1 = k_StabilizeFrames + k_InteractionFrames - 1;
	static constexpr uint32_t k_Start2 = k_StabilizeFrames + k_InteractionFrames + k_StabilizeFrames + 1;
	static constexpr uint32_t k_End2 = k_StabilizeFrames + k_InteractionFrames + k_StabilizeFrames + k_InteractionFrames - 1;

public:
	~TiltUpDownMockAction() final = default;

	[[nodiscard]] bool GetBindable(openblack::input::BindableActionMap action) const final
	{
		using openblack::input::BindableActionMap;
		if (frameNumber >= k_Start1 && frameNumber <= k_End1)
		{
			return (static_cast<uint32_t>(action) & (static_cast<uint32_t>(BindableActionMap::ROTATE_ON) |
			                                         static_cast<uint32_t>(BindableActionMap::MOVE_BACKWARDS))) != 0;
		}

		if (frameNumber >= k_Start2 && frameNumber <= k_End2)
		{
			return (static_cast<uint32_t>(action) & (static_cast<uint32_t>(BindableActionMap::ROTATE_ON) |
			                                         static_cast<uint32_t>(BindableActionMap::MOVE_FORWARDS))) != 0;
		}

		return false;
	}
};

// NOLINTBEGIN(bugprone-branch-clone, google-readability-function-size, readability-function-size)
class TiltUpDownMockDynamicsSystem: public MockDynamicsSystem
{
public:
	TiltUpDownMockDynamicsSystem() = default;

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
				return {{1000.0, -369.2239990}};
			case 204:
				return {{1000.0, -569.6903076}};
			case 205:
				return {{1000.0, -597.7822266}};
			case 633:
				return {{1000.0, -93.2309570}};
			case 634:
				return {{1000.0, 6.1534119}};
			case 635:
				return {{1000.0, 793.9700317}};
			case 636:
				return {{1000.0, 803.9315796}};
			case 637:
				return {{1000.0, 812.3620605}};
			case 638:
				return {{1000.0, 866.0405884}};
			case 639:
				return {{1000.0, 867.5985107}};
			case 640:
				return {{1000.0, 869.0427246}};
			case 641:
				return {{1000.0, 882.8950195}};
			case 642:
				return {{1000.0, 883.4082031}};
			case 643:
				return {{1000.0, 888.8662720}};
			case 644:
				return {{1000.0, 889.1212158}};
			case 645:
				return {{1000.0, 889.3684082}};
			case 646:
				return {{1000.0, 892.2231445}};
			case 647:
				return {{1000.0, 892.3505859}};
			case 648:
				return {{1000.0, 892.4707642}};
			case 649:
				return {{1000.0, 893.9665527}};
			case 650:
				return {{1000.0, 894.0292969}};
			case 651:
				return {{1000.0, 894.8074951}};
			case 652:
				return {{1000.0, 894.8489990}};
			case 653:
				return {{1000.0, 894.8835449}};
			case 654:
				return {{1000.0, 895.3118286}};
			case 655:
				return {{1000.0, 895.3325195}};
			case 656:
				return {{1000.0, 895.5184326}};
			case 657:
				return {{1000.0, 895.5184937}};
			case 658:
				return {{1000.0, 895.5528564}};
			case 659:
				return {{1000.0, 895.4908447}};
			case 660:
				return {{1000.0, 895.3738403}};
			case 661:
				return {{1000.0, 895.3738403}};
			case 662:
				return {{1000.0, 895.3601074}};
			case 663:
				return {{1000.0, 895.1876831}};
			case 664:
				return {{1000.0, 895.1738892}};
			case 665:
				return {{1000.0, 894.9873047}};
			case 666:
				return {{1000.0, 894.9666748}};
			case 667:
				return {{1000.0, 894.9597168}};
			case 668:
				return {{1000.0, 894.7589722}};
			case 669:
				return {{1000.0, 894.7451782}};
			case 670:
				return {{1000.0, 894.7382202}};
			case 671:
				return {{1000.0, 894.5301514}};
			case 672:
				return {{1000.0, 894.5162964}};
			case 673:
				return {{1000.0, 894.3146973}};
			case 674:
				return {{1000.0, 894.3009033}};
			case 675:
				return {{1000.0, 894.2939453}};
			case 676:
				return {{1000.0, 894.1059570}};
			case 677:
				return {{1000.0, 894.0920410}};
			case 678:
				return {{1000.0, 894.0850830}};
			case 679:
				return {{1000.0, 893.9037476}};
			case 680:
				return {{1000.0, 893.8967896}};
			case 681:
				return {{1000.0, 893.7360840}};
			case 682:
				return {{1000.0, 893.7221680}};
			case 683:
				return {{1000.0, 893.7151489}};
			case 684:
				return {{1000.0, 893.5681763}};
			case 685:
				return {{1000.0, 893.5612793}};
			case 686:
				return {{1000.0, 893.4281006}};
			case 687:
				return {{1000.0, 893.4141846}};
			case 688:
				return {{1000.0, 893.3088379}};
			case 689:
				return {{1000.0, 893.2036133}};
			case 690:
				return {{1000.0, 893.1192627}};
			case 691:
				return {{1000.0, 893.1122437}};
			case 692:
				return {{1000.0, 893.1052246}};
			case 693:
				return {{1000.0, 893.0349121}};
			case 694:
				return {{1000.0, 893.0208740}};
			case 695:
				return {{1000.0, 892.9645386}};
			case 696:
				return {{1000.0, 892.9575195}};
			case 697:
				return {{1000.0, 892.9574585}};
			case 698:
				return {{1000.0, 892.9082642}};
			case 699:
				return {{1000.0, 892.9011230}};
			case 700:
				return {{1000.0, 892.8589478}};
			case 701:
				return {{1000.0, 892.8518677}};
			case 702:
				return {{1000.0, 892.8167114}};
			case 703:
				return {{1000.0, 892.8167114}};
			case 704:
				return {{1000.0, 892.8095703}};
			case 705:
				return {{1000.0, 892.7884521}};
			case 706:
				return {{1000.0, 892.7884521}};
			case 707:
				return {{1000.0, 892.7884521}};
			case 708:
				return {{1000.0, 892.7602539}};
			case 709:
				return {{1000.0, 892.7602539}};
			case 710:
				return {{1000.0, 892.7320557}};
			case 711:
				return {{1000.0, 892.7320557}};
			case 712:
				return {{1000.0, 892.7320557}};
			case 713:
				return {{1000.0, 892.7250977}};
			case 714:
				return {{1000.0, 892.7250977}};
			case 715:
				return {{1000.0, 892.7250977}};
			case 716:
				return {{1000.0, 892.7109375}};
			case 717:
				return {{1000.0, 892.7109375}};
			case 718:
				return {{1000.0, 892.7038574}};
			case 719:
				return {{1000.0, 892.7038574}};
			case 720:
				return {{1000.0, 892.6898193}};
			case 721:
				return {{1000.0, 892.6898193}};
			case 722:
				return {{1000.0, 892.6898193}};
			case 723:
				return {{1000.0, 892.6897583}};
			case 724:
				return {{1000.0, 892.6897583}};
			case 725:
				return {{1000.0, 892.6897583}};
			case 726:
				return {{1000.0, 892.6897583}};
			case 727:
				return {{1000.0, 892.6897583}};
			case 728:
				return {{1000.0, 892.6897583}};
			case 729:
				return {{1000.0, 892.6897583}};
			case 730:
				return {{1000.0, 892.6897583}};
			case 731:
				return {{1000.0, 892.6897583}};
			case 732:
				return {{1000.0, 892.6897583}};
			case 733:
				return {{1000.0, 892.6897583}};
			case 734:
				return {{1000.0, 892.6897583}};
			case 735:
				return {{1000.0, 892.6897583}};
			case 736:
				return {{1000.0, 892.6897583}};
			case 737:
				return {{1000.0, 892.6897583}};
			case 738:
				return {{1000.0, 892.6897583}};
			case 739:
				return {{1000.0, 892.6897583}};
			case 740:
				return {{1000.0, 892.6897583}};
			case 741:
				return {{1000.0, 892.6897583}};
			case 742:
				return {{1000.0, 892.6897583}};
			case 743:
				return {{1000.0, 892.6897583}};
			case 744:
				return {{1000.0, 892.6897583}};
			case 745:
				return {{1000.0, 892.6897583}};
			case 746:
				return {{1000.0, 892.6897583}};
			case 747:
				return {{1000.0, 892.6897583}};
			case 748:
				return {{1000.0, 892.6898193}};
			case 749:
				return {{1000.0, 892.6898193}};
			case 750:
				return {{1000.0, 892.6898193}};
			case 751:
				return {{1000.0, 892.6898193}};
			case 752:
				return {{1000.0, 892.6898193}};
			case 753:
				return {{1000.0, 892.6898193}};
			case 754:
				return {{1000.0, 892.6898193}};
			case 755:
				return {{1000.0, 892.6898193}};
			case 756:
				return {{1000.0, 892.6898193}};
			case 757:
				return {{1000.0, 892.6898193}};
			case 758:
				return {{1000.0, 892.6898193}};
			case 759:
				return {{1000.0, 892.6898193}};
			case 760:
				return {{1000.0, 892.6898193}};
			case 761:
				return {{1000.0, 892.6898193}};
			case 762:
				return {{1000.0, 892.7038574}};
			case 763:
				return {{1000.0, 892.7038574}};
			case 764:
				return {{1000.0, 892.7038574}};
			case 765:
				return {{1000.0, 892.7038574}};
			case 766:
				return {{1000.0, 892.7038574}};
			case 767:
				return {{1000.0, 892.7038574}};
			case 768:
				return {{1000.0, 892.7038574}};
			case 769:
				return {{1000.0, 892.7038574}};
			case 770:
				return {{1000.0, 892.7038574}};
			case 771:
				return {{1000.0, 892.7038574}};
			case 772:
				return {{1000.0, 892.7038574}};
			case 773:
				return {{1000.0, 892.7038574}};
			case 774:
				return {{1000.0, 892.7038574}};
			case 775:
				return {{1000.0, 892.7038574}};
			case 776:
				return {{1000.0, 892.7038574}};
			case 777:
				return {{1000.0, 892.7038574}};
			case 778:
				return {{1000.0, 892.7038574}};
			case 779:
				return {{1000.0, 892.7038574}};
			case 780:
				return {{1000.0, 892.7038574}};
			case 781:
				return {{1000.0, 892.7038574}};
			case 782:
				return {{1000.0, 892.7038574}};
			case 783:
				return {{1000.0, 892.7038574}};
			case 784:
				return {{1000.0, 892.7038574}};
			case 785:
				return {{1000.0, 892.7038574}};
			case 786:
				return {{1000.0, 892.7038574}};
			case 787:
				return {{1000.0, 892.7038574}};
			case 788:
				return {{1000.0, 892.7038574}};
			case 789:
				return {{1000.0, 892.7038574}};
			case 790:
				return {{1000.0, 892.7038574}};
			case 791:
				return {{1000.0, 892.7038574}};
			case 792:
				return {{1000.0, 892.7038574}};
			case 793:
				return {{1000.0, 892.7038574}};
			case 794:
				return {{1000.0, 892.7038574}};
			case 795:
				return {{1000.0, 892.7038574}};
			case 796:
				return {{1000.0, 892.7038574}};
			case 797:
				return {{1000.0, 892.7038574}};
			case 798:
				return {{1000.0, 892.7038574}};
			case 799:
				return {{1000.0, 892.7038574}};
			case 800:
				return {{1000.0, 892.7038574}};
			}
			return std::nullopt;
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
			case 601:
				return {{1000.0, 1053.9960938}};
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
				return std::nullopt;
			case 502:
				return std::nullopt;
			case 601:
				return {{1000.0, 1092.4531250}};
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
				return std::nullopt;
			case 502:
				return std::nullopt;
			case 601:
				return {{1000.0, 1102.6950684}};
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
				return std::nullopt;
			case 502:
				return std::nullopt;
			case 601:
				return {{1000.0, 1107.2592773}};
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
				return std::nullopt;
			case 502:
				return std::nullopt;
			case 601:
				return {{1000.0, 1109.9497070}};
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
				return std::nullopt;
			case 502:
				return std::nullopt;
			case 601:
				return {{1000.0, 1111.6485596}};
			}
			assert(false); // Shouldn't be any unaccounted raycasts
		}
		else if (screenCoord == k_MockMousePos)
		{
			switch (frameNumber)
			{
			case 0:
				return {{15.814184, -13.421215}};
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
			case 530:
				return {{-727.4552612, -1717.5805664}};
			case 531:
				return {{-105.1952515, -695.4335938}};
			case 532:
				return {{188.3721924, -213.2091064}};
			case 533:
				return {{849.3157349, 872.5158081}};
			case 534:
				return {{856.7941284, 884.7982788}};
			case 535:
				return {{863.5875854, 895.9558105}};
			case 536:
				return {{946.0845337, 1031.4876709}};
			case 537:
				return {{947.1395264, 1033.2194824}};
			case 538:
				return {{959.9420166, 1054.2659912}};
			case 539:
				return {{960.5493774, 1055.2683105}};
			case 540:
				return {{961.1402588, 1056.2376709}};
			case 541:
				return {{968.3579102, 1068.1077881}};
			case 542:
				return {{968.7496948, 1068.7502441}};
			case 543:
				return {{969.1321411, 1069.3907471}};
			case 544:
				return {{974.2493286, 1077.8066406}};
			case 545:
				return {{974.5159912, 1078.2435303}};
			case 546:
				return {{974.7772827, 1078.6787109}};
			case 547:
				return {{978.1718750, 1084.2679443}};
			case 548:
				return {{978.3670044, 1084.5938721}};
			case 549:
				return {{978.5592651, 1084.9083252}};
			case 550:
				return {{981.2476807, 1089.3433838}};
			case 551:
				return {{981.3939819, 1089.5854492}};
			case 552:
				return {{983.4736328, 1093.0202637}};
			case 553:
				return {{983.5885620, 1093.2104492}};
			case 554:
				return {{985.1468506, 1095.7860107}};
			case 555:
				return {{985.2405396, 1095.9409180}};
			case 556:
				return {{985.3331909, 1096.0960693}};
			case 557:
				return {{986.6750488, 1098.3193359}};
			case 558:
				return {{986.7508545, 1098.4426270}};
			case 559:
				return {{986.8258667, 1098.5684814}};
			case 560:
				return {{987.9229736, 1100.3895264}};
			case 561:
				return {{987.9854736, 1100.4926758}};
			case 562:
				return {{988.0474243, 1100.5966797}};
			case 563:
				return {{988.9065552, 1102.0279541}};
			case 564:
				return {{988.9593506, 1102.1151123}};
			case 565:
				return {{989.0117798, 1102.2016602}};
			case 566:
				return {{989.7881470, 1103.4971924}};
			case 567:
				return {{989.8328857, 1103.5710449}};
			case 568:
				return {{990.4614258, 1104.6221924}};
			case 569:
				return {{991.0506592, 1105.6092529}};
			case 570:
				return {{991.5699463, 1106.4816895}};
			case 571:
				return {{991.6003418, 1106.5317383}};
			case 572:
				return {{991.6304932, 1106.5837402}};
			case 573:
				return {{992.0584717, 1107.3049316}};
			case 574:
				return {{992.0854492, 1107.3491211}};
			case 575:
				return {{992.1121216, 1107.3962402}};
			case 576:
				return {{992.5164795, 1108.0799561}};
			case 577:
				return {{992.5402832, 1108.1190186}};
			case 578:
				return {{992.5639648, 1108.1599121}};
			case 579:
				return {{992.9230957, 1108.7702637}};
			case 580:
				return {{992.9443359, 1108.8049316}};
			case 581:
				return {{992.9653931, 1108.8414307}};
			case 582:
				return {{993.2672119, 1109.3558350}};
			case 583:
				return {{993.2864380, 1109.3870850}};
			case 584:
				return {{993.5782471, 1109.8869629}};
			case 585:
				return {{993.5955200, 1109.9172363}};
			case 586:
				return {{993.6127930, 1109.9460449}};
			case 587:
				return {{993.8604736, 1110.3703613}};
			case 588:
				return {{993.8763428, 1110.3984375}};
			case 589:
				return {{993.8919678, 1110.4266357}};
			case 590:
				return {{994.1323853, 1110.8404541}};
			case 591:
				return {{994.1467285, 1110.8653564}};
			case 592:
				return {{994.1608887, 1110.8901367}};
			case 593:
				return {{994.3798828, 1111.2694092}};
			case 594:
				return {{994.3929443, 1111.2922363}};
			case 595:
				return {{994.4058838, 1111.3148193}};
			case 596:
				return {{994.5941162, 1111.6417236}};
			case 597:
				return {{994.6062012, 1111.6627197}};
			case 598:
				return {{994.6181641, 1111.6835938}};
			case 599:
				return {{994.8027954, 1112.0075684}};
			case 600:
				return {{994.8138428, 1112.0268555}};
			case 601:
				return {{995.1324463, 1112.5892334}};
			case 602:
				return {{995.1419067, 1112.6057129}};
			case 603:
				return {{995.2728271, 1112.8375244}};
			case 604:
				return {{995.2800293, 1112.8508301}};
			case 605:
				return {{995.3754272, 1113.0214844}};
			case 606:
				return {{995.3812256, 1113.0314941}};
			case 607:
				return {{995.3869019, 1113.0419922}};
			case 608:
				return {{995.4697266, 1113.1895752}};
			case 609:
				return {{995.4743652, 1113.1979980}};
			case 610:
				return {{995.5449219, 1113.3254395}};
			case 611:
				return {{995.5489502, 1113.3323975}};
			case 612:
				return {{995.6069336, 1113.4375000}};
			case 613:
				return {{995.6106567, 1113.4442139}};
			case 614:
				return {{995.6661987, 1113.5439453}};
			case 615:
				return {{995.6694946, 1113.5495605}};
			case 616:
				return {{995.6727295, 1113.5568848}};
			case 617:
				return {{995.7229614, 1113.6469727}};
			case 618:
				return {{995.7259521, 1113.6524658}};
			case 619:
				return {{995.7288818, 1113.6575928}};
			case 620:
				return {{995.7717896, 1113.7363281}};
			case 621:
				return {{995.7745361, 1113.7410889}};
			case 622:
				return {{995.8166504, 1113.8178711}};
			case 623:
				return {{995.8191528, 1113.8226318}};
			case 624:
				return {{995.8216553, 1113.8270264}};
			case 625:
				return {{995.8576660, 1113.8927002}};
			case 626:
				return {{995.8599854, 1113.8973389}};
			case 627:
				return {{995.8623047, 1113.9013672}};
			case 628:
				return {{995.9286499, 1114.0236816}};
			case 629:
				return {{995.9305420, 1114.0264893}};
			case 630:
				return {{995.9570312, 1114.0750732}};
			case 631:
				return {{995.9585571, 1114.0786133}};
			case 632:
				return {{995.9602051, 1114.0810547}};
			case 633:
				return {{995.9848633, 1114.1258545}};
			case 634:
				return {{995.9866333, 1114.1298828}};
			case 635:
				return {{996.0369873, 1114.2224121}};
			case 636:
				return {{996.0391846, 1114.2268066}};
			case 637:
				return {{996.0411987, 1114.2303467}};
			case 638:
				return {{996.0568237, 1114.2592773}};
			case 639:
				return {{996.0573730, 1114.2601318}};
			case 640:
				return {{996.0578613, 1114.2607422}};
			case 641:
				return {{996.0631714, 1114.2712402}};
			case 642:
				return {{996.0633545, 1114.2714844}};
			case 643:
				return {{996.0656128, 1114.2760010}};
			case 644:
				return {{996.0657349, 1114.2761230}};
			case 645:
				return {{996.0657959, 1114.2762451}};
			case 646:
				return {{996.0669556, 1114.2785645}};
			case 647:
				return {{996.0670166, 1114.2786865}};
			case 648:
				return {{996.0671387, 1114.2788086}};
			case 649:
				return {{996.0678101, 1114.2802734}};
			case 650:
				return {{996.0678711, 1114.2802734}};
			case 651:
				return {{996.0681763, 1114.2807617}};
			case 652:
				return {{996.0681763, 1114.2808838}};
			case 653:
				return {{996.0681763, 1114.2808838}};
			case 654:
				return {{996.0684204, 1114.2811279}};
			case 655:
				return {{996.0684204, 1114.2811279}};
			case 656:
				return {{996.0684814, 1114.2812500}};
			case 657:
				return {{996.0684814, 1114.2812500}};
			case 658:
				return {{996.0684814, 1114.2812500}};
			case 659:
				return {{996.0684814, 1114.2812500}};
			case 660:
				return {{996.0684204, 1114.2811279}};
			case 661:
				return {{996.0684204, 1114.2811279}};
			case 662:
				return {{996.0684204, 1114.2811279}};
			case 663:
				return {{996.0683594, 1114.2811279}};
			case 664:
				return {{996.0683594, 1114.2810059}};
			case 665:
				return {{996.0682373, 1114.2808838}};
			case 666:
				return {{996.0682373, 1114.2808838}};
			case 667:
				return {{996.0682373, 1114.2808838}};
			case 668:
				return {{996.0681763, 1114.2807617}};
			case 669:
				return {{996.0681763, 1114.2807617}};
			case 670:
				return {{996.0681763, 1114.2807617}};
			case 671:
				return {{996.0680542, 1114.2806396}};
			case 672:
				return {{996.0680542, 1114.2806396}};
			case 673:
				return {{996.0679932, 1114.2805176}};
			case 674:
				return {{996.0679932, 1114.2805176}};
			case 675:
				return {{996.0679932, 1114.2805176}};
			case 676:
				return {{996.0678711, 1114.2803955}};
			case 677:
				return {{996.0678711, 1114.2802734}};
			case 678:
				return {{996.0678711, 1114.2802734}};
			case 679:
				return {{996.0678101, 1114.2802734}};
			case 680:
				return {{996.0678101, 1114.2802734}};
			case 681:
				return {{996.0677490, 1114.2801514}};
			case 682:
				return {{996.0677490, 1114.2800293}};
			case 683:
				return {{996.0677490, 1114.2800293}};
			case 684:
				return {{996.0676270, 1114.2800293}};
			case 685:
				return {{996.0676270, 1114.2800293}};
			case 686:
				return {{996.0675049, 1114.2792969}};
			case 687:
				return {{996.0675049, 1114.2792969}};
			case 688:
				return {{996.0675049, 1114.2792969}};
			case 689:
				return {{996.0674438, 1114.2792969}};
			case 690:
				return {{996.0673828, 1114.2791748}};
			case 691:
				return {{996.0673828, 1114.2791748}};
			case 692:
				return {{996.0673828, 1114.2791748}};
			case 693:
				return {{996.0673828, 1114.2790527}};
			case 694:
				return {{996.0673828, 1114.2790527}};
			case 695:
				return {{996.0673828, 1114.2790527}};
			case 696:
				return {{996.0673828, 1114.2790527}};
			case 697:
				return {{996.0673828, 1114.2790527}};
			case 698:
				return {{996.0672607, 1114.2790527}};
			case 699:
				return {{996.0672607, 1114.2790527}};
			case 700:
				return {{996.0672607, 1114.2790527}};
			case 701:
				return {{996.0672607, 1114.2790527}};
			case 702:
				return {{996.0672607, 1114.2789307}};
			case 703:
				return {{996.0672607, 1114.2789307}};
			case 704:
				return {{996.0672607, 1114.2789307}};
			case 705:
				return {{996.0672607, 1114.2789307}};
			case 706:
				return {{996.0672607, 1114.2789307}};
			case 707:
				return {{996.0672607, 1114.2789307}};
			case 708:
				return {{996.0671997, 1114.2789307}};
			case 709:
				return {{996.0671997, 1114.2789307}};
			case 710:
				return {{996.0671997, 1114.2789307}};
			case 711:
				return {{996.0671997, 1114.2789307}};
			case 712:
				return {{996.0671997, 1114.2789307}};
			case 713:
				return {{996.0671997, 1114.2789307}};
			case 714:
				return {{996.0671997, 1114.2789307}};
			case 715:
				return {{996.0671997, 1114.2789307}};
			case 716:
				return {{996.0671997, 1114.2789307}};
			case 717:
				return {{996.0671997, 1114.2789307}};
			case 718:
				return {{996.0671997, 1114.2788086}};
			case 719:
				return {{996.0671997, 1114.2788086}};
			case 720:
				return {{996.0671997, 1114.2788086}};
			case 721:
				return {{996.0671997, 1114.2788086}};
			case 722:
				return {{996.0671997, 1114.2788086}};
			case 723:
				return {{996.0671997, 1114.2788086}};
			case 724:
				return {{996.0671997, 1114.2788086}};
			case 725:
				return {{996.0671997, 1114.2788086}};
			case 726:
				return {{996.0671997, 1114.2788086}};
			case 727:
				return {{996.0671997, 1114.2788086}};
			case 728:
				return {{996.0671997, 1114.2788086}};
			case 729:
				return {{996.0671997, 1114.2788086}};
			case 730:
				return {{996.0671997, 1114.2788086}};
			case 731:
				return {{996.0671997, 1114.2788086}};
			case 732:
				return {{996.0671997, 1114.2788086}};
			case 733:
				return {{996.0671997, 1114.2788086}};
			case 734:
				return {{996.0671997, 1114.2788086}};
			case 735:
				return {{996.0671997, 1114.2788086}};
			case 736:
				return {{996.0671997, 1114.2788086}};
			case 737:
				return {{996.0671997, 1114.2788086}};
			case 738:
				return {{996.0671997, 1114.2788086}};
			case 739:
				return {{996.0671997, 1114.2788086}};
			case 740:
				return {{996.0671997, 1114.2788086}};
			case 741:
				return {{996.0671997, 1114.2788086}};
			case 742:
				return {{996.0671997, 1114.2788086}};
			case 743:
				return {{996.0671997, 1114.2788086}};
			case 744:
				return {{996.0671997, 1114.2788086}};
			case 745:
				return {{996.0671997, 1114.2788086}};
			case 746:
				return {{996.0671997, 1114.2788086}};
			case 747:
				return {{996.0671997, 1114.2788086}};
			case 748:
				return {{996.0671997, 1114.2788086}};
			case 749:
				return {{996.0671997, 1114.2788086}};
			case 750:
				return {{996.0671997, 1114.2788086}};
			case 751:
				return {{996.0671997, 1114.2788086}};
			case 752:
				return {{996.0671997, 1114.2788086}};
			case 753:
				return {{996.0671997, 1114.2788086}};
			case 754:
				return {{996.0671997, 1114.2788086}};
			case 755:
				return {{996.0671997, 1114.2788086}};
			case 756:
				return {{996.0671997, 1114.2788086}};
			case 757:
				return {{996.0671997, 1114.2788086}};
			case 758:
				return {{996.0671997, 1114.2788086}};
			case 759:
				return {{996.0671997, 1114.2788086}};
			case 760:
				return {{996.0671997, 1114.2788086}};
			case 761:
				return {{996.0671997, 1114.2788086}};
			case 762:
				return {{996.0671997, 1114.2788086}};
			case 763:
				return {{996.0671997, 1114.2788086}};
			case 764:
				return {{996.0671997, 1114.2788086}};
			case 765:
				return {{996.0671997, 1114.2788086}};
			case 766:
				return {{996.0671997, 1114.2788086}};
			case 767:
				return {{996.0671997, 1114.2788086}};
			case 768:
				return {{996.0671997, 1114.2788086}};
			case 769:
				return {{996.0671997, 1114.2788086}};
			case 770:
				return {{996.0671997, 1114.2788086}};
			case 771:
				return {{996.0671997, 1114.2788086}};
			case 772:
				return {{996.0671997, 1114.2788086}};
			case 773:
				return {{996.0671997, 1114.2788086}};
			case 774:
				return {{996.0671997, 1114.2788086}};
			case 775:
				return {{996.0671997, 1114.2788086}};
			case 776:
				return {{996.0671997, 1114.2788086}};
			case 777:
				return {{996.0671997, 1114.2788086}};
			case 778:
				return {{996.0671997, 1114.2788086}};
			case 779:
				return {{996.0671997, 1114.2788086}};
			case 780:
				return {{996.0671997, 1114.2788086}};
			case 781:
				return {{996.0671997, 1114.2788086}};
			case 782:
				return {{996.0671997, 1114.2788086}};
			case 783:
				return {{996.0671997, 1114.2788086}};
			case 784:
				return {{996.0671997, 1114.2788086}};
			case 785:
				return {{996.0671997, 1114.2788086}};
			case 786:
				return {{996.0671997, 1114.2788086}};
			case 787:
				return {{996.0671997, 1114.2788086}};
			case 788:
				return {{996.0671997, 1114.2788086}};
			case 789:
				return {{996.0671997, 1114.2788086}};
			case 790:
				return {{996.0671997, 1114.2788086}};
			case 791:
				return {{996.0671997, 1114.2788086}};
			case 792:
				return {{996.0671997, 1114.2788086}};
			case 793:
				return {{996.0671997, 1114.2788086}};
			case 794:
				return {{996.0671997, 1114.2788086}};
			case 795:
				return {{996.0671997, 1114.2788086}};
			case 796:
				return {{996.0671997, 1114.2788086}};
			case 797:
				return {{996.0671997, 1114.2788086}};
			case 798:
				return {{996.0671997, 1114.2788086}};
			case 799:
				return {{996.0671997, 1114.2788086}};
			case 800:
				return {{996.0671997, 1114.2788086}};
			case 801:
				return {{996.0671997, 1114.2788086}};
			}
			return std::nullopt;
		}
		assert(false); // Shouldn't be any unaccounted raycasts
		return std::nullopt;
	}
};
// NOLINTEND(bugprone-branch-clone, google-readability-function-size, readability-function-size)
