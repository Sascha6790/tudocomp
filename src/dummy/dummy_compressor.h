#ifndef DUMMY_COMPRESSOR_H
#define DUMMY_COMPRESSOR_H

#include "tudocomp.h"

// Put every C++ code in this project into a common namespace
// in order to not pollute the global one
namespace dummy {

using namespace tudocomp;

class DummyCompressor: public Compressor {
public:
    inline DummyCompressor(Env& env): Compressor(env) {}

    virtual Rules compress(const Input& input, size_t threshold) final override;
};

}

#endif
