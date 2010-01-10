//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2010 by Bradford W. Mott and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef GL_SHADER_PROGS_HXX
#define GL_SHADER_PROGS_HXX

/**
  This code is generated using the 'create_shaders.pl' script,
  located in the src/tools directory.
*/

namespace GLShader {

static const char* bleed_frag[] = {
"uniform sampler2D tex;\n"
"uniform float pH;\n"
"uniform float pW;\n"
"uniform float pWx2;\n"
"\n"
"void main()\n"
"{\n"
"	// Save current color\n"
"	vec4 current = texture2D(tex, vec2(gl_TexCoord[0].s, gl_TexCoord[0].t));\n"
"\n"
"	// Box filter\n"
"	// Comments for position are given in (x,y) coordinates with the current pixel as the origin\n"
"	vec4 color = ( \n"
"		// (-1,1)\n"
"		texture2D(tex, vec2(gl_TexCoord[0].s-pW, gl_TexCoord[0].t+pH))\n"
"		// (0,1)\n"
"		+ texture2D(tex, vec2(gl_TexCoord[0].s, gl_TexCoord[0].t+pH))\n"
"		// (1,1)\n"
"		+ texture2D(tex, vec2(gl_TexCoord[0].s+pW, gl_TexCoord[0].t+pH))\n"
"		// (-1,0)\n"
"		+ texture2D(tex, vec2(gl_TexCoord[0].s-pW, gl_TexCoord[0].t))\n"
"		// (0,0)\n"
"		+ current\n"
"		// (1,0)\n"
"		+ texture2D(tex, vec2(gl_TexCoord[0].s+pW, gl_TexCoord[0].t))\n"
"		// (-1,-1)\n"
"		+ texture2D(tex, vec2(gl_TexCoord[0].s-pW, gl_TexCoord[0].t-pH))\n"
"		// (0,-1)\n"
"		+ texture2D(tex, vec2(gl_TexCoord[0].s, gl_TexCoord[0].t-pH))\n"
"		// (1,-1)\n"
"		+ texture2D(tex, vec2(gl_TexCoord[0].s+pW, gl_TexCoord[0].t-pH))\n"
"\n"
"		// Make it wider\n"
"		// (-2,1)\n"
"		+ texture2D(tex, vec2(gl_TexCoord[0].s-pWx2, gl_TexCoord[0].t+pH))\n"
"		// (-2,0)\n"
"		+ texture2D(tex, vec2(gl_TexCoord[0].s-pWx2, gl_TexCoord[0].t))\n"
"		// (-2,-1)\n"
"		+ texture2D(tex, vec2(gl_TexCoord[0].s-pWx2, gl_TexCoord[0].t-pH))\n"
"		// (2,1)\n"
"		+ texture2D(tex, vec2(gl_TexCoord[0].s+pWx2, gl_TexCoord[0].t+pH))\n"
"		// (2,0)\n"
"		+ texture2D(tex, vec2(gl_TexCoord[0].s+pWx2, gl_TexCoord[0].t))\n"
"		// (2,-1)\n"
"		+ texture2D(tex, vec2(gl_TexCoord[0].s+pWx2, gl_TexCoord[0].t-pH))\n"
"		) / 15.0;\n"
"\n"
"	// Make darker colors not bleed over lighter colors (act like light)\n"
"	color =  vec4(max(current.x, color.x), max(current.y, color.y), max(current.z, color.z), 1.0);\n"
"\n"
"	gl_FragColor = color;\n"
"}\n"
"\0"
};

static const char* noise_frag[] = {
"uniform sampler2D tex;\n"
"uniform sampler2D mask;\n"
"\n"
"void main()\n"
"{\n"
"	gl_FragColor =\n"
"		texture2D(tex, vec2(gl_TexCoord[0].s, gl_TexCoord[0].t))\n"
"		+ texture2D(mask, vec2(gl_TexCoord[1].s, gl_TexCoord[1].t))\n"
"	;\n"
"}\n"
"\0"
};

static const char* phosphor_frag[] = {
"uniform sampler2D tex;\n"
"uniform sampler2D mask;\n"
"\n"
"void main()\n"
"{\n"
"	gl_FragColor =\n"
"		0.65 * texture2D(tex, vec2(gl_TexCoord[0].s, gl_TexCoord[0].t))\n"
"		+ 0.35 * texture2D(mask, vec2(gl_TexCoord[1].s, gl_TexCoord[1].t))\n"
"	;\n"
"}\n"
"\0"
};

static const char* texture_frag[] = {
"uniform sampler2D tex;\n"
"uniform sampler2D mask;\n"
"\n"
"void main()\n"
"{\n"
"	gl_FragColor =\n"
"		texture2D(tex, vec2(gl_TexCoord[0].s, gl_TexCoord[0].t))\n"
"		* texture2D(mask, vec2(gl_TexCoord[1].s, gl_TexCoord[1].t))\n"
"		* 1.05\n"
"		+ 0.07\n"
"	;\n"
"}\n"
"\0"
};

static const char* texture_noise_frag[] = {
"uniform sampler2D tex;\n"
"uniform sampler2D texMask;\n"
"uniform sampler2D noiseMask;\n"
"\n"
"void main()\n"
"{\n"
"	gl_FragColor =\n"
"		// Texture part\n"
"		texture2D(tex, vec2(gl_TexCoord[0].s, gl_TexCoord[0].t))\n"
"		* texture2D(texMask, vec2(gl_TexCoord[1].s, gl_TexCoord[1].t))\n"
"		* 1.05\n"
"		+ 0.07\n"
"		// Noise part\n"
"		+ texture2D(noiseMask, vec2(gl_TexCoord[1].s, gl_TexCoord[1].t))\n"
"	;\n"
"}\n"
"\0"
};

} // namespace GLShader

#endif
