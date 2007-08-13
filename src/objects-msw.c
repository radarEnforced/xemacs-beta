/* mswindows-specific Lisp objects.
   Copyright (C) 1993, 1994 Free Software Foundation, Inc.
   Copyright (C) 1995 Board of Trustees, University of Illinois.
   Copyright (C) 1995 Tinker Systems.
   Copyright (C) 1995, 1996 Ben Wing.
   Copyright (C) 1995 Sun Microsystems, Inc.
   Copyright (C) 1997 Jonathan Harris.

This file is part of XEmacs.

XEmacs is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.

XEmacs is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with XEmacs; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* Synched up with: Not in FSF. */

/* Authorship:

   Jamie Zawinski, Chuck Thompson, Ben Wing
   Rewritten for mswindows by Jonathan Harris, November 1997 for 20.4.
 */


/* TODO: palette handling */

#include <config.h>
#include "lisp.h"

#include "console-msw.h"
#include "objects-msw.h"

#ifdef MULE
#include "mule-charset.h"
#endif

#include "buffer.h"
#include "device.h"
#include "insdel.h"

#ifdef __CYGWIN32__
#define stricmp strcasecmp
#endif

typedef struct colormap_t 
{
  char *name;
  COLORREF colorref;
} colormap_t;

/* Colors from X11R6 "XConsortium: rgb.txt,v 10.41 94/02/20 18:39:36 rws Exp" */
static CONST colormap_t mswindows_X_color_map[] = 
{
  {"snow"			, PALETTERGB (255, 250, 250) },
  {"GhostWhite"			, PALETTERGB (248, 248, 255) },
  {"WhiteSmoke"			, PALETTERGB (245, 245, 245) },
  {"gainsboro"			, PALETTERGB (220, 220, 220) },
  {"FloralWhite"		, PALETTERGB (255, 250, 240) },
  {"OldLace"			, PALETTERGB (253, 245, 230) },
  {"linen"			, PALETTERGB (250, 240, 230) },
  {"AntiqueWhite"		, PALETTERGB (250, 235, 215) },
  {"PapayaWhip"			, PALETTERGB (255, 239, 213) },
  {"BlanchedAlmond"		, PALETTERGB (255, 235, 205) },
  {"bisque"			, PALETTERGB (255, 228, 196) },
  {"PeachPuff"			, PALETTERGB (255, 218, 185) },
  {"NavajoWhite"		, PALETTERGB (255, 222, 173) },
  {"moccasin"			, PALETTERGB (255, 228, 181) },
  {"cornsilk"			, PALETTERGB (255, 248, 220) },
  {"ivory"			, PALETTERGB (255, 255, 240) },
  {"LemonChiffon"		, PALETTERGB (255, 250, 205) },
  {"seashell"			, PALETTERGB (255, 245, 238) },
  {"honeydew"			, PALETTERGB (240, 255, 240) },
  {"MintCream"			, PALETTERGB (245, 255, 250) },
  {"azure"			, PALETTERGB (240, 255, 255) },
  {"AliceBlue"			, PALETTERGB (240, 248, 255) },
  {"lavender"			, PALETTERGB (230, 230, 250) },
  {"LavenderBlush"		, PALETTERGB (255, 240, 245) },
  {"MistyRose"			, PALETTERGB (255, 228, 225) },
  {"white"			, PALETTERGB (255, 255, 255) },
  {"black"			, PALETTERGB (0, 0, 0) },
  {"DarkSlateGray"		, PALETTERGB (47, 79, 79) },
  {"DarkSlateGrey"		, PALETTERGB (47, 79, 79) },
  {"DimGray"			, PALETTERGB (105, 105, 105) },
  {"DimGrey"			, PALETTERGB (105, 105, 105) },
  {"SlateGray"			, PALETTERGB (112, 128, 144) },
  {"SlateGrey"			, PALETTERGB (112, 128, 144) },
  {"LightSlateGray"		, PALETTERGB (119, 136, 153) },
  {"LightSlateGrey"		, PALETTERGB (119, 136, 153) },
  {"gray"			, PALETTERGB (190, 190, 190) },
  {"grey"			, PALETTERGB (190, 190, 190) },
  {"LightGrey"			, PALETTERGB (211, 211, 211) },
  {"LightGray"			, PALETTERGB (211, 211, 211) },
  {"MidnightBlue"		, PALETTERGB (25, 25, 112) },
  {"navy"			, PALETTERGB (0, 0, 128) },
  {"NavyBlue"			, PALETTERGB (0, 0, 128) },
  {"CornflowerBlue"		, PALETTERGB (100, 149, 237) },
  {"DarkSlateBlue"		, PALETTERGB (72, 61, 139) },
  {"SlateBlue"			, PALETTERGB (106, 90, 205) },
  {"MediumSlateBlue"		, PALETTERGB (123, 104, 238) },
  {"LightSlateBlue"		, PALETTERGB (132, 112, 255) },
  {"MediumBlue"			, PALETTERGB (0, 0, 205) },
  {"RoyalBlue"			, PALETTERGB (65, 105, 225) },
  {"blue"			, PALETTERGB (0, 0, 255) },
  {"DodgerBlue"			, PALETTERGB (30, 144, 255) },
  {"DeepSkyBlue"		, PALETTERGB (0, 191, 255) },
  {"SkyBlue"			, PALETTERGB (135, 206, 235) },
  {"LightSkyBlue"		, PALETTERGB (135, 206, 250) },
  {"SteelBlue"			, PALETTERGB (70, 130, 180) },
  {"LightSteelBlue"		, PALETTERGB (176, 196, 222) },
  {"LightBlue"			, PALETTERGB (173, 216, 230) },
  {"PowderBlue"			, PALETTERGB (176, 224, 230) },
  {"PaleTurquoise"		, PALETTERGB (175, 238, 238) },
  {"DarkTurquoise"		, PALETTERGB (0, 206, 209) },
  {"MediumTurquoise"		, PALETTERGB (72, 209, 204) },
  {"turquoise"			, PALETTERGB (64, 224, 208) },
  {"cyan"			, PALETTERGB (0, 255, 255) },
  {"LightCyan"			, PALETTERGB (224, 255, 255) },
  {"CadetBlue"			, PALETTERGB (95, 158, 160) },
  {"MediumAquamarine"		, PALETTERGB (102, 205, 170) },
  {"aquamarine"			, PALETTERGB (127, 255, 212) },
  {"DarkGreen"			, PALETTERGB (0, 100, 0) },
  {"DarkOliveGreen"		, PALETTERGB (85, 107, 47) },
  {"DarkSeaGreen"		, PALETTERGB (143, 188, 143) },
  {"SeaGreen"			, PALETTERGB (46, 139, 87) },
  {"MediumSeaGreen"		, PALETTERGB (60, 179, 113) },
  {"LightSeaGreen"		, PALETTERGB (32, 178, 170) },
  {"PaleGreen"			, PALETTERGB (152, 251, 152) },
  {"SpringGreen"		, PALETTERGB (0, 255, 127) },
  {"LawnGreen"			, PALETTERGB (124, 252, 0) },
  {"green"			, PALETTERGB (0, 255, 0) },
  {"chartreuse"			, PALETTERGB (127, 255, 0) },
  {"MediumSpringGreen"		, PALETTERGB (0, 250, 154) },
  {"GreenYellow"		, PALETTERGB (173, 255, 47) },
  {"LimeGreen"			, PALETTERGB (50, 205, 50) },
  {"YellowGreen"		, PALETTERGB (154, 205, 50) },
  {"ForestGreen"		, PALETTERGB (34, 139, 34) },
  {"OliveDrab"			, PALETTERGB (107, 142, 35) },
  {"DarkKhaki"			, PALETTERGB (189, 183, 107) },
  {"khaki"			, PALETTERGB (240, 230, 140) },
  {"PaleGoldenrod"		, PALETTERGB (238, 232, 170) },
  {"LightGoldenrodYellow"	, PALETTERGB (250, 250, 210) },
  {"LightYellow"		, PALETTERGB (255, 255, 224) },
  {"yellow"			, PALETTERGB (255, 255, 0) },
  {"gold"			, PALETTERGB (255, 215, 0) },
  {"LightGoldenrod"		, PALETTERGB (238, 221, 130) },
  {"goldenrod"			, PALETTERGB (218, 165, 32) },
  {"DarkGoldenrod"		, PALETTERGB (184, 134, 11) },
  {"RosyBrown"			, PALETTERGB (188, 143, 143) },
  {"IndianRed"			, PALETTERGB (205, 92, 92) },
  {"SaddleBrown"		, PALETTERGB (139, 69, 19) },
  {"sienna"			, PALETTERGB (160, 82, 45) },
  {"peru"			, PALETTERGB (205, 133, 63) },
  {"burlywood"			, PALETTERGB (222, 184, 135) },
  {"beige"			, PALETTERGB (245, 245, 220) },
  {"wheat"			, PALETTERGB (245, 222, 179) },
  {"SandyBrown"			, PALETTERGB (244, 164, 96) },
  {"tan"			, PALETTERGB (210, 180, 140) },
  {"chocolate"			, PALETTERGB (210, 105, 30) },
  {"firebrick"			, PALETTERGB (178, 34, 34) },
  {"brown"			, PALETTERGB (165, 42, 42) },
  {"DarkSalmon"			, PALETTERGB (233, 150, 122) },
  {"salmon"			, PALETTERGB (250, 128, 114) },
  {"LightSalmon"		, PALETTERGB (255, 160, 122) },
  {"orange"			, PALETTERGB (255, 165, 0) },
  {"DarkOrange"			, PALETTERGB (255, 140, 0) },
  {"coral"			, PALETTERGB (255, 127, 80) },
  {"LightCoral"			, PALETTERGB (240, 128, 128) },
  {"tomato"			, PALETTERGB (255, 99, 71) },
  {"OrangeRed"			, PALETTERGB (255, 69, 0) },
  {"red"			, PALETTERGB (255, 0, 0) },
  {"HotPink"			, PALETTERGB (255, 105, 180) },
  {"DeepPink"			, PALETTERGB (255, 20, 147) },
  {"pink"			, PALETTERGB (255, 192, 203) },
  {"LightPink"			, PALETTERGB (255, 182, 193) },
  {"PaleVioletRed"		, PALETTERGB (219, 112, 147) },
  {"maroon"			, PALETTERGB (176, 48, 96) },
  {"MediumVioletRed"		, PALETTERGB (199, 21, 133) },
  {"VioletRed"			, PALETTERGB (208, 32, 144) },
  {"magenta"			, PALETTERGB (255, 0, 255) },
  {"violet"			, PALETTERGB (238, 130, 238) },
  {"plum"			, PALETTERGB (221, 160, 221) },
  {"orchid"			, PALETTERGB (218, 112, 214) },
  {"MediumOrchid"		, PALETTERGB (186, 85, 211) },
  {"DarkOrchid"			, PALETTERGB (153, 50, 204) },
  {"DarkViolet"			, PALETTERGB (148, 0, 211) },
  {"BlueViolet"			, PALETTERGB (138, 43, 226) },
  {"purple"			, PALETTERGB (160, 32, 240) },
  {"MediumPurple"		, PALETTERGB (147, 112, 219) },
  {"thistle"			, PALETTERGB (216, 191, 216) },
  {"snow1"			, PALETTERGB (255, 250, 250) },
  {"snow2"			, PALETTERGB (238, 233, 233) },
  {"snow3"			, PALETTERGB (205, 201, 201) },
  {"snow4"			, PALETTERGB (139, 137, 137) },
  {"seashell1"			, PALETTERGB (255, 245, 238) },
  {"seashell2"			, PALETTERGB (238, 229, 222) },
  {"seashell3"			, PALETTERGB (205, 197, 191) },
  {"seashell4"			, PALETTERGB (139, 134, 130) },
  {"AntiqueWhite1"		, PALETTERGB (255, 239, 219) },
  {"AntiqueWhite2"		, PALETTERGB (238, 223, 204) },
  {"AntiqueWhite3"		, PALETTERGB (205, 192, 176) },
  {"AntiqueWhite4"		, PALETTERGB (139, 131, 120) },
  {"bisque1"			, PALETTERGB (255, 228, 196) },
  {"bisque2"			, PALETTERGB (238, 213, 183) },
  {"bisque3"			, PALETTERGB (205, 183, 158) },
  {"bisque4"			, PALETTERGB (139, 125, 107) },
  {"PeachPuff1"			, PALETTERGB (255, 218, 185) },
  {"PeachPuff2"			, PALETTERGB (238, 203, 173) },
  {"PeachPuff3"			, PALETTERGB (205, 175, 149) },
  {"PeachPuff4"			, PALETTERGB (139, 119, 101) },
  {"NavajoWhite1"		, PALETTERGB (255, 222, 173) },
  {"NavajoWhite2"		, PALETTERGB (238, 207, 161) },
  {"NavajoWhite3"		, PALETTERGB (205, 179, 139) },
  {"NavajoWhite4"		, PALETTERGB (139, 121, 94) },
  {"LemonChiffon1"		, PALETTERGB (255, 250, 205) },
  {"LemonChiffon2"		, PALETTERGB (238, 233, 191) },
  {"LemonChiffon3"		, PALETTERGB (205, 201, 165) },
  {"LemonChiffon4"		, PALETTERGB (139, 137, 112) },
  {"cornsilk1"			, PALETTERGB (255, 248, 220) },
  {"cornsilk2"			, PALETTERGB (238, 232, 205) },
  {"cornsilk3"			, PALETTERGB (205, 200, 177) },
  {"cornsilk4"			, PALETTERGB (139, 136, 120) },
  {"ivory1"			, PALETTERGB (255, 255, 240) },
  {"ivory2"			, PALETTERGB (238, 238, 224) },
  {"ivory3"			, PALETTERGB (205, 205, 193) },
  {"ivory4"			, PALETTERGB (139, 139, 131) },
  {"honeydew1"			, PALETTERGB (240, 255, 240) },
  {"honeydew2"			, PALETTERGB (224, 238, 224) },
  {"honeydew3"			, PALETTERGB (193, 205, 193) },
  {"honeydew4"			, PALETTERGB (131, 139, 131) },
  {"LavenderBlush1"		, PALETTERGB (255, 240, 245) },
  {"LavenderBlush2"		, PALETTERGB (238, 224, 229) },
  {"LavenderBlush3"		, PALETTERGB (205, 193, 197) },
  {"LavenderBlush4"		, PALETTERGB (139, 131, 134) },
  {"MistyRose1"			, PALETTERGB (255, 228, 225) },
  {"MistyRose2"			, PALETTERGB (238, 213, 210) },
  {"MistyRose3"			, PALETTERGB (205, 183, 181) },
  {"MistyRose4"			, PALETTERGB (139, 125, 123) },
  {"azure1"			, PALETTERGB (240, 255, 255) },
  {"azure2"			, PALETTERGB (224, 238, 238) },
  {"azure3"			, PALETTERGB (193, 205, 205) },
  {"azure4"			, PALETTERGB (131, 139, 139) },
  {"SlateBlue1"			, PALETTERGB (131, 111, 255) },
  {"SlateBlue2"			, PALETTERGB (122, 103, 238) },
  {"SlateBlue3"			, PALETTERGB (105, 89, 205) },
  {"SlateBlue4"			, PALETTERGB (71, 60, 139) },
  {"RoyalBlue1"			, PALETTERGB (72, 118, 255) },
  {"RoyalBlue2"			, PALETTERGB (67, 110, 238) },
  {"RoyalBlue3"			, PALETTERGB (58, 95, 205) },
  {"RoyalBlue4"			, PALETTERGB (39, 64, 139) },
  {"blue1"			, PALETTERGB (0, 0, 255) },
  {"blue2"			, PALETTERGB (0, 0, 238) },
  {"blue3"			, PALETTERGB (0, 0, 205) },
  {"blue4"			, PALETTERGB (0, 0, 139) },
  {"DodgerBlue1"		, PALETTERGB (30, 144, 255) },
  {"DodgerBlue2"		, PALETTERGB (28, 134, 238) },
  {"DodgerBlue3"		, PALETTERGB (24, 116, 205) },
  {"DodgerBlue4"		, PALETTERGB (16, 78, 139) },
  {"SteelBlue1"			, PALETTERGB (99, 184, 255) },
  {"SteelBlue2"			, PALETTERGB (92, 172, 238) },
  {"SteelBlue3"			, PALETTERGB (79, 148, 205) },
  {"SteelBlue4"			, PALETTERGB (54, 100, 139) },
  {"DeepSkyBlue1"		, PALETTERGB (0, 191, 255) },
  {"DeepSkyBlue2"		, PALETTERGB (0, 178, 238) },
  {"DeepSkyBlue3"		, PALETTERGB (0, 154, 205) },
  {"DeepSkyBlue4"		, PALETTERGB (0, 104, 139) },
  {"SkyBlue1"			, PALETTERGB (135, 206, 255) },
  {"SkyBlue2"			, PALETTERGB (126, 192, 238) },
  {"SkyBlue3"			, PALETTERGB (108, 166, 205) },
  {"SkyBlue4"			, PALETTERGB (74, 112, 139) },
  {"LightSkyBlue1"		, PALETTERGB (176, 226, 255) },
  {"LightSkyBlue2"		, PALETTERGB (164, 211, 238) },
  {"LightSkyBlue3"		, PALETTERGB (141, 182, 205) },
  {"LightSkyBlue4"		, PALETTERGB (96, 123, 139) },
  {"SlateGray1"			, PALETTERGB (198, 226, 255) },
  {"SlateGray2"			, PALETTERGB (185, 211, 238) },
  {"SlateGray3"			, PALETTERGB (159, 182, 205) },
  {"SlateGray4"			, PALETTERGB (108, 123, 139) },
  {"LightSteelBlue1"		, PALETTERGB (202, 225, 255) },
  {"LightSteelBlue2"		, PALETTERGB (188, 210, 238) },
  {"LightSteelBlue3"		, PALETTERGB (162, 181, 205) },
  {"LightSteelBlue4"		, PALETTERGB (110, 123, 139) },
  {"LightBlue1"			, PALETTERGB (191, 239, 255) },
  {"LightBlue2"			, PALETTERGB (178, 223, 238) },
  {"LightBlue3"			, PALETTERGB (154, 192, 205) },
  {"LightBlue4"			, PALETTERGB (104, 131, 139) },
  {"LightCyan1"			, PALETTERGB (224, 255, 255) },
  {"LightCyan2"			, PALETTERGB (209, 238, 238) },
  {"LightCyan3"			, PALETTERGB (180, 205, 205) },
  {"LightCyan4"			, PALETTERGB (122, 139, 139) },
  {"PaleTurquoise1"		, PALETTERGB (187, 255, 255) },
  {"PaleTurquoise2"		, PALETTERGB (174, 238, 238) },
  {"PaleTurquoise3"		, PALETTERGB (150, 205, 205) },
  {"PaleTurquoise4"		, PALETTERGB (102, 139, 139) },
  {"CadetBlue1"			, PALETTERGB (152, 245, 255) },
  {"CadetBlue2"			, PALETTERGB (142, 229, 238) },
  {"CadetBlue3"			, PALETTERGB (122, 197, 205) },
  {"CadetBlue4"			, PALETTERGB (83, 134, 139) },
  {"turquoise1"			, PALETTERGB (0, 245, 255) },
  {"turquoise2"			, PALETTERGB (0, 229, 238) },
  {"turquoise3"			, PALETTERGB (0, 197, 205) },
  {"turquoise4"			, PALETTERGB (0, 134, 139) },
  {"cyan1"			, PALETTERGB (0, 255, 255) },
  {"cyan2"			, PALETTERGB (0, 238, 238) },
  {"cyan3"			, PALETTERGB (0, 205, 205) },
  {"cyan4"			, PALETTERGB (0, 139, 139) },
  {"DarkSlateGray1"		, PALETTERGB (151, 255, 255) },
  {"DarkSlateGray2"		, PALETTERGB (141, 238, 238) },
  {"DarkSlateGray3"		, PALETTERGB (121, 205, 205) },
  {"DarkSlateGray4"		, PALETTERGB (82, 139, 139) },
  {"aquamarine1"		, PALETTERGB (127, 255, 212) },
  {"aquamarine2"		, PALETTERGB (118, 238, 198) },
  {"aquamarine3"		, PALETTERGB (102, 205, 170) },
  {"aquamarine4"		, PALETTERGB (69, 139, 116) },
  {"DarkSeaGreen1"		, PALETTERGB (193, 255, 193) },
  {"DarkSeaGreen2"		, PALETTERGB (180, 238, 180) },
  {"DarkSeaGreen3"		, PALETTERGB (155, 205, 155) },
  {"DarkSeaGreen4"		, PALETTERGB (105, 139, 105) },
  {"SeaGreen1"			, PALETTERGB (84, 255, 159) },
  {"SeaGreen2"			, PALETTERGB (78, 238, 148) },
  {"SeaGreen3"			, PALETTERGB (67, 205, 128) },
  {"SeaGreen4"			, PALETTERGB (46, 139, 87) },
  {"PaleGreen1"			, PALETTERGB (154, 255, 154) },
  {"PaleGreen2"			, PALETTERGB (144, 238, 144) },
  {"PaleGreen3"			, PALETTERGB (124, 205, 124) },
  {"PaleGreen4"			, PALETTERGB (84, 139, 84) },
  {"SpringGreen1"		, PALETTERGB (0, 255, 127) },
  {"SpringGreen2"		, PALETTERGB (0, 238, 118) },
  {"SpringGreen3"		, PALETTERGB (0, 205, 102) },
  {"SpringGreen4"		, PALETTERGB (0, 139, 69) },
  {"green1"			, PALETTERGB (0, 255, 0) },
  {"green2"			, PALETTERGB (0, 238, 0) },
  {"green3"			, PALETTERGB (0, 205, 0) },
  {"green4"			, PALETTERGB (0, 139, 0) },
  {"chartreuse1"		, PALETTERGB (127, 255, 0) },
  {"chartreuse2"		, PALETTERGB (118, 238, 0) },
  {"chartreuse3"		, PALETTERGB (102, 205, 0) },
  {"chartreuse4"		, PALETTERGB (69, 139, 0) },
  {"OliveDrab1"			, PALETTERGB (192, 255, 62) },
  {"OliveDrab2"			, PALETTERGB (179, 238, 58) },
  {"OliveDrab3"			, PALETTERGB (154, 205, 50) },
  {"OliveDrab4"			, PALETTERGB (105, 139, 34) },
  {"DarkOliveGreen1"		, PALETTERGB (202, 255, 112) },
  {"DarkOliveGreen2"		, PALETTERGB (188, 238, 104) },
  {"DarkOliveGreen3"		, PALETTERGB (162, 205, 90) },
  {"DarkOliveGreen4"		, PALETTERGB (110, 139, 61) },
  {"khaki1"			, PALETTERGB (255, 246, 143) },
  {"khaki2"			, PALETTERGB (238, 230, 133) },
  {"khaki3"			, PALETTERGB (205, 198, 115) },
  {"khaki4"			, PALETTERGB (139, 134, 78) },
  {"LightGoldenrod1"		, PALETTERGB (255, 236, 139) },
  {"LightGoldenrod2"		, PALETTERGB (238, 220, 130) },
  {"LightGoldenrod3"		, PALETTERGB (205, 190, 112) },
  {"LightGoldenrod4"		, PALETTERGB (139, 129, 76) },
  {"LightYellow1"		, PALETTERGB (255, 255, 224) },
  {"LightYellow2"		, PALETTERGB (238, 238, 209) },
  {"LightYellow3"		, PALETTERGB (205, 205, 180) },
  {"LightYellow4"		, PALETTERGB (139, 139, 122) },
  {"yellow1"			, PALETTERGB (255, 255, 0) },
  {"yellow2"			, PALETTERGB (238, 238, 0) },
  {"yellow3"			, PALETTERGB (205, 205, 0) },
  {"yellow4"			, PALETTERGB (139, 139, 0) },
  {"gold1"			, PALETTERGB (255, 215, 0) },
  {"gold2"			, PALETTERGB (238, 201, 0) },
  {"gold3"			, PALETTERGB (205, 173, 0) },
  {"gold4"			, PALETTERGB (139, 117, 0) },
  {"goldenrod1"			, PALETTERGB (255, 193, 37) },
  {"goldenrod2"			, PALETTERGB (238, 180, 34) },
  {"goldenrod3"			, PALETTERGB (205, 155, 29) },
  {"goldenrod4"			, PALETTERGB (139, 105, 20) },
  {"DarkGoldenrod1"		, PALETTERGB (255, 185, 15) },
  {"DarkGoldenrod2"		, PALETTERGB (238, 173, 14) },
  {"DarkGoldenrod3"		, PALETTERGB (205, 149, 12) },
  {"DarkGoldenrod4"		, PALETTERGB (139, 101, 8) },
  {"RosyBrown1"			, PALETTERGB (255, 193, 193) },
  {"RosyBrown2"			, PALETTERGB (238, 180, 180) },
  {"RosyBrown3"			, PALETTERGB (205, 155, 155) },
  {"RosyBrown4"			, PALETTERGB (139, 105, 105) },
  {"IndianRed1"			, PALETTERGB (255, 106, 106) },
  {"IndianRed2"			, PALETTERGB (238, 99, 99) },
  {"IndianRed3"			, PALETTERGB (205, 85, 85) },
  {"IndianRed4"			, PALETTERGB (139, 58, 58) },
  {"sienna1"			, PALETTERGB (255, 130, 71) },
  {"sienna2"			, PALETTERGB (238, 121, 66) },
  {"sienna3"			, PALETTERGB (205, 104, 57) },
  {"sienna4"			, PALETTERGB (139, 71, 38) },
  {"burlywood1"			, PALETTERGB (255, 211, 155) },
  {"burlywood2"			, PALETTERGB (238, 197, 145) },
  {"burlywood3"			, PALETTERGB (205, 170, 125) },
  {"burlywood4"			, PALETTERGB (139, 115, 85) },
  {"wheat1"			, PALETTERGB (255, 231, 186) },
  {"wheat2"			, PALETTERGB (238, 216, 174) },
  {"wheat3"			, PALETTERGB (205, 186, 150) },
  {"wheat4"			, PALETTERGB (139, 126, 102) },
  {"tan1"			, PALETTERGB (255, 165, 79) },
  {"tan2"			, PALETTERGB (238, 154, 73) },
  {"tan3"			, PALETTERGB (205, 133, 63) },
  {"tan4"			, PALETTERGB (139, 90, 43) },
  {"chocolate1"			, PALETTERGB (255, 127, 36) },
  {"chocolate2"			, PALETTERGB (238, 118, 33) },
  {"chocolate3"			, PALETTERGB (205, 102, 29) },
  {"chocolate4"			, PALETTERGB (139, 69, 19) },
  {"firebrick1"			, PALETTERGB (255, 48, 48) },
  {"firebrick2"			, PALETTERGB (238, 44, 44) },
  {"firebrick3"			, PALETTERGB (205, 38, 38) },
  {"firebrick4"			, PALETTERGB (139, 26, 26) },
  {"brown1"			, PALETTERGB (255, 64, 64) },
  {"brown2"			, PALETTERGB (238, 59, 59) },
  {"brown3"			, PALETTERGB (205, 51, 51) },
  {"brown4"			, PALETTERGB (139, 35, 35) },
  {"salmon1"			, PALETTERGB (255, 140, 105) },
  {"salmon2"			, PALETTERGB (238, 130, 98) },
  {"salmon3"			, PALETTERGB (205, 112, 84) },
  {"salmon4"			, PALETTERGB (139, 76, 57) },
  {"LightSalmon1"		, PALETTERGB (255, 160, 122) },
  {"LightSalmon2"		, PALETTERGB (238, 149, 114) },
  {"LightSalmon3"		, PALETTERGB (205, 129, 98) },
  {"LightSalmon4"		, PALETTERGB (139, 87, 66) },
  {"orange1"			, PALETTERGB (255, 165, 0) },
  {"orange2"			, PALETTERGB (238, 154, 0) },
  {"orange3"			, PALETTERGB (205, 133, 0) },
  {"orange4"			, PALETTERGB (139, 90, 0) },
  {"DarkOrange1"		, PALETTERGB (255, 127, 0) },
  {"DarkOrange2"		, PALETTERGB (238, 118, 0) },
  {"DarkOrange3"		, PALETTERGB (205, 102, 0) },
  {"DarkOrange4"		, PALETTERGB (139, 69, 0) },
  {"coral1"			, PALETTERGB (255, 114, 86) },
  {"coral2"			, PALETTERGB (238, 106, 80) },
  {"coral3"			, PALETTERGB (205, 91, 69) },
  {"coral4"			, PALETTERGB (139, 62, 47) },
  {"tomato1"			, PALETTERGB (255, 99, 71) },
  {"tomato2"			, PALETTERGB (238, 92, 66) },
  {"tomato3"			, PALETTERGB (205, 79, 57) },
  {"tomato4"			, PALETTERGB (139, 54, 38) },
  {"OrangeRed1"			, PALETTERGB (255, 69, 0) },
  {"OrangeRed2"			, PALETTERGB (238, 64, 0) },
  {"OrangeRed3"			, PALETTERGB (205, 55, 0) },
  {"OrangeRed4"			, PALETTERGB (139, 37, 0) },
  {"red1"			, PALETTERGB (255, 0, 0) },
  {"red2"			, PALETTERGB (238, 0, 0) },
  {"red3"			, PALETTERGB (205, 0, 0) },
  {"red4"			, PALETTERGB (139, 0, 0) },
  {"DeepPink1"			, PALETTERGB (255, 20, 147) },
  {"DeepPink2"			, PALETTERGB (238, 18, 137) },
  {"DeepPink3"			, PALETTERGB (205, 16, 118) },
  {"DeepPink4"			, PALETTERGB (139, 10, 80) },
  {"HotPink1"			, PALETTERGB (255, 110, 180) },
  {"HotPink2"			, PALETTERGB (238, 106, 167) },
  {"HotPink3"			, PALETTERGB (205, 96, 144) },
  {"HotPink4"			, PALETTERGB (139, 58, 98) },
  {"pink1"			, PALETTERGB (255, 181, 197) },
  {"pink2"			, PALETTERGB (238, 169, 184) },
  {"pink3"			, PALETTERGB (205, 145, 158) },
  {"pink4"			, PALETTERGB (139, 99, 108) },
  {"LightPink1"			, PALETTERGB (255, 174, 185) },
  {"LightPink2"			, PALETTERGB (238, 162, 173) },
  {"LightPink3"			, PALETTERGB (205, 140, 149) },
  {"LightPink4"			, PALETTERGB (139, 95, 101) },
  {"PaleVioletRed1"		, PALETTERGB (255, 130, 171) },
  {"PaleVioletRed2"		, PALETTERGB (238, 121, 159) },
  {"PaleVioletRed3"		, PALETTERGB (205, 104, 137) },
  {"PaleVioletRed4"		, PALETTERGB (139, 71, 93) },
  {"maroon1"			, PALETTERGB (255, 52, 179) },
  {"maroon2"			, PALETTERGB (238, 48, 167) },
  {"maroon3"			, PALETTERGB (205, 41, 144) },
  {"maroon4"			, PALETTERGB (139, 28, 98) },
  {"VioletRed1"			, PALETTERGB (255, 62, 150) },
  {"VioletRed2"			, PALETTERGB (238, 58, 140) },
  {"VioletRed3"			, PALETTERGB (205, 50, 120) },
  {"VioletRed4"			, PALETTERGB (139, 34, 82) },
  {"magenta1"			, PALETTERGB (255, 0, 255) },
  {"magenta2"			, PALETTERGB (238, 0, 238) },
  {"magenta3"			, PALETTERGB (205, 0, 205) },
  {"magenta4"			, PALETTERGB (139, 0, 139) },
  {"orchid1"			, PALETTERGB (255, 131, 250) },
  {"orchid2"			, PALETTERGB (238, 122, 233) },
  {"orchid3"			, PALETTERGB (205, 105, 201) },
  {"orchid4"			, PALETTERGB (139, 71, 137) },
  {"plum1"			, PALETTERGB (255, 187, 255) },
  {"plum2"			, PALETTERGB (238, 174, 238) },
  {"plum3"			, PALETTERGB (205, 150, 205) },
  {"plum4"			, PALETTERGB (139, 102, 139) },
  {"MediumOrchid1"		, PALETTERGB (224, 102, 255) },
  {"MediumOrchid2"		, PALETTERGB (209, 95, 238) },
  {"MediumOrchid3"		, PALETTERGB (180, 82, 205) },
  {"MediumOrchid4"		, PALETTERGB (122, 55, 139) },
  {"DarkOrchid1"		, PALETTERGB (191, 62, 255) },
  {"DarkOrchid2"		, PALETTERGB (178, 58, 238) },
  {"DarkOrchid3"		, PALETTERGB (154, 50, 205) },
  {"DarkOrchid4"		, PALETTERGB (104, 34, 139) },
  {"purple1"			, PALETTERGB (155, 48, 255) },
  {"purple2"			, PALETTERGB (145, 44, 238) },
  {"purple3"			, PALETTERGB (125, 38, 205) },
  {"purple4"			, PALETTERGB (85, 26, 139) },
  {"MediumPurple1"		, PALETTERGB (171, 130, 255) },
  {"MediumPurple2"		, PALETTERGB (159, 121, 238) },
  {"MediumPurple3"		, PALETTERGB (137, 104, 205) },
  {"MediumPurple4"		, PALETTERGB (93, 71, 139) },
  {"thistle1"			, PALETTERGB (255, 225, 255) },
  {"thistle2"			, PALETTERGB (238, 210, 238) },
  {"thistle3"			, PALETTERGB (205, 181, 205) },
  {"thistle4"			, PALETTERGB (139, 123, 139) },
  {"gray0"			, PALETTERGB (0, 0, 0) },
  {"grey0"			, PALETTERGB (0, 0, 0) },
  {"gray1"			, PALETTERGB (3, 3, 3) },
  {"grey1"			, PALETTERGB (3, 3, 3) },
  {"gray2"			, PALETTERGB (5, 5, 5) },
  {"grey2"			, PALETTERGB (5, 5, 5) },
  {"gray3"			, PALETTERGB (8, 8, 8) },
  {"grey3"			, PALETTERGB (8, 8, 8) },
  {"gray4"			, PALETTERGB (10, 10, 10) },
  {"grey4"			, PALETTERGB (10, 10, 10) },
  {"gray5"			, PALETTERGB (13, 13, 13) },
  {"grey5"			, PALETTERGB (13, 13, 13) },
  {"gray6"			, PALETTERGB (15, 15, 15) },
  {"grey6"			, PALETTERGB (15, 15, 15) },
  {"gray7"			, PALETTERGB (18, 18, 18) },
  {"grey7"			, PALETTERGB (18, 18, 18) },
  {"gray8"			, PALETTERGB (20, 20, 20) },
  {"grey8"			, PALETTERGB (20, 20, 20) },
  {"gray9"			, PALETTERGB (23, 23, 23) },
  {"grey9"			, PALETTERGB (23, 23, 23) },
  {"gray10"			, PALETTERGB (26, 26, 26) },
  {"grey10"			, PALETTERGB (26, 26, 26) },
  {"gray11"			, PALETTERGB (28, 28, 28) },
  {"grey11"			, PALETTERGB (28, 28, 28) },
  {"gray12"			, PALETTERGB (31, 31, 31) },
  {"grey12"			, PALETTERGB (31, 31, 31) },
  {"gray13"			, PALETTERGB (33, 33, 33) },
  {"grey13"			, PALETTERGB (33, 33, 33) },
  {"gray14"			, PALETTERGB (36, 36, 36) },
  {"grey14"			, PALETTERGB (36, 36, 36) },
  {"gray15"			, PALETTERGB (38, 38, 38) },
  {"grey15"			, PALETTERGB (38, 38, 38) },
  {"gray16"			, PALETTERGB (41, 41, 41) },
  {"grey16"			, PALETTERGB (41, 41, 41) },
  {"gray17"			, PALETTERGB (43, 43, 43) },
  {"grey17"			, PALETTERGB (43, 43, 43) },
  {"gray18"			, PALETTERGB (46, 46, 46) },
  {"grey18"			, PALETTERGB (46, 46, 46) },
  {"gray19"			, PALETTERGB (48, 48, 48) },
  {"grey19"			, PALETTERGB (48, 48, 48) },
  {"gray20"			, PALETTERGB (51, 51, 51) },
  {"grey20"			, PALETTERGB (51, 51, 51) },
  {"gray21"			, PALETTERGB (54, 54, 54) },
  {"grey21"			, PALETTERGB (54, 54, 54) },
  {"gray22"			, PALETTERGB (56, 56, 56) },
  {"grey22"			, PALETTERGB (56, 56, 56) },
  {"gray23"			, PALETTERGB (59, 59, 59) },
  {"grey23"			, PALETTERGB (59, 59, 59) },
  {"gray24"			, PALETTERGB (61, 61, 61) },
  {"grey24"			, PALETTERGB (61, 61, 61) },
  {"gray25"			, PALETTERGB (64, 64, 64) },
  {"grey25"			, PALETTERGB (64, 64, 64) },
  {"gray26"			, PALETTERGB (66, 66, 66) },
  {"grey26"			, PALETTERGB (66, 66, 66) },
  {"gray27"			, PALETTERGB (69, 69, 69) },
  {"grey27"			, PALETTERGB (69, 69, 69) },
  {"gray28"			, PALETTERGB (71, 71, 71) },
  {"grey28"			, PALETTERGB (71, 71, 71) },
  {"gray29"			, PALETTERGB (74, 74, 74) },
  {"grey29"			, PALETTERGB (74, 74, 74) },
  {"gray30"			, PALETTERGB (77, 77, 77) },
  {"grey30"			, PALETTERGB (77, 77, 77) },
  {"gray31"			, PALETTERGB (79, 79, 79) },
  {"grey31"			, PALETTERGB (79, 79, 79) },
  {"gray32"			, PALETTERGB (82, 82, 82) },
  {"grey32"			, PALETTERGB (82, 82, 82) },
  {"gray33"			, PALETTERGB (84, 84, 84) },
  {"grey33"			, PALETTERGB (84, 84, 84) },
  {"gray34"			, PALETTERGB (87, 87, 87) },
  {"grey34"			, PALETTERGB (87, 87, 87) },
  {"gray35"			, PALETTERGB (89, 89, 89) },
  {"grey35"			, PALETTERGB (89, 89, 89) },
  {"gray36"			, PALETTERGB (92, 92, 92) },
  {"grey36"			, PALETTERGB (92, 92, 92) },
  {"gray37"			, PALETTERGB (94, 94, 94) },
  {"grey37"			, PALETTERGB (94, 94, 94) },
  {"gray38"			, PALETTERGB (97, 97, 97) },
  {"grey38"			, PALETTERGB (97, 97, 97) },
  {"gray39"			, PALETTERGB (99, 99, 99) },
  {"grey39"			, PALETTERGB (99, 99, 99) },
  {"gray40"			, PALETTERGB (102, 102, 102) },
  {"grey40"			, PALETTERGB (102, 102, 102) },
  {"gray41"			, PALETTERGB (105, 105, 105) },
  {"grey41"			, PALETTERGB (105, 105, 105) },
  {"gray42"			, PALETTERGB (107, 107, 107) },
  {"grey42"			, PALETTERGB (107, 107, 107) },
  {"gray43"			, PALETTERGB (110, 110, 110) },
  {"grey43"			, PALETTERGB (110, 110, 110) },
  {"gray44"			, PALETTERGB (112, 112, 112) },
  {"grey44"			, PALETTERGB (112, 112, 112) },
  {"gray45"			, PALETTERGB (115, 115, 115) },
  {"grey45"			, PALETTERGB (115, 115, 115) },
  {"gray46"			, PALETTERGB (117, 117, 117) },
  {"grey46"			, PALETTERGB (117, 117, 117) },
  {"gray47"			, PALETTERGB (120, 120, 120) },
  {"grey47"			, PALETTERGB (120, 120, 120) },
  {"gray48"			, PALETTERGB (122, 122, 122) },
  {"grey48"			, PALETTERGB (122, 122, 122) },
  {"gray49"			, PALETTERGB (125, 125, 125) },
  {"grey49"			, PALETTERGB (125, 125, 125) },
  {"gray50"			, PALETTERGB (127, 127, 127) },
  {"grey50"			, PALETTERGB (127, 127, 127) },
  {"gray51"			, PALETTERGB (130, 130, 130) },
  {"grey51"			, PALETTERGB (130, 130, 130) },
  {"gray52"			, PALETTERGB (133, 133, 133) },
  {"grey52"			, PALETTERGB (133, 133, 133) },
  {"gray53"			, PALETTERGB (135, 135, 135) },
  {"grey53"			, PALETTERGB (135, 135, 135) },
  {"gray54"			, PALETTERGB (138, 138, 138) },
  {"grey54"			, PALETTERGB (138, 138, 138) },
  {"gray55"			, PALETTERGB (140, 140, 140) },
  {"grey55"			, PALETTERGB (140, 140, 140) },
  {"gray56"			, PALETTERGB (143, 143, 143) },
  {"grey56"			, PALETTERGB (143, 143, 143) },
  {"gray57"			, PALETTERGB (145, 145, 145) },
  {"grey57"			, PALETTERGB (145, 145, 145) },
  {"gray58"			, PALETTERGB (148, 148, 148) },
  {"grey58"			, PALETTERGB (148, 148, 148) },
  {"gray59"			, PALETTERGB (150, 150, 150) },
  {"grey59"			, PALETTERGB (150, 150, 150) },
  {"gray60"			, PALETTERGB (153, 153, 153) },
  {"grey60"			, PALETTERGB (153, 153, 153) },
  {"gray61"			, PALETTERGB (156, 156, 156) },
  {"grey61"			, PALETTERGB (156, 156, 156) },
  {"gray62"			, PALETTERGB (158, 158, 158) },
  {"grey62"			, PALETTERGB (158, 158, 158) },
  {"gray63"			, PALETTERGB (161, 161, 161) },
  {"grey63"			, PALETTERGB (161, 161, 161) },
  {"gray64"			, PALETTERGB (163, 163, 163) },
  {"grey64"			, PALETTERGB (163, 163, 163) },
  {"gray65"			, PALETTERGB (166, 166, 166) },
  {"grey65"			, PALETTERGB (166, 166, 166) },
  {"gray66"			, PALETTERGB (168, 168, 168) },
  {"grey66"			, PALETTERGB (168, 168, 168) },
  {"gray67"			, PALETTERGB (171, 171, 171) },
  {"grey67"			, PALETTERGB (171, 171, 171) },
  {"gray68"			, PALETTERGB (173, 173, 173) },
  {"grey68"			, PALETTERGB (173, 173, 173) },
  {"gray69"			, PALETTERGB (176, 176, 176) },
  {"grey69"			, PALETTERGB (176, 176, 176) },
  {"gray70"			, PALETTERGB (179, 179, 179) },
  {"grey70"			, PALETTERGB (179, 179, 179) },
  {"gray71"			, PALETTERGB (181, 181, 181) },
  {"grey71"			, PALETTERGB (181, 181, 181) },
  {"gray72"			, PALETTERGB (184, 184, 184) },
  {"grey72"			, PALETTERGB (184, 184, 184) },
  {"gray73"			, PALETTERGB (186, 186, 186) },
  {"grey73"			, PALETTERGB (186, 186, 186) },
  {"gray74"			, PALETTERGB (189, 189, 189) },
  {"grey74"			, PALETTERGB (189, 189, 189) },
  {"gray75"			, PALETTERGB (191, 191, 191) },
  {"grey75"			, PALETTERGB (191, 191, 191) },
  {"gray76"			, PALETTERGB (194, 194, 194) },
  {"grey76"			, PALETTERGB (194, 194, 194) },
  {"gray77"			, PALETTERGB (196, 196, 196) },
  {"grey77"			, PALETTERGB (196, 196, 196) },
  {"gray78"			, PALETTERGB (199, 199, 199) },
  {"grey78"			, PALETTERGB (199, 199, 199) },
  {"gray79"			, PALETTERGB (201, 201, 201) },
  {"grey79"			, PALETTERGB (201, 201, 201) },
  {"gray80"			, PALETTERGB (204, 204, 204) },
  {"grey80"			, PALETTERGB (204, 204, 204) },
  {"gray81"			, PALETTERGB (207, 207, 207) },
  {"grey81"			, PALETTERGB (207, 207, 207) },
  {"gray82"			, PALETTERGB (209, 209, 209) },
  {"grey82"			, PALETTERGB (209, 209, 209) },
  {"gray83"			, PALETTERGB (212, 212, 212) },
  {"grey83"			, PALETTERGB (212, 212, 212) },
  {"gray84"			, PALETTERGB (214, 214, 214) },
  {"grey84"			, PALETTERGB (214, 214, 214) },
  {"gray85"			, PALETTERGB (217, 217, 217) },
  {"grey85"			, PALETTERGB (217, 217, 217) },
  {"gray86"			, PALETTERGB (219, 219, 219) },
  {"grey86"			, PALETTERGB (219, 219, 219) },
  {"gray87"			, PALETTERGB (222, 222, 222) },
  {"grey87"			, PALETTERGB (222, 222, 222) },
  {"gray88"			, PALETTERGB (224, 224, 224) },
  {"grey88"			, PALETTERGB (224, 224, 224) },
  {"gray89"			, PALETTERGB (227, 227, 227) },
  {"grey89"			, PALETTERGB (227, 227, 227) },
  {"gray90"			, PALETTERGB (229, 229, 229) },
  {"grey90"			, PALETTERGB (229, 229, 229) },
  {"gray91"			, PALETTERGB (232, 232, 232) },
  {"grey91"			, PALETTERGB (232, 232, 232) },
  {"gray92"			, PALETTERGB (235, 235, 235) },
  {"grey92"			, PALETTERGB (235, 235, 235) },
  {"gray93"			, PALETTERGB (237, 237, 237) },
  {"grey93"			, PALETTERGB (237, 237, 237) },
  {"gray94"			, PALETTERGB (240, 240, 240) },
  {"grey94"			, PALETTERGB (240, 240, 240) },
  {"gray95"			, PALETTERGB (242, 242, 242) },
  {"grey95"			, PALETTERGB (242, 242, 242) },
  {"gray96"			, PALETTERGB (245, 245, 245) },
  {"grey96"			, PALETTERGB (245, 245, 245) },
  {"gray97"			, PALETTERGB (247, 247, 247) },
  {"grey97"			, PALETTERGB (247, 247, 247) },
  {"gray98"			, PALETTERGB (250, 250, 250) },
  {"grey98"			, PALETTERGB (250, 250, 250) },
  {"gray99"			, PALETTERGB (252, 252, 252) },
  {"grey99"			, PALETTERGB (252, 252, 252) },
  {"gray100"			, PALETTERGB (255, 255, 255) },
  {"grey100"			, PALETTERGB (255, 255, 255) },
  {"DarkGrey"			, PALETTERGB (169, 169, 169) },
  {"DarkGray"			, PALETTERGB (169, 169, 169) },
  {"DarkBlue"			, PALETTERGB (0, 0, 139) },
  {"DarkCyan"			, PALETTERGB (0, 139, 139) },
  {"DarkMagenta"		, PALETTERGB (139, 0, 139) },
  {"DarkRed"			, PALETTERGB (139, 0, 0) },
  {"LightGreen"			, PALETTERGB (144, 238, 144) }
};

static int
hexval (char c) 
{
  /* assumes ASCII and isxdigit(c) */
  if (c >= 'a')
    return c-'a' + 10;
  else if (c >= 'A')
    return c-'A' + 10;
  else
    return c-'0';
}

COLORREF
mswindows_string_to_color(CONST char *name)
{
  int i;

  if (*name == '#')
    {
      /* numeric names look like "#RRGGBB", "#RRRGGGBBB" or "#RRRRGGGGBBBB" */
      unsigned int r, g, b;
  
      for (i=1; i<strlen(name); i++)
	{
	  if (!isxdigit (name[i]))
	    return(-1);
	}
      if (strlen(name)==7)
	{
	  r = hexval (name[1]) * 16 + hexval (name[2]);
	  g = hexval (name[3]) * 16 + hexval (name[4]);
	  b = hexval (name[5]) * 16 + hexval (name[6]);
	  return (PALETTERGB (r, g, b));
	}
      else if (strlen(name)==10)
	{
	  r = hexval (name[1]) * 16 + hexval (name[2]);
	  g = hexval (name[4]) * 16 + hexval (name[5]);
	  b = hexval (name[7]) * 16 + hexval (name[8]);
	  return (PALETTERGB (r, g, b));
	}
      else if (strlen(name)==13)
	{
	  r = hexval (name[1]) * 16 + hexval (name[2]);
	  g = hexval (name[5]) * 16 + hexval (name[6]);
	  b = hexval (name[9]) * 16 + hexval (name[10]);
	  return (PALETTERGB (r, g, b));
	}
    }
  else if (*name)	/* Can't be an empty string */
    {
      char *nospaces = alloca (strlen (name)+1);
      char *c=nospaces;
      while (*name)
	if (*name != ' ')
	  *(c++) = *(name++);
	else
	  name++;
      *c = '\0';

      for(i=0; i<(sizeof (mswindows_X_color_map) / sizeof (colormap_t)); i++)
	if (!stricmp (nospaces, mswindows_X_color_map[i].name))
	  return (mswindows_X_color_map[i].colorref);
    }
  return(-1);
}

static int
mswindows_initialize_color_instance (struct Lisp_Color_Instance *c, Lisp_Object name,
			       Lisp_Object device, Error_behavior errb)
{
  CONST char *extname;
  COLORREF color;

  GET_C_STRING_CTEXT_DATA_ALLOCA (name, extname);
  color = mswindows_string_to_color(extname);
  if (color != -1)
    {
      c->data = xnew (struct mswindows_color_instance_data);
      COLOR_INSTANCE_MSWINDOWS_COLOR (c) = color;
      COLOR_INSTANCE_MSWINDOWS_BRUSH (c) = CreateSolidBrush (color);
      return 1;
    }
  maybe_signal_simple_error ("unrecognized color", name, Qcolor, errb);
  return(0);
}

#if 0
static void
mswindows_mark_color_instance (struct Lisp_Color_Instance *c,
			 void (*markobj) (Lisp_Object))
{
}
#endif

static void
mswindows_print_color_instance (struct Lisp_Color_Instance *c,
			  Lisp_Object printcharfun,
			  int escapeflag)
{
  char buf[32];
  COLORREF color = COLOR_INSTANCE_MSWINDOWS_COLOR (c);
  sprintf (buf, " %06ld=(%04X,%04X,%04X)", color & 0xffffff,
	   GetRValue(color)*257, GetGValue(color)*257, GetBValue(color)*257);
  write_c_string (buf, printcharfun);
}

static void
mswindows_finalize_color_instance (struct Lisp_Color_Instance *c)
{
  if (c->data)
    {
      DeleteObject (COLOR_INSTANCE_MSWINDOWS_BRUSH (c));
      xfree (c->data);
      c->data = 0;
    }
}

static int
mswindows_color_instance_equal (struct Lisp_Color_Instance *c1,
			  struct Lisp_Color_Instance *c2,
			  int depth)
{
  return (COLOR_INSTANCE_MSWINDOWS_COLOR(c1) == COLOR_INSTANCE_MSWINDOWS_COLOR(c2));
}

static unsigned long
mswindows_color_instance_hash (struct Lisp_Color_Instance *c, int depth)
{
  return LISP_HASH (COLOR_INSTANCE_MSWINDOWS_COLOR(c));
}

static Lisp_Object
mswindows_color_instance_rgb_components (struct Lisp_Color_Instance *c)
{
  COLORREF color = COLOR_INSTANCE_MSWINDOWS_COLOR (c);
  return list3 (make_int (GetRValue (color) * 257),
		make_int (GetGValue (color) * 257),
		make_int (GetBValue (color) * 257));
}

static int
mswindows_valid_color_name_p (struct device *d, Lisp_Object color)
{
  CONST char *extname;

  GET_C_STRING_CTEXT_DATA_ALLOCA (color, extname);
  return (mswindows_string_to_color(extname)!=-1);
}



static void
mswindows_finalize_font_instance (struct Lisp_Font_Instance *f)
{
  if (f->data)
    {
      DeleteObject(f->data);
      f->data=0;
    }
}

static int
mswindows_initialize_font_instance (struct Lisp_Font_Instance *f, Lisp_Object name,
			      Lisp_Object device, Error_behavior errb)
{
  CONST char *extname;
  LOGFONT logfont;
  int fields;
  int pt;
  char fontname[LF_FACESIZE], weight[32], *style, points[8], effects[32], charset[32];

  GET_C_STRING_CTEXT_DATA_ALLOCA (f->name, extname);

  /*
   * mswindows fonts look like:
   *	fontname[:[weight ][style][:pointsize[:effects[:charset]]]]
   * The font name field shouldn't be empty.
   * XXX Windows will substitute a default (monospace) font if the font name
   * specifies a non-existent font. We don't catch this.
   * effects and charset are currently ignored.
   *
   * ie:
   *	Lucida Console:Regular:10
   * minimal:
   *	Courier New
   * maximal:
   *	Courier New:Bold Italic:10:underline strikeout:ansi
   */
  fields = sscanf (extname, "%31[^:]:%31[^:]:%7[^:]:%31[^:]:%31s",
		   fontname, weight, points, effects, charset);

  if (fields<0)
  {
    maybe_signal_simple_error ("Invalid font", f->name, Qfont, errb);
    return (0);
  }

  if (fields>0 && strlen(fontname))
  {
    strncpy (logfont.lfFaceName, fontname, LF_FACESIZE);
    logfont.lfFaceName[LF_FACESIZE-1] = 0;
  }
  else
  {
    maybe_signal_simple_error ("Must specify a font name", f->name, Qfont, errb);
    return (0);
  }

  if (fields > 1 && strlen(weight))
  {
    char *c;
    /* Maybe split weight into weight and style */
    if ((c=strchr(weight, ' ')))
    {
      *c = '\0';
      style = c+1;
    }
    else
      style = NULL;

    /* weight: Most-often used (maybe) first */
    if (stricmp (weight,"regular") == 0)
      logfont.lfWeight = FW_REGULAR;
    else if (stricmp (weight,"normal") == 0)
      logfont.lfWeight = FW_NORMAL;
    else if (stricmp (weight,"bold") == 0)
      logfont.lfWeight = FW_BOLD;
    else if (stricmp (weight,"medium") == 0)
      logfont.lfWeight = FW_MEDIUM;
    else if (stricmp (weight,"italic") == 0)	/* Hack for early exit */
    {
      logfont.lfItalic = TRUE;
      style=weight;
    }
    /* the rest */
    else if (stricmp (weight,"black") == 0)
      logfont.lfWeight = FW_BLACK;
    else if (stricmp (weight,"heavy") == 0)
      logfont.lfWeight = FW_HEAVY;
    else if (stricmp (weight,"ultrabold") == 0)
      logfont.lfWeight = FW_ULTRABOLD;
    else if (stricmp (weight,"extrabold") == 0)
      logfont.lfWeight = FW_EXTRABOLD;
    else if (stricmp (weight,"demibold") == 0)
      logfont.lfWeight = FW_SEMIBOLD;
    else if (stricmp (weight,"semibold") == 0)
      logfont.lfWeight = FW_SEMIBOLD;
    else if (stricmp (weight,"light") == 0)
      logfont.lfWeight = FW_LIGHT;
    else if (stricmp (weight,"ultralight") == 0)
      logfont.lfWeight = FW_ULTRALIGHT;
    else if (stricmp (weight,"extralight") == 0)
      logfont.lfWeight = FW_EXTRALIGHT;
    else if (stricmp (weight,"thin") == 0)
      logfont.lfWeight = FW_THIN;
    else
    {
      logfont.lfWeight = FW_NORMAL;
      if (!style)
	style = weight;	/* May have specified a style without a weight */
      else
      {
        maybe_signal_simple_error ("Invalid font weight", f->name, Qfont, errb);
	return (0);	/* Invalid weight */
      }
    }

    if (style)
    {
      /* XXX what about oblique? */
      if (stricmp (style,"italic") == 0)
	logfont.lfItalic = TRUE;
      else if (stricmp (style,"roman") == 0)
	logfont.lfItalic = FALSE;
      else
      {
        maybe_signal_simple_error ("Invalid font weight or style", f->name, Qfont, errb);
	return (0);	/* Invalid weight or style */
      }
    }
    else
    {
      logfont.lfItalic = FALSE;
    }

  }
  else
  {
    logfont.lfWeight = FW_NORMAL;
    logfont.lfItalic = FALSE;
  }

  /* XXX Should we reject strings that don't specify a size? */
  if (fields < 3 || !strlen(points) || (pt=atoi(points))==0)
    pt = 10;

  /* Formula for pointsize->height from LOGFONT docs in MSVC5 Platform SDK */
  logfont.lfHeight = -MulDiv(pt, DEVICE_MSWINDOWS_LOGPIXELSY(XDEVICE (device)), 72);
  logfont.lfWidth = 0;

  /* Default to monospaced if the specified font name is not found */
  logfont.lfPitchAndFamily = FF_MODERN;

  /* XXX: FIXME? */
  logfont.lfUnderline = FALSE;
  logfont.lfStrikeOut = FALSE;

  /* XXX: FIXME: we ignore charset */
  logfont.lfCharSet = DEFAULT_CHARSET;

  /* Misc crud */
  logfont.lfEscapement = logfont.lfOrientation = 0;
#if 1
  logfont.lfOutPrecision = OUT_DEFAULT_PRECIS;
  logfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
  logfont.lfQuality = DEFAULT_QUALITY;
#else
  logfont.lfOutPrecision = OUT_STROKE_PRECIS;
  logfont.lfClipPrecision = CLIP_STROKE_PRECIS;
  logfont.lfQuality = PROOF_QUALITY;
#endif

  if ((f->data = CreateFontIndirect(&logfont)) == NULL)
  {
    maybe_signal_simple_error ("Couldn't create font", f->name, Qfont, errb);
    return 0;
  }

  /* Have to apply Font to a GC to get its values.
   * We'll borrow the desktop window becuase its the only window that we
   * know about that is guaranteed to exist when this gets called
   */ 
  {
    HWND hwnd;
    HDC hdc;
    HFONT holdfont;
    TEXTMETRIC metrics;

    hwnd = GetDesktopWindow();
    assert(hdc = GetDC(hwnd));	/* XXX FIXME: can this temporarily fail? */
    holdfont = SelectObject(hdc, f->data);
    if (!holdfont)
    {
      mswindows_finalize_font_instance (f);
      maybe_signal_simple_error ("Couldn't map font", f->name, Qfont, errb);
      return 0;
    }
    GetTextMetrics(hdc, &metrics);
    SelectObject(hdc, holdfont);
    ReleaseDC(hwnd, hdc);
    f->width = (unsigned short) metrics.tmAveCharWidth;
    f->height = (unsigned short) metrics.tmHeight;
    f->ascent = (unsigned short) metrics.tmAscent;
    f->descent = (unsigned short) metrics.tmDescent;
    f->proportional_p = (metrics.tmPitchAndFamily & TMPF_FIXED_PITCH);
  }

  return 1;
}

#if 0
static void
mswindows_mark_font_instance (struct Lisp_Font_Instance *f,
			void (*markobj) (Lisp_Object))
{
}
#endif

static void
mswindows_print_font_instance (struct Lisp_Font_Instance *f,
			 Lisp_Object printcharfun,
			 int escapeflag)
{
}

static Lisp_Object
mswindows_list_fonts (Lisp_Object pattern, Lisp_Object device)
{
  /* XXX Implement me */
  return list1 (build_string ("Courier New:Regular:10"));
}

#ifdef MULE

static int
mswindows_font_spec_matches_charset (struct device *d, Lisp_Object charset,
			     CONST Bufbyte *nonreloc, Lisp_Object reloc,
			     Bytecount offset, Bytecount length)
{
  /* XXX Implement me */
  if (UNBOUNDP (charset))
    return 1;
  
  return 1;
}

/* find a font spec that matches font spec FONT and also matches
   (the registry of) CHARSET. */
static Lisp_Object
mswindows_find_charset_font (Lisp_Object device, Lisp_Object font,
		     Lisp_Object charset)
{
  /* XXX Implement me */
  return build_string ("Courier New:Regular:10");
}

#endif /* MULE */


/************************************************************************/
/*                            initialization                            */
/************************************************************************/

void
syms_of_objects_mswindows (void)
{
}

void
console_type_create_objects_mswindows (void)
{
  /* object methods */
  CONSOLE_HAS_METHOD (mswindows, initialize_color_instance);
/*  CONSOLE_HAS_METHOD (mswindows, mark_color_instance); */
  CONSOLE_HAS_METHOD (mswindows, print_color_instance);
  CONSOLE_HAS_METHOD (mswindows, finalize_color_instance);
  CONSOLE_HAS_METHOD (mswindows, color_instance_equal);
  CONSOLE_HAS_METHOD (mswindows, color_instance_hash);
  CONSOLE_HAS_METHOD (mswindows, color_instance_rgb_components);
  CONSOLE_HAS_METHOD (mswindows, valid_color_name_p);

  CONSOLE_HAS_METHOD (mswindows, initialize_font_instance);
/*  CONSOLE_HAS_METHOD (mswindows, mark_font_instance); */
  CONSOLE_HAS_METHOD (mswindows, print_font_instance);
  CONSOLE_HAS_METHOD (mswindows, finalize_font_instance);
/*  CONSOLE_HAS_METHOD (mswindows, font_instance_truename); */
  CONSOLE_HAS_METHOD (mswindows, list_fonts);
#ifdef MULE
  CONSOLE_HAS_METHOD (mswindows, font_spec_matches_charset);
  CONSOLE_HAS_METHOD (mswindows, find_charset_font);
#endif
}

void
vars_of_objects_mswindows (void)
{
}
