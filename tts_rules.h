//These are the definitions of just the rules.  We have several ways of using
//them, including directly, for systems which do not have memory constraints
//and for testing, and compactly, for systems such as embedded that are
//constrained on how much data they can store.  The compact representation is
//functionally equivalent but inscrutable to the eye.


#ifndef __TTS_RULES_H
#define __TTS_RULES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

//special strings in the 'context'
#define Anything ""
#define Nothing "$"



typedef struct PhonSeq {
	const char*	_phone;
	size_t	_len;
} PhonSeq;


typedef struct TTSRule {
	const char*	_left;
	const char*	_bracket;
	const char*	_right;
	PhonSeq	_phone;
} TTSRule;



//rules are segregated by leading latin letter.  first group is 'punctuation'.
//each group (variable length array) of rules is terminated in a sentinel rule
//	{ NULL, NULL, NULL, { NULL, 0 } }
extern const TTSRule* _rules[27];



#ifdef __cplusplus
}
#endif

#endif
