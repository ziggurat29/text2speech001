// text2speech001.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <iomanip>

#include "text_to_speech.h"
#include "make_compact_ruleset.h"
#include "tts_rules.h"

#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include <set>

typedef std::vector<uint8_t>	VEC_BYTE;
typedef std::set<std::string>	SET_STR;
typedef std::set<VEC_BYTE>	SET_BLOB;



void analyze()
{
	SET_STR strs;
	SET_BLOB bins;

	size_t nRules = 0;
	size_t nStrs = 0;
	size_t nStrsLen = 0;
	size_t nStrLongest = 0;
	size_t nBins = 0;
	size_t nBinsLen = 0;
	size_t nBinLongest = 0;

	//whizz through all the rules
	for (size_t nIdxRuleSec = 0; nIdxRuleSec < 27; ++nIdxRuleSec)
	{
		size_t nIdxRule = 0;
		const TTSRule* pRule = _rules[nIdxRuleSec];	//this group of rules; length unknown
		while (NULL != pRule->_bracket)	//not at sentinel
		{
			//inc raw counts
			nRules += 1;
			nStrs += 3;
			nBins += 1;

			//accum raw lens
			nStrsLen += strlen(pRule->_left) + 1;
			if (nStrLongest < strlen(pRule->_left))
				nStrLongest = strlen(pRule->_left);
			nStrsLen += strlen(pRule->_bracket) + 1;
			if (nStrLongest < strlen(pRule->_bracket))
				nStrLongest = strlen(pRule->_bracket);
			nStrsLen += strlen(pRule->_right) + 1;
			if (nStrLongest < strlen(pRule->_right))
				nStrLongest = strlen(pRule->_right);
			nBinsLen += pRule->_phone._len + 1;
			if (nBinLongest < pRule->_phone._len)
				nBinLongest = pRule->_phone._len;

			std::string strLeft(pRule->_left);
			std::string strBracket(pRule->_bracket);
			std::string strRight(pRule->_right);

			//YYY little peek at spaces while converting to a metacharacter
			/*
			size_t nLoc;
			if (std::string::npos != (nLoc = strLeft.find(' ', 0))&&1 != strLeft.length())
			{
				std::cout << "  a rule (" << nIdxRuleSec << "," << nIdxRule << ") left '" << strLeft << "'" << std::endl;
			}
			if (std::string::npos != (nLoc = strBracket.find(' ', 0)))
			{
				std::cout << "  a rule (" << nIdxRuleSec << "," << nIdxRule << ") bracket '" << strBracket << "'" << std::endl;
			}
			if (std::string::npos != (nLoc = strRight.find(' ', 1)))
			{
				std::cout << "  a rule (" << nIdxRuleSec << "," << nIdxRule << ") right '" << strRight << "'" << std::endl;
			}
			*/

			//stick them in the sets to de-dupe
			strs.insert(strLeft);
			strs.insert(strBracket);
			strs.insert(strRight);
			bins.insert(VEC_BYTE((const uint8_t*)pRule->_phone._phone,
					(const uint8_t*)pRule->_phone._phone+pRule->_phone._len));

			++pRule;	//next rule in group
			++nIdxRule;
		}
	}

	//gather de-duped stats
	size_t nDStrsLen = 0;
	for ( const std::string& str : strs)
	{
		nDStrsLen += str.length() + 1;
	}
	size_t nDBinsLen = 0;
	for (const VEC_BYTE& bin : bins)
	{
		nDBinsLen += bin.size() + 1;
	}

	//spew simple statistics
	std::cout << "Rules: " << nRules << std::endl <<
		"strs: " << nStrs << ", bins: " << nBins << std::endl <<
		"dstrs: " << strs.size() << ", dbins: " << bins.size() << std::endl <<
		"strlen: " << nStrsLen << ", binlen: " << nBinsLen << std::endl <<
		"dstrlen: " << nDStrsLen << ", dbinlen: " << nDBinsLen << std::endl <<
		"strlongest: " << nStrLongest << ", binlongest: " << nBinLongest << std::endl;
}

/* results:
Rules: 706
strs: 2118, bins: 706
dstrs: 484, dbins: 400
strlen: 4901, binlen: 2394
dstrlen: 2033, dbinlen: 1646
strlongest: 8, binlongest: 13
*/






int main()
{
	std::cout << "Hello World!\n";
	analyze();

	VEC_BYTE abyBlob;
	make_compact_ruleset ( abyBlob );
	//std::cout << "bloblen: " << abyBlob.size() << std::endl;

/**/
	//now, whizz through the blob, and emit it as a C file

	//for the .h
	std::cout << "#ifndef __TTS_RULES_COMPACT_H" << std::endl;
	std::cout << "#define __TTS_RULES_COMPACT_H" << std::endl << std::endl;
	std::cout << "#ifdef __cplusplus" << std::endl;
	std::cout << "extern \"C\" {" << std::endl;
	std::cout << "#endif" << std::endl << std::endl;
	std::cout << "#include <stdint.h>" << std::endl << std::endl;
	std::cout << "extern const uint8_t g_abyTTS[" << abyBlob.size() << "];" << std::endl << std::endl;
	std::cout << "#ifdef __cplusplus" << std::endl;
	std::cout << "}" << std::endl;
	std::cout << "#endif" << std::endl << std::endl;
	std::cout << "#endif" << std::endl;

	//for the .c
	std::cout << "#include \"tts_rules_compact.h\"" << std::endl;
	std::cout << "const uint8_t g_abyTTS[" << abyBlob.size() << "] = {" << std::endl;
	std::cout << std::hex << std::setfill('0');
	for (size_t nIdxBlob = 0; nIdxBlob < abyBlob.size(); )
	{
		std::cout << "/*" << std::setw(4) << nIdxBlob << "*/  ";
		//16 bytes per line
		for (size_t nIter = 0; nIter < 16 && nIdxBlob < abyBlob.size(); ++nIter )
		{
			std::cout << "0x" << std::setw(2) << (unsigned)abyBlob[nIdxBlob] << ", ";
			++nIdxBlob;
		}
		std::cout << std::endl;
	}
	std::cout << std::dec;	//return to dec
	std::cout << "};" << std::endl;
/**/


/** /
	const char* pchWordStart, * pchWordEnd;
	int eCvt;
	eCvt = pluckWord(NULL, -1, &pchWordStart, &pchWordEnd);
	eCvt = pluckWord("", -1, &pchWordStart, &pchWordEnd);
	eCvt = pluckWord("boo", -1, &pchWordStart, &pchWordEnd);
	eCvt = pluckWord("boo ", -1, &pchWordStart, &pchWordEnd);
	eCvt = pluckWord(" boo", -1, &pchWordStart, &pchWordEnd);
	eCvt = pluckWord(" boo ", -1, &pchWordStart, &pchWordEnd);
	eCvt = pluckWord(" bap'bap/ba,loo:bap;ba!lop-bam.boo? ", -1, &pchWordStart, &pchWordEnd);


static const char achGettysburg[] = 
R"(Four score and seven years ago our fathers brought forth on this continent 
a new nation, conceived in liberty, and dedicated to the proposition that all 
men are created equal.
Now we are engaged in a great civil war, testing whether that nation, or any 
nation so conceived and so dedicated, can long endure. We are met on a great 
battlefield of that war. We have come to dedicate a portion of that field, as 
a final resting place for those who here gave their lives that that nation 
might live. It is altogether fitting and proper that we should do this.
But, in a larger sense, we can not dedicate, we can not consecrate, we can not 
hallow this ground. The brave men, living and dead, who struggled here, have 
consecrated it, far above our poor power to add or detract. The world will 
little note, nor long remember what we say here, but it can never forget what 
they did here. It is for us the living, rather, to be dedicated here to the 
unfinished work which they who fought here have thus far so nobly advanced. It 
is rather for us to be here dedicated to the great task remaining before us -- 
that from these honored dead we take increased devotion to that cause for 
which they gave the last full measure of devotion -- that we here highly 
resolve that these dead shall not have died in vain -- that this nation, under 
God, shall have a new birth of freedom -- and that government of the people, 
by the people, for the people, shall not perish from the earth.)";

	//quicky test running through text
	const char* pszText = achGettysburg;
	int nTextLen = _countof(achGettysburg);
	while ( 0 == ( eCvt = pluckWord ( pszText, nTextLen, 
			&pchWordStart, &pchWordEnd ) ) )
	{
		int nWordLen = pchWordEnd - pchWordStart;
		std::cout << std::string(pchWordStart, nWordLen) << std::endl;
		//advance
		nTextLen -= pchWordEnd - pszText;
		pszText = pchWordEnd;
	}
/ **/

/** /
//there are 27 sections; 0-26
	for (int nSec = 0; nSec < 27; ++nSec)
	{
		int nSecLen = _getRuleSectionLength(abyBlob.data(), nSec);
		std::cout << "Rule section " << nSec << " has " << nSecLen << " rules" << std::endl;
		TTSRule_compact rule;
		for (int nRule = 0; nRule < nSecLen; ++nRule)
		{
			_reconstitute_rule(abyBlob.data(), nSec, nRule, &rule);
			std::cout << "  Rule " << nRule <<
				" l: '" << std::string((const char* const) &rule._left[1], (size_t)rule._left[0]) <<
				"', b: '" << std::string((const char* const)&rule._bracket[1], (size_t)rule._bracket[0]) <<
				"', r: '" << std::string((const char* const)&rule._right[1], (size_t)rule._right[0]) <<
				"', p: '";
			std::cout << std::hex << std::setfill('0');
			for (int nIdxPhone = 1; nIdxPhone <= (size_t)rule._phone[0]; ++nIdxPhone)
			{
				if (nIdxPhone > 1)
					std::cout << " ";
				std::cout << std::setw(2) << (unsigned)rule._phone[nIdxPhone];
			}
			std::cout << "'";
			std::cout << std::dec;	//return to dec
			std::cout << std::endl;
		}
	}
/ **/

/** /
	VEC_BYTE abyPhon(256);
	int nRet;
	//nRet = ttsWord("a", -1, abyBlob.data(), abyPhon.data(), abyPhon.size());
	//nRet = ttsWord("above", -1, abyBlob.data(), abyPhon.data(), abyPhon.size());
	nRet = ttsWord("fagainf", -1, abyBlob.data(), abyPhon.data(), abyPhon.size());
	//nRet = ttsWord(" bany", -1, abyBlob.data(), abyPhon.data(), abyPhon.size());

	//nRet = ttsWord("a", -1, abyBlob.data(), NULL, 0);

	//nRet = ttsWord("ussr", -1, abyBlob.data(), abyPhon.data(), 8);
/ **/

	return 0;
}
