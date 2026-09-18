file(READ "${TTK_EMBED_INPUT}" bytes HEX)

string(REGEX REPLACE "(..)" "0x\\1," bytes "${bytes}")
string(REGEX REPLACE "(0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,)" "\\1\n    " bytes "${bytes}")

file(WRITE "${TTK_EMBED_OUTPUT}"
"// Generated from ${TTK_EMBED_INPUT}. Do not edit.

#include <cstddef>

namespace Embedded {

extern const unsigned char ${TTK_EMBED_SYMBOL}[];
extern const std::size_t ${TTK_EMBED_SYMBOL}Size;

const unsigned char ${TTK_EMBED_SYMBOL}[] = {
    ${bytes}
};

const std::size_t ${TTK_EMBED_SYMBOL}Size = sizeof(${TTK_EMBED_SYMBOL});

}
")
