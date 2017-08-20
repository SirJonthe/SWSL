#ifndef SWSL_JSON_H
#define SWSL_JSON_H

#include <fstream>
#include "swsl_astgen_new.h"

bool SerializeToJSON(const new_Token *tree, const mtlChars &file);

#endif // SWSL_JSON_H
