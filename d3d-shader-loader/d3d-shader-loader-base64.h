#pragma once
#include <stdint.h>
#include <string>
#include <vector>


namespace LoaderPriv {

	std::vector<uint8_t> FromBase64(const std::string& str);

};
