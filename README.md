# text2speech001
This is an ad-hoc tool for compacting Text-to-Speech (for SP0256-AL2) rules into a form for embedded use.  It was created along the was of another project 'BluePillSP0256AL2'.  It was built with Visual Studio 2017, but is otherwise a garden-variety C++ command line application.

This program is not a paragon of software design, but it evolved from a throw-away program for verifying my original (human readable) ruleset, and then when that straightforward declaration proved to require too much flash, it accumulated the mechanism for 'compactifying' the ruleset into a blob.  The compactification process includes de-duping strings, and creating a self-referencing structure using 16-bit offsets (instead of 32-bit pointers).

Since this is now the 'master' ruleset, it made sense to check this in, but because of the peculiarities of System Workbench for STM32, it needs to be in a separate repo from the BluePillSP0256AL2. (Otherwise, SWSTM will find the source and try to compile it as part of the BluePill project.)

The human-readable form of the rules is in tts_rules.c.  There are comments there that explain how they work.

The main() function has some tests that are commented out (but left for posterity), and what is left will emit to stdout the contents of a .h and a .c file that contain the tts rules compact blob.  Conventionally I name these files 'tts_rules_compact.h' and 'tts_rules_compact.c'.  These are processed by code in the BluePillSP0256AL2 project in 'text_to_speech.h' and 'text_to_speech.c'.
