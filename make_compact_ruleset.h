#ifndef __MAKE_COMPACT_RULESET_H
#define __MAKE_COMPACT_RULESET_H

#include <stdint.h>
#include <vector>

typedef std::vector<uint8_t>	VEC_BYTE;

//do the whole thing
void make_compact_ruleset ( VEC_BYTE& abyBlob );


#endif
