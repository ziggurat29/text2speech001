#include "make_compact_ruleset.h"
#include "tts_rules.h"

#include <string>
#include <map>
#include <set>
#include <algorithm>


typedef std::set<std::string>	SET_STR;
typedef std::set<VEC_BYTE>	SET_BLOB;
typedef std::map<std::string,size_t>	MAP_STR_OFFSET;
typedef std::map<VEC_BYTE, size_t>	MAP_BLOB_OFFSET;



//whizz through all the rules and collect deduped data
void makeDeDups(SET_STR& strs, SET_BLOB& bins)
{
	//whizz through all the rules
	for (size_t nIdx = 0; nIdx < 27; ++nIdx)
	{
		const TTSRule* pRule = _rules[nIdx];	//this group of rules; length unknown
		while (NULL != pRule->_bracket)	//not at sentinel
		{
			//stick them in the sets to de-dupe
			strs.insert(pRule->_left);
			strs.insert(pRule->_bracket);
			strs.insert(pRule->_right);
			VEC_BYTE abyPhon((const uint8_t*)pRule->_phone._phone,
				(const uint8_t*)pRule->_phone._phone + pRule->_phone._len);
			//the rules are defined such that the phoneme values are all +1;
			//this is a hack so that C string merging can be exploited to
			//simplify declaring the rules in source code.  However, for our
			//compact rules, we must reverse this transformation.
			std::transform(abyPhon.begin(), abyPhon.end(), abyPhon.begin(),
					[](uint8_t by) { return by - 1; });
			bins.insert(abyPhon);

			++pRule;	//next rule in group
		}
	}
}



//make string blob
//here, we take the set of strings and concatenate them as length-prefixed
//instead of nul-term.  we make a separate index of string to offset to be
//used during rule encoding.
void makeStringBlob(VEC_BYTE& abyBlob, SET_STR& strs, MAP_STR_OFFSET& strsidx )
{
	for (const std::string& str : strs)
	{
		size_t nIdx = abyBlob.size();	//index where we are appending
		abyBlob.push_back((uint8_t)str.length());	//length prefix
		//append ASCII
		abyBlob.insert(abyBlob.end(),
				(uint8_t*)str.c_str(),
				(uint8_t*)str.c_str() + str.length());
		//add to index
		strsidx.insert(MAP_STR_OFFSET::value_type(str, nIdx));
	}
}


//make (or append) phoneme blob
//same thing for the phoneme blobs.
void makePhonemeBlob(VEC_BYTE& abyBlob, SET_BLOB& bins, MAP_BLOB_OFFSET& binsidx)
{
	for (const VEC_BYTE& bin : bins)
	{
		size_t nIdx = abyBlob.size();	//index where we are appending
		abyBlob.push_back((uint8_t)bin.size());	//length prefix
		//append bin
		if (0 != bin.size())	//avoid crash on deref
		{
			abyBlob.insert(abyBlob.end(),
				&bin[0],
				&bin[0] + bin.size());
		}
		//add to index
		binsidx.insert(MAP_BLOB_OFFSET::value_type(bin, nIdx));
	}
}



//The ruleset blob will precede the data blob.  These will consist of 16-bit
//values.
//The first section will be an array of indices to each rule group.  There will
//actually be 28 entries (instead of 27, the number of groups) because this
//will simplify calculating the length of the rule group based on index --
//particularly the last one (len = (idxnext - idxthis) / sizeof(rule).
//Each rule will be 4 16-bit indices into the data blob for the deduped data
//values.  The data values (already computed) will be 8-bit length-prefixed
//byte sequences -- ascii for the strings and binary for the phoneme sequences.
void makeRulesetBlob(VEC_BYTE& abyBlob, 
		VEC_BYTE& abyDataBlob,
		MAP_STR_OFFSET& strsidx,
		MAP_BLOB_OFFSET& binsidx)
{
	//XXX could reserve ((27+1) + 706*4)*sizeof(uint16_t) + abyDataBlob.size();

	//first, we know that the rule group index will be 27+1 entries, so just
	//set that up now, so we can directly index into it.
	abyBlob.resize((27 + 1) * sizeof(uint16_t));
	uint16_t nIdxRuleOffset = (uint16_t)abyBlob.size();

	for (size_t nIdxGroup = 0; nIdxGroup < 27; ++nIdxGroup)
	{
		//setup the index to this rule group's start
		uint16_t* pidxgroup = (uint16_t*) &abyBlob[nIdxGroup * sizeof(uint16_t)];
		*pidxgroup = (uint16_t)abyBlob.size();
		const TTSRule* pRule = _rules[nIdxGroup];	//this group of rules; length unknown
		while (NULL != pRule->_bracket)	//not at sentinel
		{
			//four 16-bit values:  indices into data blob for
			//left, bracket, right, phoneme data.
			//these will be relative to the start of the data blob, and we don't know
			//where that is yet, so we will do a second pass through all the rules
			//and fix these values up later.
			uint16_t nIdxLeft = (uint16_t)strsidx[pRule->_left];
			uint16_t nIdxBracket = (uint16_t)strsidx[pRule->_bracket];
			uint16_t nIdxRight = (uint16_t)strsidx[pRule->_right];
			VEC_BYTE phone((const uint8_t*)pRule->_phone._phone,
				(const uint8_t*)pRule->_phone._phone + pRule->_phone._len);
			//once again, untransform the phoneme data
			std::transform(phone.begin(), phone.end(), phone.begin(),
					[](uint8_t by) { return by - 1; });
			uint16_t nIdxPhoneme = (uint16_t)binsidx[phone];
			//now moosh them on
			abyBlob.insert(abyBlob.end(), (uint8_t*)&nIdxLeft, (uint8_t*)&nIdxLeft + sizeof(nIdxLeft));
			abyBlob.insert(abyBlob.end(), (uint8_t*)&nIdxBracket, (uint8_t*)&nIdxBracket + sizeof(nIdxBracket));
			abyBlob.insert(abyBlob.end(), (uint8_t*)&nIdxRight, (uint8_t*)&nIdxRight + sizeof(nIdxRight));
			abyBlob.insert(abyBlob.end(), (uint8_t*)&nIdxPhoneme, (uint8_t*)&nIdxPhoneme + sizeof(nIdxPhoneme));

			++pRule;	//next rule in group
		}
	}
	//set the pseudo-index to the last group, which also happens to be the
	//offset to the start of the data blob.
	uint16_t nIdxDataOffset = (uint16_t)abyBlob.size();
	*(uint16_t*)&abyBlob[27 * sizeof(uint16_t)] = nIdxDataOffset;

	//now append the data blob
	abyBlob.insert(abyBlob.end(), abyDataBlob.begin(), abyDataBlob.end());

	//now, whizz through all the rules, fixing up the indices by adding the
	//offset to the data start
	for (size_t nIdxRule = nIdxRuleOffset; nIdxRule < nIdxDataOffset; )
	{
		//for each of the 4 values, add on the offset to the data blob
		for (size_t nIterVal = 0; nIterVal < 4; ++nIterVal)
		{
			uint16_t* val = (uint16_t*)&abyBlob[nIdxRule];
			*val += nIdxDataOffset;
			nIdxRule += sizeof(uint16_t);	//next value
		}
	}
}



//do the whole thing
void make_compact_ruleset ( VEC_BYTE& abyBlob )
{
	//make deduped data sets
	SET_STR strs;
	SET_BLOB bins;
	makeDeDups(strs, bins);

	//make indexed data blob of deduped data
	VEC_BYTE abyDataBlob;
	MAP_STR_OFFSET strsidx;
	makeStringBlob(abyDataBlob, strs, strsidx);
	MAP_BLOB_OFFSET binsidx;
	makePhonemeBlob(abyDataBlob, bins, binsidx);

	//std::cout << "dstrs: " << strs.size() << ", dbins: " << bins.size() << 
	//		", blobsize: " << abyBlob.size() << std::endl;

	//now, make list-of-rulegroups-lengths, and list-of-all-rules
	makeRulesetBlob(abyBlob, abyDataBlob, strsidx, binsidx);
}
