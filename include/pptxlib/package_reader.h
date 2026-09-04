// Phase 0 deliverable: abstract package-reader interface skeleton.
// See ARCHITECTURE.md section 7. Interface only — described purely as
// "a byte-range-readable source." No implementation (file-backed or
// otherwise) belongs in this header; that is Phase 1 work.
#pragma once

#include <cstddef>
#include <cstdint>

#include "errors.h"

namespace pptxlib {

class IPackageReader {
public:
    virtual ~IPackageReader() = default;

    // Reads up to `size` bytes starting at `offset` into `buffer`.
    // Returns the number of bytes actually read, or an Error.
    virtual Result<size_t> Read(uint64_t offset, void* buffer, size_t size) = 0;

    // Returns the total size of the underlying source, or an Error.
    virtual Result<uint64_t> Size() const = 0;
};

} // namespace pptxlib
