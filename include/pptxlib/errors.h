// Phase 0 deliverable: error-handling model skeleton.
// See ARCHITECTURE.md section 4. This is a header ONLY — no parser,
// no ZIP/XML logic. Intentionally minimal until Phase 1.
#pragma once

#include <optional>
#include <string>
#include <variant>

namespace pptxlib {

enum class ErrorCode {
    InvalidPackage,
    InvalidXml,
    MissingPart,
    InvalidRelationship,
    ResourceLimitExceeded,
    Unsupported,
    IoError,
};

struct Error {
    ErrorCode code;
    std::string message;
    std::optional<std::string> context; // e.g. offending part path
};

// Minimal Result<T, Error>. A vendored single-header implementation
// (or std::expected, if/when the project's minimum C++ standard is
// raised to C++23) should replace this in Phase 1 — this skeleton
// exists only to fix the shape of the API surface for Phase 0.
template <typename T>
class Result {
public:
    static Result Ok(T value) { return Result(std::move(value)); }
    static Result Err(Error error) { return Result(std::move(error)); }

    bool has_value() const { return std::holds_alternative<T>(storage_); }
    explicit operator bool() const { return has_value(); }

    const T& value() const { return std::get<T>(storage_); }
    T& value() { return std::get<T>(storage_); }
    const Error& error() const { return std::get<Error>(storage_); }

private:
    explicit Result(T value) : storage_(std::move(value)) {}
    explicit Result(Error error) : storage_(std::move(error)) {}
    std::variant<T, Error> storage_;
};

} // namespace pptxlib
