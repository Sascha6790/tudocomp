#pragma once

#include <tudocomp/ds/DSProvider.hpp>

namespace tdc {

/// Constructs the inverse suffix array from the suffix array.
class ISAFromSA : public DSProvider {
public:
    inline static Meta meta() {
        Meta m("provider", "isa");
        return m;
    }

    using provides = tl::set<ds::INVERSE_SUFFIX_ARRAY, ISAFromSA>;

    using DSProvider::DSProvider;

    virtual dsid_list_t requirements() const override {
        return dsid_list_t { ds::SUFFIX_ARRAY };
    }

    virtual dsid_list_t products() const override {
        return dsid_list_t { ds::INVERSE_SUFFIX_ARRAY };
    }
};

} //ns