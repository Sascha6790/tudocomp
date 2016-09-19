/*
    Include this to include practically all of tudocomp.

    This header also contains the Doxygen documentation of the main namespaces.
*/

#ifndef _INCLUDED_TUDOCOMP_HPP
#define _INCLUDED_TUDOCOMP_HPP

/// \brief Contains the text compression and encoding framework.
///
/// This is the framework's central namespace in which text compression and
/// coding algorithms are contained, as well as utilities needed for text
/// compression and coding (e.g. I/O). Families of compressors and encoders
/// or utility groups are contained in the respective sub-namespaces. The
/// namespace \c tudocomp itself contains types important for all of the
/// framework and its communication.
namespace tudocomp {

/// \brief Contains I/O abstractions and utilities.
///
/// All I/O done by compressors and encoders is routed through the \ref Input
/// and \ref Output abstractions over the underlying file, memory or stream
/// I/O.
///
/// \sa
/// \ref Input for the input interface and \ref Output for the output
/// interface.
namespace io {
}

/// \brief Contains compressors and encoders that work with Lempel-Ziv-78-like
/// dictionaries.
///
/// The LZ78 family works with bottom-up dictionaries containing indexed
/// entries to achieve compression. Each entry points to a \e prefix (another
/// dictionary entry) and a follow-up symbol.
namespace lz78 {
}

/// \brief Contains compressors and encoders that work with
/// Lempel-Ziv-Storer-Szymansky-like factors.
///
/// The LZSS family works with factors representing references to positions
/// within the original text that replace parts of the same text, effectively
/// using the input text itself as a dictionary. They consist of a \e source
/// text position and a \e length.
namespace lzss {
}

/// \brief Contains compressors and encoders that work with
/// Lempel-Ziv-Welch-like dictionaries.
///
/// The LZW family works with bottom-up dictionaries containing indexed entries
/// to achieve compression. Other than \ref lz78, the dictionary entries do not
/// explicitly store the follow-up symbol. Instead, they are re-generated on
/// the fly by the decoder.
namespace lzw {
}

} //namespace tudocomp

// Include f'in everything!
#include <glog/logging.h>
#include <sdsl/int_vector.hpp>

#include <tudocomp/io.h>
#include <tudocomp/util.h>

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ChainCompressor.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/Env.hpp>

#include <tudocomp/util/Counter.hpp>
#include <tudocomp/util/DecodeBuffer.hpp>

#include <tudocomp/ds/TextDS.hpp>
#include <tudocomp/ds/SuffixArray.hpp>
#include <tudocomp/ds/InverseSuffixArray.hpp>
#include <tudocomp/ds/PhiArray.hpp>
#include <tudocomp/ds/LCPArray.hpp>
#include <tudocomp/ds/uint_t.hpp>

#include <tudocomp/lz78/Lz78Compressor.hpp>
#include <tudocomp/lz78/Lz78DebugCoder.hpp>
#include <tudocomp/lz78/Lz78BitCoder.hpp>

#include <tudocomp/lzw/LzwCompressor.hpp>
#include <tudocomp/lzw/LzwDebugCoder.hpp>
#include <tudocomp/lzw/LzwBitCoder.hpp>

#include <tudocomp/lz78/lzcics/Lz78cicsCompressor.hpp>

#include <tudocomp/lzss/LZ77SSSlidingWindowCompressor.hpp>
#include <tudocomp/lzss/LZ77SSLCPCompressor.hpp>
#include <tudocomp/lzss/LZSSESACompressor.hpp>

#include <tudocomp/lzss/DebugLZSSCoder.hpp>
#include <tudocomp/lzss/OnlineLZSSCoder.hpp>
#include <tudocomp/lzss/OfflineLZSSCoder.hpp>

#include <tudocomp/alphabet/OnlineAlphabetCoder.hpp>
#include <tudocomp/alphabet/OfflineAlphabetCoder.hpp>

#endif
