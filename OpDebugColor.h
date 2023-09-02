// This file is generated. Edit OpDebugColor.txt instead.

#ifndef OpDebugColor_DEFINED
#define OpDebugColor_DEFINED

#include "OpDebug.h"

#if OP_DEBUG_IMAGE

#include <vector>

#define COLOR_LIST \
OP_X(Active) \
OP_X(Between) \
OP_X(Disabled) \
OP_X(Linkups) \
OP_X(Opp) \
OP_X(Out) \
OP_X(Unsectables) \
OP_X(Unsortables)

#define OP_X(Thing) \
	extern void color##Thing(); \
	extern void color##Thing(uint32_t color); \
	extern void color##Thing(uint8_t alpha, uint32_t color); \
	extern void uncolor##Thing(); \
	extern bool color##Thing##On; \
	extern uint32_t color##Thing##Color; 
	COLOR_LIST
#undef OP_X
extern int colorID;
extern uint32_t colorIDColor;
extern uint32_t OP_DEBUG_MULTICOLORED;

extern uint32_t maroon;
extern uint32_t Maroon;
extern uint32_t darkred;
extern uint32_t Darkred;
extern uint32_t darkRed;
extern uint32_t DarkRed;
extern uint32_t dark_red;
extern uint32_t brown;
extern uint32_t Brown;
extern uint32_t firebrick;
extern uint32_t Firebrick;
extern uint32_t crimson;
extern uint32_t Crimson;
extern uint32_t red;
extern uint32_t Red;
extern uint32_t tomato;
extern uint32_t Tomato;
extern uint32_t coral;
extern uint32_t Coral;
extern uint32_t indianred;
extern uint32_t Indianred;
extern uint32_t indianRed;
extern uint32_t IndianRed;
extern uint32_t indian_red;
extern uint32_t lightcoral;
extern uint32_t Lightcoral;
extern uint32_t lightCoral;
extern uint32_t LightCoral;
extern uint32_t light_coral;
extern uint32_t darksalmon;
extern uint32_t Darksalmon;
extern uint32_t darkSalmon;
extern uint32_t DarkSalmon;
extern uint32_t dark_salmon;
extern uint32_t salmon;
extern uint32_t Salmon;
extern uint32_t lightsalmon;
extern uint32_t Lightsalmon;
extern uint32_t lightSalmon;
extern uint32_t LightSalmon;
extern uint32_t light_salmon;
extern uint32_t orangered;
extern uint32_t Orangered;
extern uint32_t orangeRed;
extern uint32_t OrangeRed;
extern uint32_t orange_red;
extern uint32_t darkorange;
extern uint32_t Darkorange;
extern uint32_t darkOrange;
extern uint32_t DarkOrange;
extern uint32_t dark_orange;
extern uint32_t orange;
extern uint32_t Orange;
extern uint32_t gold;
extern uint32_t Gold;
extern uint32_t darkgoldenrod;
extern uint32_t Darkgoldenrod;
extern uint32_t darkGoldenRod;
extern uint32_t DarkGoldenRod;
extern uint32_t dark_golden_rod;
extern uint32_t goldenrod;
extern uint32_t Goldenrod;
extern uint32_t goldenRod;
extern uint32_t GoldenRod;
extern uint32_t golden_rod;
extern uint32_t palegoldenrod;
extern uint32_t Palegoldenrod;
extern uint32_t paleGoldenRod;
extern uint32_t PaleGoldenRod;
extern uint32_t pale_golden_rod;
extern uint32_t darkkhaki;
extern uint32_t Darkkhaki;
extern uint32_t darkKhaki;
extern uint32_t DarkKhaki;
extern uint32_t dark_khaki;
extern uint32_t khaki;
extern uint32_t Khaki;
extern uint32_t olive;
extern uint32_t Olive;
extern uint32_t yellow;
extern uint32_t Yellow;
extern uint32_t yellowgreen;
extern uint32_t Yellowgreen;
extern uint32_t yellowGreen;
extern uint32_t YellowGreen;
extern uint32_t yellow_green;
extern uint32_t darkolivegreen;
extern uint32_t Darkolivegreen;
extern uint32_t darkOliveGreen;
extern uint32_t DarkOliveGreen;
extern uint32_t dark_olive_green;
extern uint32_t olivedrab;
extern uint32_t Olivedrab;
extern uint32_t oliveDrab;
extern uint32_t OliveDrab;
extern uint32_t olive_drab;
extern uint32_t lawngreen;
extern uint32_t Lawngreen;
extern uint32_t lawnGreen;
extern uint32_t LawnGreen;
extern uint32_t lawn_green;
extern uint32_t chartreuse;
extern uint32_t Chartreuse;
extern uint32_t greenyellow;
extern uint32_t Greenyellow;
extern uint32_t greenYellow;
extern uint32_t GreenYellow;
extern uint32_t green_yellow;
extern uint32_t darkgreen;
extern uint32_t Darkgreen;
extern uint32_t darkGreen;
extern uint32_t DarkGreen;
extern uint32_t dark_green;
extern uint32_t green;
extern uint32_t Green;
extern uint32_t forestgreen;
extern uint32_t Forestgreen;
extern uint32_t forestGreen;
extern uint32_t ForestGreen;
extern uint32_t forest_green;
extern uint32_t lime;
extern uint32_t Lime;
extern uint32_t limegreen;
extern uint32_t Limegreen;
extern uint32_t limeGreen;
extern uint32_t LimeGreen;
extern uint32_t lime_green;
extern uint32_t lightgreen;
extern uint32_t Lightgreen;
extern uint32_t lightGreen;
extern uint32_t LightGreen;
extern uint32_t light_green;
extern uint32_t palegreen;
extern uint32_t Palegreen;
extern uint32_t paleGreen;
extern uint32_t PaleGreen;
extern uint32_t pale_green;
extern uint32_t darkseagreen;
extern uint32_t Darkseagreen;
extern uint32_t darkSeaGreen;
extern uint32_t DarkSeaGreen;
extern uint32_t dark_sea_green;
extern uint32_t mediumspringgreen;
extern uint32_t Mediumspringgreen;
extern uint32_t mediumSpringGreen;
extern uint32_t MediumSpringGreen;
extern uint32_t medium_spring_green;
extern uint32_t springgreen;
extern uint32_t Springgreen;
extern uint32_t springGreen;
extern uint32_t SpringGreen;
extern uint32_t spring_green;
extern uint32_t seagreen;
extern uint32_t Seagreen;
extern uint32_t seaGreen;
extern uint32_t SeaGreen;
extern uint32_t sea_green;
extern uint32_t mediumaquamarine;
extern uint32_t Mediumaquamarine;
extern uint32_t mediumAquaMarine;
extern uint32_t MediumAquaMarine;
extern uint32_t medium_aqua_marine;
extern uint32_t mediumseagreen;
extern uint32_t Mediumseagreen;
extern uint32_t mediumSeaGreen;
extern uint32_t MediumSeaGreen;
extern uint32_t medium_sea_green;
extern uint32_t lightseagreen;
extern uint32_t Lightseagreen;
extern uint32_t lightSeaGreen;
extern uint32_t LightSeaGreen;
extern uint32_t light_sea_green;
extern uint32_t darkslategray;
extern uint32_t Darkslategray;
extern uint32_t darkSlateGray;
extern uint32_t DarkSlateGray;
extern uint32_t dark_slate_gray;
extern uint32_t teal;
extern uint32_t Teal;
extern uint32_t darkcyan;
extern uint32_t Darkcyan;
extern uint32_t darkCyan;
extern uint32_t DarkCyan;
extern uint32_t dark_cyan;
extern uint32_t aqua;
extern uint32_t Aqua;
extern uint32_t cyan;
extern uint32_t Cyan;
extern uint32_t lightcyan;
extern uint32_t Lightcyan;
extern uint32_t lightCyan;
extern uint32_t LightCyan;
extern uint32_t light_cyan;
extern uint32_t darkturquoise;
extern uint32_t Darkturquoise;
extern uint32_t darkTurquoise;
extern uint32_t DarkTurquoise;
extern uint32_t dark_turquoise;
extern uint32_t turquoise;
extern uint32_t Turquoise;
extern uint32_t mediumturquoise;
extern uint32_t Mediumturquoise;
extern uint32_t mediumTurquoise;
extern uint32_t MediumTurquoise;
extern uint32_t medium_turquoise;
extern uint32_t paleturquoise;
extern uint32_t Paleturquoise;
extern uint32_t paleTurquoise;
extern uint32_t PaleTurquoise;
extern uint32_t pale_turquoise;
extern uint32_t aquamarine;
extern uint32_t Aquamarine;
extern uint32_t aquaMarine;
extern uint32_t AquaMarine;
extern uint32_t aqua_marine;
extern uint32_t powderblue;
extern uint32_t Powderblue;
extern uint32_t powderBlue;
extern uint32_t PowderBlue;
extern uint32_t powder_blue;
extern uint32_t cadetblue;
extern uint32_t Cadetblue;
extern uint32_t cadetBlue;
extern uint32_t CadetBlue;
extern uint32_t cadet_blue;
extern uint32_t steelblue;
extern uint32_t Steelblue;
extern uint32_t steelBlue;
extern uint32_t SteelBlue;
extern uint32_t steel_blue;
extern uint32_t cornflowerblue;
extern uint32_t Cornflowerblue;
extern uint32_t cornFlowerBlue;
extern uint32_t CornFlowerBlue;
extern uint32_t corn_flower_blue;
extern uint32_t deepskyblue;
extern uint32_t Deepskyblue;
extern uint32_t deepSkyBlue;
extern uint32_t DeepSkyBlue;
extern uint32_t deep_sky_blue;
extern uint32_t dodgerblue;
extern uint32_t Dodgerblue;
extern uint32_t dodgerBlue;
extern uint32_t DodgerBlue;
extern uint32_t dodger_blue;
extern uint32_t lightblue;
extern uint32_t Lightblue;
extern uint32_t lightBlue;
extern uint32_t LightBlue;
extern uint32_t light_blue;
extern uint32_t skyblue;
extern uint32_t Skyblue;
extern uint32_t skyBlue;
extern uint32_t SkyBlue;
extern uint32_t sky_blue;
extern uint32_t lightskyblue;
extern uint32_t Lightskyblue;
extern uint32_t lightSkyBlue;
extern uint32_t LightSkyBlue;
extern uint32_t light_sky_blue;
extern uint32_t midnightblue;
extern uint32_t Midnightblue;
extern uint32_t midnightBlue;
extern uint32_t MidnightBlue;
extern uint32_t midnight_blue;
extern uint32_t navy;
extern uint32_t Navy;
extern uint32_t darkblue;
extern uint32_t Darkblue;
extern uint32_t darkBlue;
extern uint32_t DarkBlue;
extern uint32_t dark_blue;
extern uint32_t mediumblue;
extern uint32_t Mediumblue;
extern uint32_t mediumBlue;
extern uint32_t MediumBlue;
extern uint32_t medium_blue;
extern uint32_t blue;
extern uint32_t Blue;
extern uint32_t royalblue;
extern uint32_t Royalblue;
extern uint32_t royalBlue;
extern uint32_t RoyalBlue;
extern uint32_t royal_blue;
extern uint32_t blueviolet;
extern uint32_t Blueviolet;
extern uint32_t blueViolet;
extern uint32_t BlueViolet;
extern uint32_t blue_violet;
extern uint32_t indigo;
extern uint32_t Indigo;
extern uint32_t darkslateblue;
extern uint32_t Darkslateblue;
extern uint32_t darkSlateBlue;
extern uint32_t DarkSlateBlue;
extern uint32_t dark_slate_blue;
extern uint32_t slateblue;
extern uint32_t Slateblue;
extern uint32_t slateBlue;
extern uint32_t SlateBlue;
extern uint32_t slate_blue;
extern uint32_t mediumslateblue;
extern uint32_t Mediumslateblue;
extern uint32_t mediumSlateBlue;
extern uint32_t MediumSlateBlue;
extern uint32_t medium_slate_blue;
extern uint32_t mediumpurple;
extern uint32_t Mediumpurple;
extern uint32_t mediumPurple;
extern uint32_t MediumPurple;
extern uint32_t medium_purple;
extern uint32_t darkmagenta;
extern uint32_t Darkmagenta;
extern uint32_t darkMagenta;
extern uint32_t DarkMagenta;
extern uint32_t dark_magenta;
extern uint32_t darkviolet;
extern uint32_t Darkviolet;
extern uint32_t darkViolet;
extern uint32_t DarkViolet;
extern uint32_t dark_violet;
extern uint32_t darkorchid;
extern uint32_t Darkorchid;
extern uint32_t darkOrchid;
extern uint32_t DarkOrchid;
extern uint32_t dark_orchid;
extern uint32_t mediumorchid;
extern uint32_t Mediumorchid;
extern uint32_t mediumOrchid;
extern uint32_t MediumOrchid;
extern uint32_t medium_orchid;
extern uint32_t purple;
extern uint32_t Purple;
extern uint32_t thistle;
extern uint32_t Thistle;
extern uint32_t plum;
extern uint32_t Plum;
extern uint32_t violet;
extern uint32_t Violet;
extern uint32_t fuchsia;
extern uint32_t Fuchsia;
extern uint32_t magenta;
extern uint32_t Magenta;
extern uint32_t orchid;
extern uint32_t Orchid;
extern uint32_t mediumvioletred;
extern uint32_t Mediumvioletred;
extern uint32_t mediumVioletRed;
extern uint32_t MediumVioletRed;
extern uint32_t medium_violet_red;
extern uint32_t palevioletred;
extern uint32_t Palevioletred;
extern uint32_t paleVioletRed;
extern uint32_t PaleVioletRed;
extern uint32_t pale_violet_red;
extern uint32_t deeppink;
extern uint32_t Deeppink;
extern uint32_t deepPink;
extern uint32_t DeepPink;
extern uint32_t deep_pink;
extern uint32_t hotpink;
extern uint32_t Hotpink;
extern uint32_t hotPink;
extern uint32_t HotPink;
extern uint32_t hot_pink;
extern uint32_t lightpink;
extern uint32_t Lightpink;
extern uint32_t lightPink;
extern uint32_t LightPink;
extern uint32_t light_pink;
extern uint32_t pink;
extern uint32_t Pink;
extern uint32_t antiquewhite;
extern uint32_t Antiquewhite;
extern uint32_t antiqueWhite;
extern uint32_t AntiqueWhite;
extern uint32_t antique_white;
extern uint32_t beige;
extern uint32_t Beige;
extern uint32_t bisque;
extern uint32_t Bisque;
extern uint32_t blanchedalmond;
extern uint32_t Blanchedalmond;
extern uint32_t blanchedAlmond;
extern uint32_t BlanchedAlmond;
extern uint32_t blanched_almond;
extern uint32_t wheat;
extern uint32_t Wheat;
extern uint32_t cornsilk;
extern uint32_t Cornsilk;
extern uint32_t cornSilk;
extern uint32_t CornSilk;
extern uint32_t corn_silk;
extern uint32_t lemonchiffon;
extern uint32_t Lemonchiffon;
extern uint32_t lemonChiffon;
extern uint32_t LemonChiffon;
extern uint32_t lemon_chiffon;
extern uint32_t lightgoldenrodyellow;
extern uint32_t Lightgoldenrodyellow;
extern uint32_t lightGoldenRodYellow;
extern uint32_t LightGoldenRodYellow;
extern uint32_t light_golden_rod_yellow;
extern uint32_t lightyellow;
extern uint32_t Lightyellow;
extern uint32_t lightYellow;
extern uint32_t LightYellow;
extern uint32_t light_yellow;
extern uint32_t saddlebrown;
extern uint32_t Saddlebrown;
extern uint32_t saddleBrown;
extern uint32_t SaddleBrown;
extern uint32_t saddle_brown;
extern uint32_t sienna;
extern uint32_t Sienna;
extern uint32_t chocolate;
extern uint32_t Chocolate;
extern uint32_t peru;
extern uint32_t Peru;
extern uint32_t sandybrown;
extern uint32_t Sandybrown;
extern uint32_t sandyBrown;
extern uint32_t SandyBrown;
extern uint32_t sandy_brown;
extern uint32_t burlywood;
extern uint32_t Burlywood;
extern uint32_t burlyWood;
extern uint32_t BurlyWood;
extern uint32_t burly_wood;
extern uint32_t tanbrown;
extern uint32_t Tanbrown;
extern uint32_t tanBrown;
extern uint32_t TanBrown;
extern uint32_t tan_brown;
extern uint32_t rosybrown;
extern uint32_t Rosybrown;
extern uint32_t rosyBrown;
extern uint32_t RosyBrown;
extern uint32_t rosy_brown;
extern uint32_t moccasin;
extern uint32_t Moccasin;
extern uint32_t navajowhite;
extern uint32_t Navajowhite;
extern uint32_t navajoWhite;
extern uint32_t NavajoWhite;
extern uint32_t navajo_white;
extern uint32_t peachpuff;
extern uint32_t Peachpuff;
extern uint32_t peachPuff;
extern uint32_t PeachPuff;
extern uint32_t peach_puff;
extern uint32_t mistyrose;
extern uint32_t Mistyrose;
extern uint32_t mistyRose;
extern uint32_t MistyRose;
extern uint32_t misty_rose;
extern uint32_t lavenderblush;
extern uint32_t Lavenderblush;
extern uint32_t lavenderBlush;
extern uint32_t LavenderBlush;
extern uint32_t lavender_blush;
extern uint32_t linen;
extern uint32_t Linen;
extern uint32_t oldlace;
extern uint32_t Oldlace;
extern uint32_t oldLace;
extern uint32_t OldLace;
extern uint32_t old_lace;
extern uint32_t papayawhip;
extern uint32_t Papayawhip;
extern uint32_t papayaWhip;
extern uint32_t PapayaWhip;
extern uint32_t papaya_whip;
extern uint32_t seashell;
extern uint32_t Seashell;
extern uint32_t seaShell;
extern uint32_t SeaShell;
extern uint32_t sea_shell;
extern uint32_t mintcream;
extern uint32_t Mintcream;
extern uint32_t mintCream;
extern uint32_t MintCream;
extern uint32_t mint_cream;
extern uint32_t slategray;
extern uint32_t Slategray;
extern uint32_t slateGray;
extern uint32_t SlateGray;
extern uint32_t slate_gray;
extern uint32_t lightslategray;
extern uint32_t Lightslategray;
extern uint32_t lightSlateGray;
extern uint32_t LightSlateGray;
extern uint32_t light_slate_gray;
extern uint32_t lightsteelblue;
extern uint32_t Lightsteelblue;
extern uint32_t lightSteelBlue;
extern uint32_t LightSteelBlue;
extern uint32_t light_steel_blue;
extern uint32_t lavender;
extern uint32_t Lavender;
extern uint32_t floralwhite;
extern uint32_t Floralwhite;
extern uint32_t floralWhite;
extern uint32_t FloralWhite;
extern uint32_t floral_white;
extern uint32_t aliceblue;
extern uint32_t Aliceblue;
extern uint32_t aliceBlue;
extern uint32_t AliceBlue;
extern uint32_t alice_blue;
extern uint32_t ghostwhite;
extern uint32_t Ghostwhite;
extern uint32_t ghostWhite;
extern uint32_t GhostWhite;
extern uint32_t ghost_white;
extern uint32_t honeydew;
extern uint32_t Honeydew;
extern uint32_t ivory;
extern uint32_t Ivory;
extern uint32_t azure;
extern uint32_t Azure;
extern uint32_t snow;
extern uint32_t Snow;
extern uint32_t black;
extern uint32_t Black;
extern uint32_t dimgray;
extern uint32_t Dimgray;
extern uint32_t dimGray;
extern uint32_t DimGray;
extern uint32_t dim_gray;
extern uint32_t dimgrey;
extern uint32_t Dimgrey;
extern uint32_t dimGrey;
extern uint32_t DimGrey;
extern uint32_t dim_grey;
extern uint32_t gray;
extern uint32_t Gray;
extern uint32_t grey;
extern uint32_t Grey;
extern uint32_t darkgray;
extern uint32_t Darkgray;
extern uint32_t darkGray;
extern uint32_t DarkGray;
extern uint32_t dark_gray;
extern uint32_t darkgrey;
extern uint32_t Darkgrey;
extern uint32_t darkGrey;
extern uint32_t DarkGrey;
extern uint32_t dark_grey;
extern uint32_t silver;
extern uint32_t Silver;
extern uint32_t lightgray;
extern uint32_t Lightgray;
extern uint32_t lightGray;
extern uint32_t LightGray;
extern uint32_t light_gray;
extern uint32_t lightgrey;
extern uint32_t Lightgrey;
extern uint32_t lightGrey;
extern uint32_t LightGrey;
extern uint32_t light_grey;
extern uint32_t gainsboro;
extern uint32_t Gainsboro;
extern uint32_t whitesmoke;
extern uint32_t Whitesmoke;
extern uint32_t whiteSmoke;
extern uint32_t WhiteSmoke;
extern uint32_t white_smoke;
extern uint32_t white;
extern uint32_t White;

extern std::vector<uint32_t> debugColorArray;

#endif

#endif
