//These are just the text to speech rules.  For various reasons, we may
//transform their representation.  This is the straightforward 'master'
//encoding from which more compact but less readable forms are derived.


#include "tts_rules.h"
#include <stddef.h>


//=========================================================================
//Text2Speech rules


//This implementation is derived from the following research:
/*
AUTOMATIC TRANSLATION OF ENGLISH TEXT TO PHONETICS
BY MEANS OF LETTER-TO-SOUND RULES

NRL Report 7948

January 21st, 1976
Naval Research Laboratory, Washington, D.C.

Published by the National Technical Information Service as
document "AD/A021 929".
*/

//additionally, this implementation is derived from a work by
//John A. Wasser which the author placed into the public domain.

//additionally, this implementation uses additional rules
//presumably developed by Tom Jennings for his t2a program.

//additionally, I (ziggurat29) added a couple mods of my own here and there

//'rules' are a tuple of ( "left context", "bracket context", "right context", "phoneme list" )
//the way rules work are that the prefix, bracket, suffix must match literally.  If they
//do, then the phoneme list is emitted.

//I called the middle part that is being matched and replaced, the 'bracket' context,
//because in the original text the rules are written as:
//   a[b]c=d



//the NRL Report 7948 describes a system containing a small-ish set of rules
//that match text patterns, replacing them with phoneme sequences.  As
//described, the rules work by attempting to match a text sequence (to be
//replaced) with the match being subject to the 'context' of the surrounding
//characters (which contribute to the match, but are not themselves
//replaced upon match).

//the left and right context matches have some enhancements:
//a literal match for alphabetic characters, and the apostrophe, and space, 
//and some meta character classes represented by these symbols:
//  #  one or more vowels
//  :  zero or more consonants
//  ^  one consonant
//  .  one voiced consonant
//  %  'e'-related things at the end of the word '-e', '-ed', '-er', '-es', '-ely', '-ing'
//  +  'front' vowels 'e', 'i', 'y'
//  $  beginning or end of a word

//note:  I originally intended to use regexes instead of this bespoke
//implementation, but there were too many rules to hold all the machines,
//and anyway that would be less easily to port to MCUs.


//To expedite the matching process, the rules are grouped according to the first
//character in the left context.  This tweak avoids testing most of the rules
//that have no chance of matching.

//A group of rules is processed linearly, so more specific rules should precede
//more general ones; the last rule should be a catchall for the group.

//The empty string represents 'anything', and the '$' represents 'beginning or
//end'.

//YYY maybe make the metacharacters high ascii to get them out of the printable
//range?


//(syntatic sugar for readability)
#define Silent { "", 0 }

//symbolic 'constants' for readability
//note: we're adding 1 to the value so that we can use a macro to make it into
//a nul-terminated string.  Consequently, the code will need to -1 all the
//values when generating the actual sequence.
#define PA1  "\x01"	//PAUSE		 1OMS
#define PA2  "\x02"	//PAUSE		 30MS
#define PA3  "\x03"	//PAUSE		 50MS
#define PA4  "\x04"	//PAUSE		1OOMS
#define PA5  "\x05"	//PAUSE		200MS
#define OY   "\x06"	//bOY		420MS
#define AY   "\x07"	//skY		260MS
#define EH   "\x08"	//End		 70MS
#define KK3  "\x09"	//Comb		120MS
#define PP   "\x0a"	//Pow		21OMS
#define JH   "\x0b"	//doDGe		140MS
#define NN1  "\x0c"	//thiN		140MS
#define IH   "\x0d"	//sIt		 70MS
#define TT2  "\x0e"	//To		140MS
#define RR1  "\x0f"	//Rural		170MS
#define AX   "\x0f"	//sUcceed	 70MS
#define AH   "\x10"	//(pseudo-phoneme)	XXX have to fixup rules; scrutinize this
#define MM   "\x11"	//Milk		180MS
#define TT1  "\x12"	//parT		1OOMS
#define DH1  "\x13"	//THey		290MS
#define IY   "\x14"	//sEE		250MS
#define EY   "\x15"	//bEIge		280MS
#define DD1  "\x16"	//coulD		 70MS
#define UW1  "\x17"	//tO		1OOMS
#define AO   "\x18"	//AUght		1OOMS
#define AA   "\x19"	//hOt		1OOMS
#define YY2  "\x1a"	//Yes		180MS
#define AE   "\x1b"	//hAt		120MS
#define HH1  "\x1c"	//He		130MS
#define BB1  "\x1d"	//Business	 80MS
#define TH   "\x1e"	//THin		180MS
#define UH   "\x1f"	//bOOk		100MS
#define UW2  "\x20"	//fOOd		260MS
#define AW   "\x21"	//OUt		370MS
#define DD2  "\x22"	//Do		160MS
#define GG3  "\x23"	//wiG		140MS
#define VV   "\x24"	//Vest		19OMS
#define GG1  "\x25"	//Got		 80MS
#define SH   "\x26"	//SHip		160MS
#define ZH   "\x27"	//aZure		190MS
#define RR2  "\x28"	//bRain		12OMS
#define FF   "\x29"	//Food		150MS
#define KK2  "\x2a"	//sKy		190MS
#define KK1  "\x2b"	//Can't		160MS
#define ZZ   "\x2c"	//Zoo		21OMS
#define NG   "\x2d"	//aNchor	220MS
#define LL   "\x2e"	//Lake		110MS
#define WW   "\x2f"	//Wool		180MS
#define XR   "\x30"	//repAIR	360MS
#define WH   "\x31"	//WHig		200MS
#define YY1  "\x32"	//Yes		130MS
#define CH   "\x33"	//CHurch	190MS
#define ER1  "\x34"	//fIR		160MS
#define ER2  "\x35"	//fIR		300MS
#define OW   "\x36"	//bEAU		240MS
#define DH2  "\x37"	//THey		240MS
#define SS   "\x38"	//veSt		 90MS
#define NN2  "\x39"	//No		190MS
#define HH2  "\x3a"	//Hoe		180MS
#define OR   "\x3b"	//stORe		330MS
#define AR   "\x3c"	//alARm		290MS
#define YR   "\x3d"	//clEAR		350MS
#define GG2  "\x3e"	//Guest		 40MS
#define EL   "\x3f"	//saddLe	190MS
#define BB2  "\x40"	//Business	 50MS



#define PHONESEQ(a) { a, sizeof(a)-1 }



//0 - punctuation
const TTSRule r_punc[] = {
	{ Anything,		" ",		Anything,		PHONESEQ( PA4 PA3 )},
	{ Anything,		"-",		Anything,		PHONESEQ( PA4 ) },
	{ ".",			"'s",		Anything,		PHONESEQ( ZZ ) },
	{ "#:.e",		"'s",		Anything,		PHONESEQ( ZZ ) },
	{ "#",			"'s",		Anything,		PHONESEQ( ZZ ) },
	{ Anything,		"'",		Anything,		PHONESEQ( PA1 ) },
	{ Anything,		";",		Anything,		PHONESEQ( PA5 ) },
	{ Anything,		":",		Anything,		PHONESEQ( PA5 ) },
	{ Anything,		",",		Anything,		PHONESEQ( PA5 ) },

	{ Anything,		".",		"#",			Silent},
	{ Anything,		".",		"^",			Silent},
	{ Anything,		".",		Anything,		PHONESEQ( PA5 PA5 PA4 ) },

	{ Anything,		"?",		Anything,		PHONESEQ( PA5 PA5 PA4 ) },
	{ Anything,		"!",		Anything,		PHONESEQ(  PA5 PA5 PA4 ) },
	{ NULL, NULL, NULL, { NULL, 0 } },	//sentinel
};


//1 - a
const TTSRule r_a[] = {
	{ Nothing,		"a",		Nothing,		PHONESEQ( EH EY ) },
	{ Anything,		"ahead",	Anything,		PHONESEQ( AX HH1 EH EH DD1 ) },
	{ Anything,		"apropos",	Anything,		PHONESEQ( AE PP ER1 OW PP OW ) },
	{ Anything,		"ass",		"h",			PHONESEQ( AE AE SS SS ) },
	{ Anything,		"allege",	Anything,		PHONESEQ( AX LL EH DD2 JH ) },
	{ Anything,		"again",	Anything,		PHONESEQ( AX GG3 EH EH NN1 ) },
	{ Nothing,		"able",		Anything,		PHONESEQ( EY HH1 BB2 AX LL ) },
	{ Nothing,		"above",	Nothing,		PHONESEQ( AX BB2 AX AX VV HH1 ) },
	{ Nothing,		"acro",		".",			PHONESEQ( AE HH1 KK1 ER1 OW ) },
	{ Nothing,		"are",		Nothing,		PHONESEQ( AA ER2 ) },
	{ Nothing,		"ally",		Nothing,		PHONESEQ( AE AE LL AY ) },
	{ Anything,		"atomic",	Anything,		PHONESEQ( AX TT2 AA MM PA1 IH KK1 ) },
	{ Anything,		"arch",		"#v",			PHONESEQ( AX AX ER1 PA1 KK1 IH ) },
	{ Anything,		"arch",		"#.",			PHONESEQ( AX AX ER1 CH IH ) },
	{ Anything,		"arch",		"#^",			PHONESEQ( AX AX ER1 KK1 PA1 IH ) },
	{ Anything,		"argue",	Anything,		PHONESEQ( AA ER2 GG1 YY2 UW2 ) },

	{ Nothing,		"abb",		Anything,		PHONESEQ( AX AX BB2 ) },
	{ Nothing,		"ab",		Anything,		PHONESEQ( AE AE BB1 PA2 ) },
	{ Nothing,		"an",		"#",			PHONESEQ( AE NN1 ) },
	{ Nothing,		"allo",		"t",			PHONESEQ( AE LL AA ) },
	{ Nothing,		"allo",		"w",			PHONESEQ( AE LL AW ) },
	{ Nothing,		"allo",		Anything,		PHONESEQ( AE LL OW ) },
	{ Nothing,		"ar",		"o",			PHONESEQ( AX ER2 ) },

	{ "#:",			"ally",		Anything,		PHONESEQ( PA1 AX LL IY ) },
	{ "^",			"able",		Anything,		PHONESEQ( PA1 EY HH1 BB2 AX LL ) },
	{ Anything,		"able",		Anything,		PHONESEQ( PA1 AX HH1 BB2 AX LL ) },
	{ "^",			"ance",		Anything,		PHONESEQ( PA1 AE NN1 SS ) },
	{ Anything,		"air",		Anything,		PHONESEQ( EY XR ) },
	{ Anything,		"aic",		Nothing,		PHONESEQ( EY IH KK1 ) },
	{ "#:",			"als",		Nothing,		PHONESEQ( AX LL ZZ ) },
	{ Anything,		"alk",		Anything,		PHONESEQ( AO AO KK1 ) },
	{ Anything,		"arr",		Anything,		PHONESEQ( AA ER1 ) },
	{ Anything,		"ang",		"+",			PHONESEQ( EY NN1 JH ) },
	{ Nothing ":",	"any",		Anything,		PHONESEQ( EH NN1 IY ) },
	{ Anything,		"ary",		Nothing,		PHONESEQ( PA1 AX ER2 IY ) },
	{ "^",			"as",		"#",			PHONESEQ( EY SS ) },
	{ "#:",			"al",		Nothing,		PHONESEQ( AX LL ) },
	{ Anything,		"al",		"^",			PHONESEQ( AO LL ) },
	{ Nothing,		"al",		"#",			PHONESEQ( EH EY LL ) },
	{ "#:",			"ag",		"e",			PHONESEQ( IH JH ) },

	{ Anything,		"ai",		Anything,		PHONESEQ( EH EY ) },
	{ Anything,		"ay",		Anything,		PHONESEQ( EH EY ) },
	{ Anything,		"au",		Anything,		PHONESEQ( AO AO ) },
	{ Anything,		"aw",		Nothing,		PHONESEQ( AO AO ) },
	{ Anything,		"aw",		"^",			PHONESEQ( AO AO ) },
	{ ":",			"ae",		Anything,		PHONESEQ( EH ) },
	{ Anything,		"a",		"tion",			PHONESEQ( EY ) },
	{ "c",			"a",		"bl",			PHONESEQ( EH EY ) },
	{ "c",			"a",		"b#",			PHONESEQ( AE AE ) },
	{ "c",			"a",		"pab",			PHONESEQ( EH EY ) },
	{ "c",			"a",		"p#",			PHONESEQ( AE AE ) },
	{ "c",			"a",		"t#^",			PHONESEQ( AE AE ) },

	{ "^^^",		"a",		Anything,		PHONESEQ( EY ) },
	{ "^.",			"a",		"^e",			PHONESEQ( EY ) },
	{ "^.",			"a",		"^i",			PHONESEQ( EY ) },
	{ "^^",			"a",		Anything,		PHONESEQ( AE ) },
	{ "^",			"a",		"^##",			PHONESEQ( EY ) },
	{ "^",			"a",		"^#",			PHONESEQ( EY ) },
	{ "^",			"a",		"^#",			PHONESEQ( EH EY ) },
	{ Anything,		"a",		"^%",			PHONESEQ( EY ) },
	{ "#",			"a",		Nothing,		PHONESEQ( AO ) },
	{ Anything,		"a",		"wa",			PHONESEQ( AX ) },
	{ Anything,		"a",		Nothing,		PHONESEQ( AX ) },
	{ Anything,		"a",		"^+#",			PHONESEQ( EY ) },
	{ Anything,		"a",		"^+:#",			PHONESEQ( AE ) },
	{ Nothing ":",	"a",		"^+" Nothing,	PHONESEQ( EY ) },

	{ Anything,		"a",		Anything,		PHONESEQ( AE ) },
	{ NULL, NULL, NULL, { NULL, 0 } },	//sentinel
};

//2 - b
const TTSRule r_b[] = {
	{ "b",			"b",		Anything,		Silent },
	{ Anything,		"bi",		"cycle",		PHONESEQ( BB2 AY ) },
	{ Anything,		"bi",		"cycle",		PHONESEQ( BB2 AY ) },
	{ Anything,		"bbq",		Anything,		PHONESEQ( BB2 AX AX ER1 BB2 AX KK2 YY2 UW2 ) },
	{ Anything,		"barbeque",	Anything,		PHONESEQ( BB2 AX AX ER1 BB2 AX KK2 YY2 UW2 ) },
	{ Anything,		"barbaque",	Anything,		PHONESEQ( BB2 AX AX ER1 BB2 AX KK2 YY2 UW2 ) },
	{ Anything,		"bargain",	Anything,		PHONESEQ( BB2 AO ER1 GG1 EH NN1 ) },
	{ Anything,		"bagel",	Anything,		PHONESEQ( BB2 EY GG1 EH LL ) },
	{ Anything,		"being",	Anything,		PHONESEQ( BB2 IY IH NG ) },
	{ Anything,		"bomb",		Anything,		PHONESEQ( BB2 AA AA MM ) },
	{ Nothing,		"both",		Nothing,		PHONESEQ( BB2 OW TH ) },
	{ Anything,		"buil",		Anything,		PHONESEQ( BB2 IH LL ) },
	{ Nothing,		"bus",		"y",			PHONESEQ( BB2 IH ZZ ) },
	{ Nothing,		"bus",		"#",			PHONESEQ( BB2 IH ZZ ) },
	{ Anything,		"bye",		Anything,		PHONESEQ( BB2 AO AY ) },
	{ Anything,		"bear",		Nothing,		PHONESEQ( BB2 EY ER2 ) },
	{ Anything,		"bear",		"%",			PHONESEQ( BB2 EY ER2 ) },
	{ Anything,		"bear",		"s",			PHONESEQ( BB2 EY ER2 ) },
	{ Anything,		"bear",		"#",			PHONESEQ( BB2 EY ER2 ) },
	{ Nothing,		"beau",		Anything,		PHONESEQ( BB2 OW ) },

	{ Anything,		"ban",		"ish",			PHONESEQ( BB2 AE AE NN1 ) },

	{ Nothing,		"be",		"^#",			PHONESEQ( BB2 IH ) },
	{ Nothing,		"by",		Anything,		PHONESEQ( BB2 AO AY ) },
	{ "y",			"be",		Nothing,		PHONESEQ( BB2 IY ) },

	{ Nothing,		"b",		"#",			PHONESEQ( BB2 ) },
	{ Anything,		"b",		Nothing,		PHONESEQ( BB1 ) },
	{ Anything,		"b",		"#",			PHONESEQ( BB1 ) },
	{ Anything,		"b",		"l",			PHONESEQ( BB1 ) },
	{ Anything,		"b",		"r",			PHONESEQ( BB1 ) },

	{ Anything,		"b",		Anything,		PHONESEQ( BB2 ) },
	{ NULL, NULL, NULL, { NULL, 0 } },	//sentinel
};

//3 - c
const TTSRule r_c[] = {
	{ Anything,		"chinese",	Anything,		PHONESEQ( CH AY NN1 IY SS ) },
	{ Anything,		"country",	Anything,		PHONESEQ( KK1 AX AX NN1 TT2 ER1 IY ) },
	{ Anything,		"christ",	Nothing,		PHONESEQ( KK3 ER1 AY SS TT2 ) },
	{ Anything,		"chassis",	Anything,		PHONESEQ( CH AX AX SS IY ) },
	{ Anything,		"closet",	Anything,		PHONESEQ( KK3 LL AO AO ZZ EH TT2 ) },
	{ Anything,		"china",	Anything,		PHONESEQ( CH AY NN1 AX ) },
	{ Nothing,		"cafe",		Nothing,		PHONESEQ( KK1 AE FF AE EY ) },
	{ Anything,		"cele",		Anything,		PHONESEQ( SS EH LL PA1 EH ) },
	{ Anything,		"cycle",	Anything,		PHONESEQ( SS AY KK3 UH LL ) },
	{ Anything,		"chron",	Anything,		PHONESEQ( KK1 ER1 AO NN1 ) },
	{ Anything,		"crea",		"t",			PHONESEQ( KK3 ER1 IY EY ) },
	{ Nothing,		"cry",		Nothing,		PHONESEQ( KK3 ER1 IY ) },
	{ Nothing,		"chry",		Anything,		PHONESEQ( KK3 ER1 AO AY ) },
	{ Nothing,		"cry",		"#",			PHONESEQ( KK3 ER1 AO AY ) },
	{ Nothing,		"caveat",	":",			PHONESEQ( KK1 AE VV IY AE TT2 ) },
	{ "^",			"cuit",		Anything,		PHONESEQ( KK1 IH TT2 ) },
	{ Anything,		"chaic",	Anything,		PHONESEQ( KK1 EY IH KK1 ) },
	{ Anything,		"cation",	Anything,		PHONESEQ( KK1 EY SH AX NN1 ) },
	{ Nothing,		"ch",		"aract",		PHONESEQ( KK1 ) },
	{ Nothing,		"ch",		"^",			PHONESEQ( KK1 ) },
	{ "^e",			"ch",		Anything,		PHONESEQ( KK1 ) },
	{ Anything,		"ch",		Anything,		PHONESEQ( CH ) },
	{ Nothing "s",	"ci",		"#",			PHONESEQ( SS AY ) },
	{ Anything,		"ci",		"a",			PHONESEQ( SH ) },
	{ Anything,		"ci",		"o",			PHONESEQ( SH ) },
	{ Anything,		"ci",		"en",			PHONESEQ( SH ) },
	{ Anything,		"c",		"+",			PHONESEQ( SS ) },
	{ Anything,		"ck",		Anything,		PHONESEQ( KK2 ) },
	{ Anything,		"com",		"%",			PHONESEQ( KK1 AH MM ) },
	//{ Anything,	"c",		"^",			PHONESEQ( KK3 ) },

	{ Anything,		"c",		"u",			PHONESEQ( KK3 ) },
	{ Anything,		"c",		"o",			PHONESEQ( KK3 ) },
	{ Anything,		"c",		"a^^",			PHONESEQ( KK3 ) },
	{ Anything,		"c",		"o^^",			PHONESEQ( KK3 ) },
	{ Anything,		"c",		"l",			PHONESEQ( KK3 ) },
	{ Anything,		"c",		"r",			PHONESEQ( KK3 ) },

	{ Anything,		"c",		"a",			PHONESEQ( KK1 ) },
	{ Anything,		"c",		"e",			PHONESEQ( KK1 ) },
	{ Anything,		"c",		"i",			PHONESEQ( KK1 ) },

	{ Anything,		"c",		Nothing,		PHONESEQ( KK2 ) },
	{ Anything,		"c",		Anything,		PHONESEQ( KK1 ) },
	{ NULL, NULL, NULL, { NULL, 0 } },	//sentinel
};

//4 - d
const TTSRule r_d[] = {
	{ Anything,		"dead",		Anything,		PHONESEQ( DD2 EH EH DD1 ) },
	{ Nothing,		"dogged",	Anything,		PHONESEQ( DD2 AO GG1 PA1 EH DD1 ) },
	{ "#:",			"ded",		Nothing,		PHONESEQ( DD2 IH DD1 ) },
	{ Nothing,		"dig",		Anything,		PHONESEQ( DD2 IH IH GG1 ) },
	{ Nothing,		"dry",		Nothing,		PHONESEQ( DD2 ER1 AO AY ) },
	{ Nothing,		"dry",		"#",			PHONESEQ( DD2 ER1 AO AY ) },
	{ Nothing,		"de",		"^#",			PHONESEQ( DD2 IH ) },
	{ Nothing,		"do",		Nothing,		PHONESEQ( DD2 UW2 ) },
	{ Nothing,		"does",		Anything,		PHONESEQ( DD2 AH ZZ ) },
	{ Nothing,		"doing",	Anything,		PHONESEQ( UW2 IH NG ) },
	{ Nothing,		"dow",		Anything,		PHONESEQ( DD2 AW ) },
	{ Anything,		"du",		"a",			PHONESEQ( JH UW2 ) },
	{ Anything,		"dyna",		Anything,		PHONESEQ( DD2 AY NN1 AX PA1 ) },
	{ Anything,		"dyn",		"#",			PHONESEQ( DD2 AY NN1 PA1 ) },
	{ "d",			"d",		Anything,		Silent },
	{ Anything,		"d",		Nothing,		PHONESEQ( DD1 ) },
	{ Nothing,		"d",		Anything,		PHONESEQ( DD2 ) },
	{ Anything,		"d",		Anything,		PHONESEQ( DD2 ) },
	{ NULL, NULL, NULL, { NULL, 0 } },	//sentinel
};

//5 - e
const TTSRule r_e[] = {
	{ Nothing,		"eye",		Anything,		PHONESEQ( AA AY ) },
	{ Anything,		"ered",		Nothing,		PHONESEQ( ER2 DD1 ) },
	{ Nothing,		"ego",		Anything,		PHONESEQ( IY GG1 OW ) },
	{ Nothing,		"err",		Anything,		PHONESEQ( EH EH ER1 ) },
	{ "^",			"err",		Anything,		PHONESEQ( EH EH ER1 ) },
	{ Anything,		"ev",		"er",			PHONESEQ( EH EH VV HH1 ) },
	{ Anything,		"e",		"ness",			Silent },
	//{ Anything,		"e",		"^%",		PHONESEQ( IY ) },
	{ Anything,		"eri",		"#",			PHONESEQ( IY XR IY ) },
	{ Anything,		"eri",		Anything,		PHONESEQ( EH ER1 IH ) },
	{ "#:",			"er",		"#",			PHONESEQ( ER2 ) },
	{ Anything,		"er",		"#",			PHONESEQ( EH EH ER1 ) },
	{ Anything,		"er",		Anything,		PHONESEQ( ER2 ) },
	{ Nothing,		"evil",		Anything,		PHONESEQ( IY VV EH LL ) },
	{ Nothing,		"even",		Anything,		PHONESEQ( IY VV EH NN1 ) },
	{ "m",			"edia",		Anything,		PHONESEQ( IY DD2 IY AX ) },
	{ Anything,		"ecia",		Anything,		PHONESEQ( IY SH IY EY ) },
	{ ":",			"eleg",		Anything,		PHONESEQ( EH LL EH GG1 ) },

	{ "#:",			"e",		"w",			Silent },
	{ "t",			"ew",		Anything,		PHONESEQ( UW2 ) },
	{ "s",			"ew",		Anything,		PHONESEQ( UW2 ) },
	{ "r",			"ew",		Anything,		PHONESEQ( UW2 ) },
	{ "d",			"ew",		Anything,		PHONESEQ( UW2 ) },
	{ "l",			"ew",		Anything,		PHONESEQ( UW2 ) },
	{ "z",			"ew",		Anything,		PHONESEQ( UW2 ) },
	{ "n",			"ew",		Anything,		PHONESEQ( UW2 ) },
	{ "j",			"ew",		Anything,		PHONESEQ( UW2 ) },
	{ "th",			"ew",		Anything,		PHONESEQ( UW2 ) },
	{ "ch",			"ew",		Anything,		PHONESEQ( UW2 ) },
	{ "sh",			"ew",		Anything,		PHONESEQ( UW2 ) },
	{ Anything,		"ew",		Anything,		PHONESEQ( YY2 UW2 ) },
	{ Anything,		"e",		"o",			PHONESEQ( IY ) },
	{ "#:s",		"es",		Nothing,		PHONESEQ( IH ZZ ) },
	{ "#:c",		"es",		Nothing,		PHONESEQ( IH ZZ ) },
	{ "#:g",		"es",		Nothing,		PHONESEQ( IH ZZ ) },
	{ "#:z",		"es",		Nothing,		PHONESEQ( IH ZZ ) },
	{ "#:x",		"es",		Nothing,		PHONESEQ( IH ZZ ) },
	{ "#:j",		"es",		Nothing,		PHONESEQ( IH ZZ ) },
	{ "#:ch",		"es",		Nothing,		PHONESEQ( IH ZZ ) },
	{ "#:sh",		"es",		Nothing,		PHONESEQ( IH ZZ ) },
	{ "#:",			"e",		"s" Nothing,	Silent },
	{ "#:",			"ely",		Nothing,		PHONESEQ( LL IY ) },
	{ "#:",			"ement",	Anything,		PHONESEQ( PA1 MM EH NN1 TT2 ) },
	{ Anything,		"eful",		Anything,		PHONESEQ( PA1 FF UH LL ) },
	{ Anything,		"ee",		Anything,		PHONESEQ( IY ) },
	{ Anything,		"earn",		Anything,		PHONESEQ( ER2 NN1 ) },
	{ Nothing,		"ear",		"^",			PHONESEQ( ER2 ) },
	{ "k.",			"ead",		Anything,		PHONESEQ( IY DD2 ) },
	{ "^.",			"ead",		Anything,		PHONESEQ( EH DD2 ) },
	{ "d",			"ead",		Anything,		PHONESEQ( EH DD2 ) },
	{ Anything,		"ead",		Anything,		PHONESEQ( IY DD2 ) },
	{ "#:",			"ea",		Nothing,		PHONESEQ( IY AX ) },
	{ "#:",			"ea",		"s",			PHONESEQ( IY AX ) },
	{ Anything,		"ea",		"su",			PHONESEQ( EH ) },
	{ Anything,		"ea",		Anything,		PHONESEQ( IY ) },
	{ Anything,		"eigh",		Anything,		PHONESEQ( EY ) },
	{ "l",			"ei",		Anything,		PHONESEQ( IY ) },
	{ ".",			"ei",		Anything,		PHONESEQ( EY ) },
	{ Anything,		"ei",		"n",			PHONESEQ( AY ) },
	{ Anything,		"ei",		Anything,		PHONESEQ( IY ) },
	{ Anything,		"ey",		Anything,		PHONESEQ( IY ) },
	{ Anything,		"eu",		Anything,		PHONESEQ( YY2 UW2 ) },

	{ "#:",			"e",		"d" Nothing,	Silent },
	{ "#s",			"e",		"^",			Silent },
	{ ":",			"e",		"x",			PHONESEQ( EH EH ) },
	{ "#:",			"e",		Nothing,		Silent },
	{ "+:",			"e",		Nothing,		Silent },
	{ "':^",		"e",		Nothing,		Silent },
	{ ":",			"equ",		Anything,		PHONESEQ( IY KK1 WW ) },
	{ "dg",			"e",		Anything,		Silent },
	{ "dh",			"e",		Anything,		PHONESEQ( IY ) },
	{ Nothing ":",	"e",		Nothing,		PHONESEQ( IY ) },
	{ "#",			"ed",		Nothing,		PHONESEQ( DD1 ) },
	{ Anything,		"e",		Anything,		PHONESEQ( EH ) },
	{ NULL, NULL, NULL, { NULL, 0 } },	//sentinel
};

//6 - f
const TTSRule r_f[] = {
	{ Anything,		"fnord",	Anything,		PHONESEQ( FF NN1 AO OR DD1 ) },
	{ Anything,		"four",		Anything,		PHONESEQ( FF OW ER1 ) },
	{ Anything,		"ful",		Anything,		PHONESEQ( PA1 FF UH LL ) },
	{ Nothing,		"fly",		Anything,		PHONESEQ( FF LL AO AY ) },
	{ ".",			"fly",		Anything,		PHONESEQ( FF LL AO AY ) },
	{ Anything,		"fixed",	Anything,		PHONESEQ( FF IH KK1 SS TT2 ) },
	{ Anything,		"five",		Anything,		PHONESEQ( FF AO AY VV ) },
	{ Anything,		"foot",		Anything,		PHONESEQ( FF UH UH TT2 ) },
	{ Anything,		"f",		Anything,		PHONESEQ( FF ) },
	{ NULL, NULL, NULL, { NULL, 0 } },	//sentinel
};

//7 - g
const TTSRule r_g[] = {
	{ Anything,		"gadget",	Anything,		PHONESEQ( GG2 AE AE DD1 PA2 JH EH EH TT2 ) },
	{ Anything,		"god",		Anything,		PHONESEQ( GG3 AA AA DD1 ) },
	{ Anything,		"get",		Anything,		PHONESEQ( GG3 EH EH TT2 ) },
	{ Anything,		"gen",		"^",			PHONESEQ( JH EH EH NN1 ) },
	{ Anything,		"gen",		"#^",			PHONESEQ( JH EH EH NN1 ) },
	{ Anything,		"gen",		Nothing,		PHONESEQ( JH EH EH NN1 ) },
	{ Anything,		"giv",		Anything,		PHONESEQ( GG2 IH IH VV HH1 ) },
	{ "su",			"gges",		Anything,		PHONESEQ( GG1 JH EH SS ) },
	{ Anything,		"great",	Anything,		PHONESEQ( GG2 ER1 EY TT2 ) },
	{ Anything,		"good",		Anything,		PHONESEQ( GG2 UH UH DD1 ) },
	//hmmm guest guess
	{ Nothing,		"gue",		Anything,		PHONESEQ( GG2 EH ) },
	//hmm don't know about this one.  argue? vague?
	{ Anything,		"gue",		Anything,		PHONESEQ( GG3 ) },

	{ "d",			"g",		Anything,		PHONESEQ( JH ) },
	{ "##",			"g",		Anything,		PHONESEQ( GG1 ) },
	{ Anything,		"g",		"+",			PHONESEQ( JH ) },
	{ Anything,		"gg",		Anything,		PHONESEQ( GG3 PA1 ) },

	{ "campai",		"g",		"n",			Silent },
	{ "arrai",		"g",		"n",			Silent },
	{ "ali",		"g",		"n",			Silent },
	{ "beni",		"g",		"n",			Silent },
	{ "arrai",		"g",		"n",			Silent },

	{ Anything,		"g",		"a",			PHONESEQ( GG1 ) },
	{ Anything,		"g",		"e",			PHONESEQ( GG1 ) },
	{ Anything,		"g",		"i",			PHONESEQ( GG1 ) },
	{ Anything,		"g",		"y",			PHONESEQ( GG1 ) },

	{ Anything,		"g",		"o",			PHONESEQ( GG2 ) },
	{ Anything,		"g",		"u",			PHONESEQ( GG2 ) },
	{ Anything,		"g",		"l",			PHONESEQ( GG2 ) },
	{ Anything,		"g",		"r",			PHONESEQ( GG2 ) },


	{ Anything,		"g",		Nothing,		PHONESEQ( GG3 ) },
	{ "n",			"g",		Anything,		PHONESEQ( GG3 ) },
	{ Anything,		"g",		Anything,		PHONESEQ( GG3 ) },
	{ NULL, NULL, NULL, { NULL, 0 } },	//sentinel
};

//8 - h
const TTSRule r_h[] = {
	{ Anything,		"honor",	Anything,		PHONESEQ( AO NN1 ER2 ) },
	{ Anything,		"heard",	Anything,		PHONESEQ( HH1 ER2 DD1 ) },
	{ Anything,		"height",	Anything,		PHONESEQ( HH1 AY TT2 ) },
	{ Anything,		"honest",	Anything,		PHONESEQ( AO NN1 EH SS TT2 ) },
	{ Anything,		"hood",		Anything,		PHONESEQ( HH1 UH UH DD1 ) },
	{ "ab",			"hor",		Anything,		PHONESEQ( OW ER2 ) },
	{ Anything,		"heavy",	Anything,		PHONESEQ( HH1 AE VV IY ) },
	{ Anything,		"heart",	Anything,		PHONESEQ( HH1 AA ER1 TT2 ) },
	{ Anything,		"half",		Anything,		PHONESEQ( HH1 AE AE FF ) },
	{ Anything,		"hive",		Anything,		PHONESEQ( HH1 AA AY VV ) },
	{ Anything,		"heavi",	":#",			PHONESEQ( HH1 AE VV IY ) },
	{ Nothing,		"hav",		Anything,		PHONESEQ( HH1 AE VV HH1 ) },
	{ Anything,		"ha",		Nothing,		PHONESEQ( HH1 AA AA ) },
	{ Nothing,		"hi",		Nothing,		PHONESEQ( HH1 AA AY ) },
	{ Anything,		"he",		"t",			PHONESEQ( HH1 AE ) },
	{ Anything,		"he",		"x",			PHONESEQ( HH1 AE ) },
	{ Anything,		"hy",		Anything,		PHONESEQ( HH1 AA AY ) },
	{ Nothing,		"hang",		Anything,		PHONESEQ( HH1 AE NG ) },
	{ Nothing,		"here",		Anything,		PHONESEQ( HH1 IY XR ) },
	{ Nothing,		"hour",		Anything,		PHONESEQ( AW ER2 ) },
	{ Anything,		"how",		Anything,		PHONESEQ( HH1 AW ) },
	{ Anything,		"h",		"onor",			Silent },
	{ Anything,		"h",		"onest",		Silent },
	{ Anything,		"h",		"#",			PHONESEQ( HH1 ) },
	{ Anything,		"h",		Anything,		Silent },
	{ NULL, NULL, NULL, { NULL, 0 } },	//sentinel
};

//9 - i
const TTSRule r_i[] = {
	{ Nothing,		"i",		Nothing,		PHONESEQ( AO AY ) },
	{ Nothing,		"ii",		Nothing,		PHONESEQ( TT2 UW2 ) },
	{ Nothing,		"iii",		Nothing,		PHONESEQ( TH ER1 IY ) },

	{ Nothing,		"intrigu",	"#",			PHONESEQ( IH NN1 TT2 ER1 IY GG1 ) },
	{ Nothing,		"iso",		Anything,		PHONESEQ( AY SS OW ) },
	{ Anything,		"ity",		Nothing,		PHONESEQ( PA1 IH TT2 IY ) },
	{ Nothing,		"in",		Anything,		PHONESEQ( IH IH NN1 ) },
	{ Nothing,		"i",		"o",			PHONESEQ( AY ) },
	{ Anything,		"ify",		Anything,		PHONESEQ( PA1 IH FF AY ) },
	{ Anything,		"igh",		Anything,		PHONESEQ( AY ) },
	{ Anything,		"ild",		Anything,		PHONESEQ( AY LL DD1 ) },
	{ Anything,		"ign",		Nothing,		PHONESEQ( AY NN1 ) },
	{ Anything,		"in",		"d",			PHONESEQ( AY NN1 ) },
	{ Anything,		"ier",		Anything,		PHONESEQ( IY ER2 ) },
	{ Anything,		"idea",		Anything,		PHONESEQ( AY DD2 IY AX ) },
	{ Nothing,		"idl",		Anything,		PHONESEQ( AY DD2 AX LL ) },	//there was previously a 'YYY' at the end
	{ Anything,		"iron",		Anything,		PHONESEQ( AA AY ER2 NN1 ) },
	{ Anything,		"ible",		Anything,		PHONESEQ( IH BB1 LL ) },
	{ "r",			"iend",		Anything,		PHONESEQ( AE NN1 DD1 ) },
	{ Anything,		"iend",		Anything,		PHONESEQ( IY NN1 DD1 ) },
	{ "#:r",		"ied",		Anything,		PHONESEQ( IY DD1 ) },
	{ Anything,		"ied",		Nothing,		PHONESEQ( AY DD1 ) },
	{ Anything,		"ien",		Anything,		PHONESEQ( IY EH NN1 ) },
	{ Anything,		"ion",		Anything,		PHONESEQ( YY2 AX NN1 ) },
	{ "ch",			"ine",		Anything,		PHONESEQ( IY NN1 ) },
	{ "ent",		"ice",		Anything,		PHONESEQ( AY SS ) },
	{ Anything,		"ice",		Anything,		PHONESEQ( IH SS ) },
	{ Anything,		"iec",		"%",			PHONESEQ( IY SS SS ) },
	{ "#.",			"ies",		Nothing,		PHONESEQ( IY ZZ ) },
	{ Anything,		"ies",		Nothing,		PHONESEQ( AY ZZ ) },
	{ Anything,		"ie",		"t",			PHONESEQ( AY EH ) },
	{ Anything,		"ie",		"^",			PHONESEQ( IY ) },
	{ Anything,		"i",		"cation",		PHONESEQ( IH ) },

	{ Anything,		"ing",		Anything,		PHONESEQ( IH NG ) },
	{ Anything,		"ign",		"^",			PHONESEQ( AA AY NN1 ) },
	{ Anything,		"ign",		"%",			PHONESEQ( AA AY NN1 ) },
	{ Anything,		"ique",		Anything,		PHONESEQ( IY KK1 ) },
	{ Anything,		"ish",		Anything,		PHONESEQ( IH SH ) },


	{ Nothing,		"ir",		Anything,		PHONESEQ( YR ) },
	{ Anything,		"ir",		"#",			PHONESEQ( AA AY ER1 ) },
	{ Anything,		"ir",		Anything,		PHONESEQ( ER2 ) },
	{ Anything,		"iz",		"%",			PHONESEQ( AA AY ZZ ) },
	{ Anything,		"is",		"%",			PHONESEQ( AA AY ZZ ) },

	{ "^ch",		"i",		".",			PHONESEQ( AA AY ) },
	{ "^ch",		"i",		"^",			PHONESEQ( IH ) },
	{ Nothing "#^",	"i",		"^",			PHONESEQ( IH ) },
	{ "^#^",		"i",		"^",			PHONESEQ( IH ) },
	{ "^#^",		"i",		"#",			PHONESEQ( IY ) },
	{ ".",			"i",		Nothing,		PHONESEQ( AO AY ) },
	{ "#^",			"i",		"^#",			PHONESEQ( AY ) },
	{ Anything,		"i",		"gue",			PHONESEQ( IY ) },
	{ ".",			"i",		"ve",			PHONESEQ( AA AY ) },
	{ Anything,		"i",		"ve",			PHONESEQ( IH ) },
	{ Anything,		"i",		"^+:#",			PHONESEQ( IH ) },
	{ ".",			"i",		"o",			PHONESEQ( AO AY ) },
	{ "#^",			"i",		"^" Nothing,	PHONESEQ( IH ) },
	{ "#^",			"i",		"^#^",			PHONESEQ( IH ) },
	{ "#^",			"i",		"^",			PHONESEQ( IY ) },
	{ "^",			"i",		"^#",			PHONESEQ( AY ) },
	{ "^",			"i",		"o",			PHONESEQ( IY ) },
	{ ".",			"i",		"a",			PHONESEQ( AY ) },
	{ Anything,		"i",		"a",			PHONESEQ( IY ) },
	{ Nothing ":",	"i",		"%",			PHONESEQ( AY ) },
	{ Anything,		"i",		"%",			PHONESEQ( IY ) },
	{ ".",			"i",		".#",			PHONESEQ( AA AY ) },	// there was previously an 'XX' at the end
	{ Anything,		"i",		"d%",			PHONESEQ( AH AY ) },
	{ "+^",			"i",		"^+",			PHONESEQ( AH AY ) },
	{ Anything,		"i",		"t%",			PHONESEQ( AH AY ) },
	{ "#:^",		"i",		"^+",			PHONESEQ( AH AY ) },
	{ Anything,		"i",		"^+",			PHONESEQ( AH AY ) },
	{ ".",			"i",		".",			PHONESEQ( IH IH ) },
	{ Anything,		"i",		"nus",			PHONESEQ( AA AY ) },
	{ Anything,		"i",		Anything,		PHONESEQ( IH ) },
	{ NULL, NULL, NULL, { NULL, 0 } },	//sentinel
};

//10 - j
const TTSRule r_j[] = {
	{ Anything,		"japanese",	Anything,		PHONESEQ( JH AX PP AE AE NN1 IY SS SS ) },
	{ Anything,		"japan",	Anything,		PHONESEQ( JH AX PP AE AE NN1 ) },
	{ Anything,		"july",		Anything,		PHONESEQ( JH UW2 LL AE AY ) },
	{ Anything,		"jesus",	Anything,		PHONESEQ( JH IY ZZ AX SS ) },
	{ Anything,		"j",		Anything,		PHONESEQ( JH ) },
	{ NULL, NULL, NULL, { NULL, 0 } },	//sentinel
};

//11 - k
const TTSRule r_k[] = {
	{ Nothing,		"k",		"n",			Silent },

	{ Anything,		"k",		"u",			PHONESEQ( KK3 ) },
	{ Anything,		"k",		"o",			PHONESEQ( KK3 ) },
	{ Anything,		"k",		"a^^",			PHONESEQ( KK3 ) },
	{ Anything,		"k",		"o^^",			PHONESEQ( KK3 ) },
	{ Anything,		"k",		"l",			PHONESEQ( KK3 ) },
	{ Anything,		"k",		"r",			PHONESEQ( KK3 ) },

	{ Anything,		"k",		"a",			PHONESEQ( KK1 ) },
	{ Anything,		"k",		"e",			PHONESEQ( KK1 ) },
	{ Anything,		"k",		"i",			PHONESEQ( KK1 ) },

	{ Anything,		"k",		Nothing,		PHONESEQ( KK2 ) },
	{ Anything,		"k",		Anything,		PHONESEQ( KK1 ) },
	{ NULL, NULL, NULL, { NULL, 0 } },	//sentinel
};

//12 - l
const TTSRule r_l[] = {
	{ "l",			"l",		Anything,		Silent },
	{ Nothing,		"lion",		Anything,		PHONESEQ( LL AY AX NN1 ) },
	{ Anything,		"lead",		Anything,		PHONESEQ( LL IY DD1 ) },
	{ Anything,		"level",	Anything,		PHONESEQ( LL EH VV AX LL ) },
	{ Anything,		"liber",	Anything,		PHONESEQ( LL IH BB2 ER2 ) },
	{ Nothing,		"lose",		Anything,		PHONESEQ( LL UW2 ZZ ) },
	{ Nothing,		"liv",		Anything,		PHONESEQ( LL IH VV ) },
	{ "^",			"liv",		Anything,		PHONESEQ( LL AY VV ) },
	{ "#",			"liv",		Anything,		PHONESEQ( LL IH VV ) },
	{ Anything,		"liv",		Anything,		PHONESEQ( LL AY VV ) },
	{ Anything,		"lo",		"c#",			PHONESEQ( LL OW ) },
	{ "#:^",		"l",		"%",			PHONESEQ( LL ) },

	{ Anything,		"ly",		Nothing,		PHONESEQ( PA1 LL IY ) },
	{ Anything,		"l",		Anything,		PHONESEQ( LL ) },
	{ NULL, NULL, NULL, { NULL, 0 } },	//sentinel
};

//13 - m
const TTSRule r_m[] = {
	{ "m",			"m",		Anything,		Silent },
	{ Nothing,		"my",		Nothing,		PHONESEQ( MM AY ) },
	{ Nothing,		"mary",		Nothing,		PHONESEQ( MM EY XR IY ) },
	{ "#",			"mary",		Nothing,		PHONESEQ( PA1 MM EY XR IY ) },
	{ Anything,		"micro",	Anything,		PHONESEQ( MM AY KK1 ER1 OW ) },
	{ Anything,		"mono",		".",			PHONESEQ( MM AA NN1 OW ) },
	{ Anything,		"mono",		"^",			PHONESEQ( MM AA NN1 AA ) },
	{ Anything,		"mon",		"#",			PHONESEQ( MM AA AA NN1 ) },
	{ Anything,		"mos",		Anything,		PHONESEQ( MM OW SS ) },
	{ Anything,		"mov",		Anything,		PHONESEQ( MM UW2 VV HH1 ) },
	{ "th",			"m",		"#",			PHONESEQ( MM ) },
	{ "th",			"m",		Nothing,		PHONESEQ( IH MM ) },
	{ Anything,		"m",		Anything,		PHONESEQ( MM ) },
	{ NULL, NULL, NULL, { NULL, 0 } },	//sentinel
};

//14 - n
const TTSRule r_n[] = {
	{ "n",			"n",		Anything,		Silent },
	{ Nothing,		"now",		Nothing,		PHONESEQ( NN1 AW ) },
	{ "#",			"ng",		"+",			PHONESEQ( NN1 JH ) },
	{ Anything,		"ng",		"r",			PHONESEQ( NG GG1 ) },
	{ Anything,		"ng",		"#",			PHONESEQ( NG GG1 ) },
	{ Anything,		"ngl",		"%",			PHONESEQ( NG GG1 AX LL ) },
	{ Anything,		"ng",		Anything,		PHONESEQ( NG ) },
	{ Anything,		"nk",		Anything,		PHONESEQ( NG KK1 ) },
	{ Nothing,		"none",		Anything,		PHONESEQ( NN2 AH NN1 ) },
	{ Nothing,		"non",		":",			PHONESEQ( NN2 AA AA NN1 ) },
	{ Anything,		"nuc",		"l",			PHONESEQ( NN2 UW1 KK1 ) },

	{ "r",			"n",		Anything,		PHONESEQ( NN1 ) },
	{ Anything,		"n",		"#r",			PHONESEQ( NN1 ) },
	{ Anything,		"n",		"o",			PHONESEQ( NN2 ) },

	{ Anything,		"n",		Anything,		PHONESEQ( NN1 ) },
	{ NULL, NULL, NULL, { NULL, 0 } },	//sentinel
};

//15 - o
const TTSRule r_o[] = {
	{ Nothing,		"only",		Anything,		PHONESEQ( OW NN1 LL IY ) },
	{ Nothing,		"once",		Anything,		PHONESEQ( WW AH NN1 SS ) },
	{ Nothing,		"oh",		Nothing,		PHONESEQ( OW ) },
	{ Nothing,		"ok",		Nothing,		PHONESEQ( OW PA3 KK1 EH EY ) },
	{ Nothing,		"okay",		Nothing,		PHONESEQ( OW PA3 KK1 EH EY ) },
	{ Nothing,		"ohio",		Nothing,		PHONESEQ( OW HH1 AY OW ) },
	{ Nothing,		"over",		Anything,		PHONESEQ( OW VV ER2 ) },
	{ Anything,		"other",	Anything,		PHONESEQ( AH DH2 ER2 ) },
	{ Anything,		"ohm",		Nothing,		PHONESEQ( OW MM ) },

	{ Anything,		"origin",	Anything,		PHONESEQ( OR IH DD2 JH IH NN1 ) },
	{ Anything,		"orough",	Anything,		PHONESEQ( ER2 OW ) },
	{ Anything,		"ought",	Anything,		PHONESEQ( AO TT2 ) },
	{ Anything,		"occu",		"p",			PHONESEQ( AA KK1 PA1 UW1 ) },
	{ Anything,		"ough",		Anything,		PHONESEQ( AH FF ) },
	{ Anything,		"ore",		Anything,		PHONESEQ( OW ER1 ) },
	{ "#:",			"ors",		Nothing,		PHONESEQ( ER2 ZZ ) },
	{ Anything,		"orr",		Anything,		PHONESEQ( AO ER1 ) },
	{ "d",			"one",		Anything,		PHONESEQ( AH NN1 ) },
	{ "^y",			"one",		Anything,		PHONESEQ( WW AH NN1 ) },
	{ Nothing,		"one",		Anything,		PHONESEQ( WW AH NN1 ) },
	{ Anything,		"our",		Nothing,		PHONESEQ( AW ER1 ) },
	{ Anything,		"our",		"^",			PHONESEQ( OR ) },
	{ Anything,		"our",		Anything,		PHONESEQ( AO AW ER1 ) },
	{ "t",			"own",		Anything,		PHONESEQ( AW NN1 ) },
	{ "br",			"own",		Anything,		PHONESEQ( AW NN1 ) },
	{ "fr",			"own",		Anything,		PHONESEQ( AW NN1 ) },
	{ Anything,		"olo",		Anything,		PHONESEQ( AO AA LL AO ) },
	{ Anything,		"ould",		Anything,		PHONESEQ( UH DD1 ) },
	{ Anything,		"oup",		Anything,		PHONESEQ( UW2 PP ) },
	{ Anything,		"oing",		Anything,		PHONESEQ( OW IH NG ) },
	{ Anything,		"omb",		"%",			PHONESEQ( OW MM ) },
	{ Anything,		"oor",		Anything,		PHONESEQ( AO ER1 ) },
	{ Anything,		"ook",		Anything,		PHONESEQ( UH KK1 ) },
	{ Anything,		"on't",		Anything,		PHONESEQ( OW NN1 TT2 ) },
	{ Anything,		"oss",		Nothing,		PHONESEQ( AO SS ) },

	{ Anything,		"of",		Nothing,		PHONESEQ( AX AX VV HH1 ) },
	{ "^",			"or",		Nothing,		PHONESEQ( AO AO ER1 ) },
	{ "#:",			"or",		Nothing,		PHONESEQ( ER2 ) },
	{ Anything,		"or",		Anything,		PHONESEQ( AO AO ER1 ) },
	{ Anything,		"ow",		Nothing,		PHONESEQ( OW ) },
	{ Anything,		"ow",		"#",			PHONESEQ( OW ) },
	{ Anything,		"ow",		".",			PHONESEQ( OW ) },
	{ Anything,		"ow",		Anything,		PHONESEQ( AW ) },
	{ Nothing "l",	"ov",		Anything,		PHONESEQ( AH VV HH1 ) },
	{ Nothing "d",	"ov",		Anything,		PHONESEQ( AH VV HH1 ) },
	{ "gl",			"ov",		Anything,		PHONESEQ( AH VV HH1 ) },
	{ "^",			"ov",		Anything,		PHONESEQ( OW VV HH1 ) },
	{ Anything,		"ov",		Anything,		PHONESEQ( AH VV HH1 ) },
	{ Anything,		"ol",		"d",			PHONESEQ( OW LL ) },
	{ Nothing,		"ou",		Anything,		PHONESEQ( AW ) },
	{ "h",			"ou",		"s#",			PHONESEQ( AW ) },
	{ "ac",			"ou",		"s",			PHONESEQ( UW2 ) },
	{ "^",			"ou",		"^l",			PHONESEQ( AH ) },
	{ Anything,		"ou",		Anything,		PHONESEQ( AW ) },
	{ Anything,		"oa",		Anything,		PHONESEQ( OW ) },
	{ Anything,		"oy",		Anything,		PHONESEQ( OY ) },
	{ Anything,		"oi",		Anything,		PHONESEQ( OY ) },
	{ "i",			"on",		Anything,		PHONESEQ( AX AX NN1 ) },
	{ "#:",			"on",		Nothing,		PHONESEQ( AX AX NN1 ) },
	{ "#^",			"on",		Anything,		PHONESEQ( AX AX NN1 ) },
	{ Anything,		"of",		"^",			PHONESEQ( AO FF ) },
	{ "#:^",		"om",		Anything,		PHONESEQ( AH MM ) },
	{ Anything,		"oo",		Anything,		PHONESEQ( UW2 ) },

	{ Anything,		"ous",		Anything,		PHONESEQ( AX SS ) },

	{ "^#^",		"o",		"^",			PHONESEQ( AX ) },
	{ "^#^",		"o",		"#",			PHONESEQ( OW ) },
	{ "#",			"o",		".",			PHONESEQ( OW ) },
	{ "^",			"o",		"^#^",			PHONESEQ( AX AX ) },
	{ "^",			"o",		"^#",			PHONESEQ( OW ) },
	{ Anything,		"o",		"^%",			PHONESEQ( OW ) },
	{ Anything,		"o",		"^en",			PHONESEQ( OW ) },
	{ Anything,		"o",		"^i#",			PHONESEQ( OW ) },
	{ Anything,		"o",		"e",			PHONESEQ( OW ) },
	{ Anything,		"o",		Nothing,		PHONESEQ( OW ) },
	{ "c",			"o",		"n",			PHONESEQ( AA ) },
	{ Anything,		"o",		"ng",			PHONESEQ( AO ) },
	{ Nothing ":^",	"o",		"n",			PHONESEQ( AX ) },
	{ Anything,		"o",		"st" Nothing,	PHONESEQ( OW ) },
	{ Anything,		"o",		Anything,		PHONESEQ( AO ) },
	{ NULL, NULL, NULL, { NULL, 0 } },	//sentinel
};

//16 - p
const TTSRule r_p[] = {
	{ Nothing,		"pi",		Nothing,		PHONESEQ( PP AY ) },
	{ Anything,		"put",		Nothing,		PHONESEQ( PP UH TT2 ) },
	{ Anything,		"prove",	Anything,		PHONESEQ( PP ER1 UW2 VV ) },
	{ Anything,		"ply",		Anything,		PHONESEQ( PP LL AY ) },
	{ "p",			"p",		Anything,		Silent },
	{ Anything,		"phe",		Nothing,		PHONESEQ( FF IY ) },
	{ Anything,		"phe",		"s" Nothing,	PHONESEQ( FF IY ) },
	{ Anything,		"peop",		Anything,		PHONESEQ( PP IY PP ) },
	{ Anything,		"pow",		Anything,		PHONESEQ( PP AW ) },
	{ Anything,		"ph",		Anything,		PHONESEQ( FF ) },
	{ Anything,		"p",		Anything,		PHONESEQ( PP ) },
	{ NULL, NULL, NULL, { NULL, 0 } },	//sentinel
};

//17 - q
const TTSRule r_q[] = {
	{ Anything,		"quar",		Anything,		PHONESEQ( KK3 WW AO ER1 ) },
	{ Anything,		"que",		Nothing,		PHONESEQ( KK2 ) },
	{ Anything,		"que",		"s",			PHONESEQ( KK2 ) },
	{ Anything,		"qu",		Anything,		PHONESEQ( KK3 WW ) },
	{ Anything,		"q",		Anything,		PHONESEQ( KK1 ) },
	{ NULL, NULL, NULL, { NULL, 0 } },	//sentinel
};

//18 - r
const TTSRule r_r[] = {
	{ Nothing,		"rugged",	Anything,		PHONESEQ( ER1 AX GG1 PA1 EH DD1 ) },
	{ Nothing,		"russia",	Anything,		PHONESEQ( ER1 AX SH PA1 AX ) },
	{ Nothing,		"reality",	Anything,		PHONESEQ( ER1 IY AE LL IH TT2 IY ) },
	{ Anything,		"radio",	Anything,		PHONESEQ( ER1 EY DD2 IY OW ) },
	{ Anything,		"radic",	Anything,		PHONESEQ( ER1 AE DD2 IH KK1 ) },
	{ Nothing,		"re",		"^#",			PHONESEQ( ER1 IY ) },
	{ Nothing,		"re",		"^^#",			PHONESEQ( ER1 IY ) },
	{ Nothing,		"re",		"^^+",			PHONESEQ( ER1 IY ) },

	{ "^",			"r",		Anything,		PHONESEQ( RR2 ) },
	{ Anything,		"r",		Anything,		PHONESEQ( ER1 ) },
	{ NULL, NULL, NULL, { NULL, 0 } },	//sentinel
};

//19 - s
const TTSRule r_s[] = {
	{ Anything,		"said",		Anything,		PHONESEQ( SS EH DD1 ) },
	{ Anything,		"secret",	Anything,		PHONESEQ( SS IY KK1 ER1 EH TT2 ) },
	{ Nothing,		"sly",		Anything,		PHONESEQ( SS LL AY ) },
	{ Nothing,		"satur",	Anything,		PHONESEQ( SS AE AE TT2 ER2 ) },
	{ Anything,		"some",		Anything,		PHONESEQ( SS AH MM ) },
	{ Anything,		"s",		"hon#^",		PHONESEQ( SS ) },
	{ Anything,		"sh",		Anything,		PHONESEQ( SH ) },
	{ "#",			"sur",		"#",			PHONESEQ( ZH ER2 ) },
	{ Anything,		"sur",		"#",			PHONESEQ( SH ER2 ) },
	{ "#",			"su",		"#",			PHONESEQ( ZH UW2 ) },
	{ "#",			"ssu",		"#",			PHONESEQ( SH UW2 ) },
	{ "#",			"sed",		Nothing,		PHONESEQ( ZZ DD1 ) },
	{ "#",			"sion",		Anything,		PHONESEQ( PA1 ZH AX NN1 ) },
	{ "^",			"sion",		Anything,		PHONESEQ( PA1 SH AX NN1 ) },
	{ "s",			"sian",		Anything,		PHONESEQ( SS SS IY AX NN1 ) },
	{ "#",			"sian",		Anything,		PHONESEQ( PA1 ZH IY AX NN1 ) },
	{ Anything,		"sian",		Anything,		PHONESEQ( PA1 ZH AX NN1 ) },
	{ Nothing,		"sch",		Anything,		PHONESEQ( SS KK1 ) },
	{ "#",			"sm",		Anything,		PHONESEQ( ZZ MM ) },
	{ "#",			"sn",		"'",			PHONESEQ( ZZ AX NN1 ) },
	{ Nothing,		"sky",		Anything,		PHONESEQ( SS KK1 AY ) },
	{ "#",			"s",		"#",			PHONESEQ( ZZ ) },
	{ ".",			"s",		Nothing,		PHONESEQ( ZZ ) },
	{ "#:.e",		"s",		Nothing,		PHONESEQ( ZZ ) },
	{ "#:^##",		"s",		Nothing,		PHONESEQ( ZZ ) },
	{ "#:^#",		"s",		Nothing,		PHONESEQ( SS ) },
	{ "u",			"s",		Nothing,		PHONESEQ( SS ) },
	{ Nothing ":#",	"s",		Nothing,		PHONESEQ( ZZ ) },
	{ Anything,		"s",		"s",			Silent },
	{ Anything,		"s",		"c+",			Silent },
	{ Anything,		"s",		Anything,		PHONESEQ( SS ) },
	{ NULL, NULL, NULL, { NULL, 0 } },	//sentinel
};

//20 - t
const TTSRule r_t[] = {
	{ Nothing,		"the",		Nothing,		PHONESEQ( DH1 IY ) },
	{ Nothing,		"this",		Nothing,		PHONESEQ( DH2 IH IH SS SS ) },
	{ Nothing,		"than",		Nothing,		PHONESEQ( DH2 AE AE NN1 ) },
	{ Nothing,		"them",		Nothing,		PHONESEQ( DH2 EH EH MM ) },
	{ Nothing,		"tilde",	Nothing,		PHONESEQ( TT2 IH LL DD2 AX ) },
	{ Nothing,		"tuesday",	Nothing,		PHONESEQ( TT2 UW2 ZZ PA2 DD2 EY ) },

	{ Nothing,		"try",		Anything,		PHONESEQ( TT2 ER1 AY ) },
	{ Nothing,		"thy",		Anything,		PHONESEQ( DH2 AY ) },
	{ Nothing,		"they",		Anything,		PHONESEQ( DH2 EH EY ) },
	{ Nothing,		"there",	Anything,		PHONESEQ( DH2 EH XR ) },
	{ Nothing,		"then",		Anything,		PHONESEQ( DH2 EH EH NN1 ) },
	{ Nothing,		"thus",		Anything,		PHONESEQ( DH2 AH AH SS ) },
	{ Anything,		"that",		Nothing,		PHONESEQ( DH2 AE TT2 ) },

	{ Anything,		"truly",	Anything,		PHONESEQ( TT2 ER1 UW2 LL IY ) },
	{ Anything,		"truth",	Anything,		PHONESEQ( TT2 ER1 UW2 TH ) },
	{ Anything,		"their",	Anything,		PHONESEQ( DH2 EH IY XR ) },
	{ Anything,		"these",	Nothing,		PHONESEQ( DH2 IY ZZ ) },
	{ Anything,		"through",	Anything,		PHONESEQ( TH ER1 UW2 ) },
	{ Anything,		"those",	Anything,		PHONESEQ( DH2 OW ZZ ) },
	{ Anything,		"though",	Nothing,		PHONESEQ( DH2 OW ) },

	{ Anything,		"tion",		Anything,		PHONESEQ( PA1 SH AX NN1 ) },
	{ Anything,		"tian",		Anything,		PHONESEQ( PA1 SH AX NN1 ) },
	{ Anything,		"tien",		Anything,		PHONESEQ( SH AX NN1 ) },

	{ Anything,		"tear",		Nothing,		PHONESEQ( TT2 EY ER2 ) },
	{ Anything,		"tear",		"%",			PHONESEQ( TT2 EY ER2 ) },
	{ Anything,		"tear",		"#",			PHONESEQ( TT2 EY ER2 ) },

	{ "#",			"t",		"ia",			PHONESEQ( SH ) },
	{ ".",			"t",		"ia",			PHONESEQ( SH ) },

	{ Anything,		"ther",		Anything,		PHONESEQ( DH2 PA2 ER2 ) },
	{ Anything,		"to",		Nothing,		PHONESEQ( TT2 UW2 ) },
	{ "#",			"th",		Anything,		PHONESEQ( TH ) },
	{ Anything,		"th",		Anything,		PHONESEQ( TH ) },
	{ "#:",			"ted",		Nothing,		PHONESEQ( PA1 TT2 IH DD1 ) },
	{ Anything,		"tur",		"#",			PHONESEQ( PA1 CH ER2 ) },
	{ Anything,		"tur",		"^",			PHONESEQ( TT2 ER2 ) },
	{ Anything,		"tu",		"a",			PHONESEQ( CH UW2 ) },
	{ Nothing,		"two",		Anything,		PHONESEQ( TT2 UW2 ) },

	{ "t",			"t",		Anything,		Silent },

	{ Anything,		"t",		"s",			PHONESEQ( TT1 ) },
	{ Anything,		"t",		Anything,		PHONESEQ( TT2 ) },
	{ NULL, NULL, NULL, { NULL, 0 } },	//sentinel
};

//21 - u
const TTSRule r_u[] = {
	{ Nothing,		"un",		Nothing,		PHONESEQ( YY2 UW2 PA3 AE NN1 ) },
	{ Nothing,		"usa",		Nothing,		PHONESEQ( YY2 UW2 PA3 AE SS SS PA3 EH EY ) },
	{ Nothing,		"ussr",		Nothing,		PHONESEQ( YY2 UW2 PA3 AE SS SS PA3 AE SS SS PA3 AA AR ) },

	{ Nothing,		"u",		Nothing,		PHONESEQ( YY2 UW1 ) },
	{ Nothing,		"un",		"i",			PHONESEQ( YY2 UW2 NN1 ) },
	{ Nothing,		"un",		":",			PHONESEQ( AH NN1 PA1 ) },
	{ Nothing,		"un",		Anything,		PHONESEQ( AH NN1 ) },
	{ Nothing,		"upon",		Anything,		PHONESEQ( AX PP AO NN1 ) },
	{ "d",			"up",		Anything,		PHONESEQ( UW2 PP ) },
	//{ Anything,		"use",		".",	PHONESEQ( UW1 ZZ ) },
	{ "t",			"ur",		"#",			PHONESEQ( UH ER1 ) },
	{ "s",			"ur",		"#",			PHONESEQ( UH ER1 ) },
	{ "r",			"ur",		"#",			PHONESEQ( UH ER1 ) },
	{ "d",			"ur",		"#",			PHONESEQ( UH ER1 ) },
	{ "l",			"ur",		"#",			PHONESEQ( UH ER1 ) },
	{ "z",			"ur",		"#",			PHONESEQ( UH ER1 ) },
	{ "n",			"ur",		"#",			PHONESEQ( UH ER1 ) },
	{ "j",			"ur",		"#",			PHONESEQ( UH ER1 ) },
	{ "th",			"ur",		"#",			PHONESEQ( UH ER1 ) },
	{ "ch",			"ur",		"#",			PHONESEQ( UH ER1 ) },
	{ "sh",			"ur",		"#",			PHONESEQ( UH ER1 ) },
	{ "arg",		"u",		"#",			PHONESEQ( YY2 UW2 ) },
	{ Anything,		"ur",		"#",			PHONESEQ( YY2 UH ER1 ) },
	{ Anything,		"ur",		Anything,		PHONESEQ( ER2 ) },
	{ Anything,		"uy",		Anything,		PHONESEQ( AA AY ) },

	{ Anything,		"u",		"^#^",			PHONESEQ( YY2 UW2 ) },
	{ Anything,		"u",		"^" Nothing,	PHONESEQ( AH ) },
	{ Anything,		"u",		"%",			PHONESEQ( UW2 ) },
	{ Nothing "g",	"u",		"#",			Silent },
	{ "g",			"u",		"%",			Silent },
	{ "g",			"u",		"#",			PHONESEQ( WW ) },
	{ "#n",			"u",		Anything,		PHONESEQ( YY2 UW2 ) },
	{ "#m",			"u",		Anything,		PHONESEQ( YY2 UW2 ) },
	{ "f",			"u",		"^^",			PHONESEQ( UH ) },
	{ "b",			"u",		"^^",			PHONESEQ( UH ) },
	{ "^",			"u",		"^e",			PHONESEQ( YY2 UW2 ) },
	{ "^",			"u",		"^",			PHONESEQ( AX ) },
	{ Anything,		"u",		"^^",			PHONESEQ( AH ) },
	{ "t",			"u",		Anything,		PHONESEQ( UW2 ) },
	{ "s",			"u",		Anything,		PHONESEQ( UW2 ) },
	{ "r",			"u",		Anything,		PHONESEQ( UW2 ) },
	{ "d",			"u",		Anything,		PHONESEQ( UW2 ) },
	{ "l",			"u",		Anything,		PHONESEQ( UW2 ) },
	{ "z",			"u",		Anything,		PHONESEQ( UW2 ) },
	{ "n",			"u",		Anything,		PHONESEQ( UW2 ) },
	{ "j",			"u",		Anything,		PHONESEQ( UW2 ) },
	{ "th",			"u",		Anything,		PHONESEQ( UW2 ) },
	{ "ch",			"u",		Anything,		PHONESEQ( UW2 ) },
	{ "sh",			"u",		Anything,		PHONESEQ( UW2 ) },
	{ Anything,		"u",		Anything,		PHONESEQ( YY2 UW2 ) },
	{ NULL, NULL, NULL, { NULL, 0 } },	//sentinel
};

//22 - v
const TTSRule r_v[] = {
	{ Anything,		"view",		Anything,		PHONESEQ( VV YY2 UW2 ) },
	{ Nothing,		"very",		Nothing,		PHONESEQ( VV EH ER2 PA1 IY ) },
	{ Anything,		"vary",		Anything,		PHONESEQ( VV EY PA1 ER1 IY ) },
	{ Anything,		"v",		Anything,		PHONESEQ( VV ) },
	{ NULL, NULL, NULL, { NULL, 0 } },	//sentinel
};

//23 - w
const TTSRule r_w[] = {
	{ Nothing,		"were",		Anything,		PHONESEQ( WW ER2 ) },
	{ Anything,		"weigh",	Anything,		PHONESEQ( WW EH EY ) },
	{ Anything,		"wood",		Anything,		PHONESEQ( WW UH UH DD1 ) },
	{ Anything,		"wary",		Anything,		PHONESEQ( WW EH ER2 PA1 IY ) },
	{ Anything,		"where",	Anything,		PHONESEQ( WW EH ER1 ) },
	{ Anything,		"what",		Anything,		PHONESEQ( WW AA AA TT2 ) },
	{ Anything,		"want",		Anything,		PHONESEQ( WW AA AA NN1 TT2 ) },
	{ Anything,		"whol",		Anything,		PHONESEQ( HH1 OW LL ) },
	{ Anything,		"who",		Anything,		PHONESEQ( HH1 UW2 ) },
	{ Anything,		"why",		Anything,		PHONESEQ( WW AO AY ) },
	{ Anything,		"wear",		Anything,		PHONESEQ( WW EY ER2 ) },
	{ Anything,		"wea",		"th",			PHONESEQ( WW EH ) },
	{ Anything,		"wea",		"l",			PHONESEQ( WW EH ) },
	{ Anything,		"wea",		"p",			PHONESEQ( WW EH ) },
	{ Anything,		"wa",		"s",			PHONESEQ( WW AA ) },
	{ Anything,		"wa",		"t",			PHONESEQ( WW AA ) },
	{ Anything,		"wh",		Anything,		PHONESEQ( WH ) },
	{ Anything,		"war",		Nothing,		PHONESEQ( WW AO ER1 ) },
	{ Nothing,		"wicked",	Anything,		PHONESEQ( WW IH KK2 PA1 EH DD1 ) },
	{ "be",			"wilder",	Anything,		PHONESEQ( WW IH LL DD2 ER2 ) },
	{ Nothing,		"wilder",	"ness",			PHONESEQ( WW IH LL DD2 ER2 ) },
	{ Nothing,		"wild",		"erness",		PHONESEQ( WW IH LL DD2 ) },
	{ Nothing,		"wily",		Nothing,		PHONESEQ( WW AY LL IY ) },
	{ Anything,		"wor",		"^",			PHONESEQ( WW ER2 ) },
	{ Anything,		"wr",		Anything,		PHONESEQ( ER1 ) },

	{ Anything,		"w",		Anything,		PHONESEQ( WW ) },
	{ NULL, NULL, NULL, { NULL, 0 } },	//sentinel
};

//24 - x
const TTSRule r_x[] = {
	{ Anything,		"x",		Anything,		PHONESEQ( KK1 SS ) },
	{ NULL, NULL, NULL, { NULL, 0 } },	//sentinel
};

//25 - y
const TTSRule r_y[] = {
	{ Anything,		"young",	Anything,		PHONESEQ( YY2 AH NG ) },
	{ Nothing,		"your",		Anything,		PHONESEQ( YY2 UH ER2 ) },
	{ Nothing,		"you",		Anything,		PHONESEQ( YY2 UW2 ) },
	{ Nothing,		"yes",		Anything,		PHONESEQ( YY2 EH SS ) },
	{ Anything,		"yte",		Anything,		PHONESEQ( AY TT2 PA1 ) },

	{ Anything,		"y",		Nothing,		PHONESEQ( IY ) },
	{ Anything,		"y",		Anything,		PHONESEQ( IH ) },
	//{ Nothing,		"y",		Anything,		PHONESEQ( YY2 ) },
	//{ "ph",			"y",		Anything,		PHONESEQ( IH ) },
	//{ ":s",			"y",		".",			PHONESEQ( IH ) },
	//{ "#^",			"y",		".",			PHONESEQ( AY ) },
	//{ "h",			"y",		"^",			PHONESEQ( AY ) },
	//{ "#",			"y",		"#",			PHONESEQ( OY ) },
	//{ "^",			"y",		"z",			PHONESEQ( AY ) },
	//{ "#:^",		"y",		Nothing,		PHONESEQ( IY ) },
	//{ "#:^",		"y",		"i",			PHONESEQ( IY ) },
	//{ Anything ":",			"y",		Nothing,		PHONESEQ( AY ) },
	//{ Anything ":",			"y",		"#",			PHONESEQ( AY ) },
	//{ Anything ":",			"y",		".",			PHONESEQ( AY ) },
	//{ Anything ":",			"y",		"^+:#",			PHONESEQ( IH ) },
	//{ Anything ":",			"y",		"^#",			PHONESEQ( AY ) },
	{ Anything,		"y",		Anything,		PHONESEQ( IH ) },
	{ NULL, NULL, NULL, { NULL, 0 } },	//sentinel
};

//26 - z
const TTSRule r_z[] = {
	{ "z",			"z",		Anything,		Silent },
	{ Anything,		"z",		Anything,		PHONESEQ( ZZ ) },
	{ NULL, NULL, NULL, { NULL, 0 } },	//sentinel
};



const TTSRule* _rules[27] = {
	r_punc,
	r_a, r_b, r_c, r_d, r_e, r_f, r_g, r_h,
	r_i, r_j, r_k, r_l, r_m, r_n, r_o, r_p,
	r_q, r_r, r_s, r_t, r_u, r_v, r_w, r_x,
	r_y, r_z,
};




