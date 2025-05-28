#include "d3d-shader-loader-base64.h"



namespace LoaderPriv {

    std::vector<uint8_t> FromBase64(const std::string& base64Str)
    {
        // Thanks chatgpt!
        static const int8_t decodingTable[256] = {
            -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,  //   0 -  15
            -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,  //  16 -  31
            -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,62,-1,-1,-1,63,  //  32 -  47
            52,53,54,55,56,57,58,59, 60,61,-1,-1,-1, 0,-1,-1,  //  48 -  63
            -1, 0, 1, 2, 3, 4, 5, 6,  7, 8, 9,10,11,12,13,14,  //  64 -  79
            15,16,17,18,19,20,21,22, 23,24,25,-1,-1,-1,-1,-1,  //  80 -  95
            -1,26,27,28,29,30,31,32, 33,34,35,36,37,38,39,40,  //  96 - 111
            41,42,43,44,45,46,47,48, 49,50,51,-1,-1,-1,-1,-1,  // 112 - 127
            -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,  // 128 - 143
            -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,  // 144 - 159
            -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,  // 160 - 175
            -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,  // 176 - 191
            -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,  // 192 - 207
            -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,  // 208 - 223
            -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,  // 224 - 239
            -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1   // 240 - 255
        };

        std::string filteredInput;
        for (char c : base64Str) {
            if ((unsigned char)c < 256 && (decodingTable[(unsigned char)c] != -1 || c == '=')) {
                filteredInput += c;
            }
        }

        size_t inputLength = filteredInput.size();
        if (inputLength % 4 != 0) {
            return std::vector<uint8_t>();
        }

        std::vector<uint8_t> result;
        uint8_t charArray4[4], charArray3[3];

        for (size_t i = 0; i < inputLength; i += 4) {
            for (size_t j = 0; j < 4; ++j) {
                charArray4[j] = (filteredInput[i + j] == '=')
                    ? 0
                    : decodingTable[static_cast<unsigned char>(filteredInput[i + j])];
            }

            charArray3[0] = (charArray4[0] << 2) | ((charArray4[1] & 0x30) >> 4);
            charArray3[1] = ((charArray4[1] & 0xf) << 4) | ((charArray4[2] & 0x3c) >> 2);
            charArray3[2] = ((charArray4[2] & 0x3) << 6) | charArray4[3];

            result.push_back(charArray3[0]);
            if (filteredInput[i + 2] != '=') result.push_back(charArray3[1]);
            if (filteredInput[i + 3] != '=') result.push_back(charArray3[2]);
        }

        return result;
    }

}

