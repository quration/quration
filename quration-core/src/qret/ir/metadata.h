/**
 * @file qret/ir/metadata.h
 * @brief Metadata of instruction.
 */

#ifndef QRET_IR_METADATA_H
#define QRET_IR_METADATA_H

#include <fmt/core.h>

#include <cassert>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>

#include "qret/ir/value.h"

namespace qret::ir {
enum class MDKind : std::uint8_t {
    // Annotation
    Annotation,
    Qubit,
    Register,
    // Debug
    BreakPoint,
    Location,  // Source location
};
inline std::string MDKindToString(MDKind md_kind) {
    switch (md_kind) {
        case MDKind::Annotation:
            return "Annotation";
        case MDKind::Qubit:
            return "Qubit";
        case MDKind::Register:
            return "Register";
        case MDKind::BreakPoint:
            return "Breakpoint";
        case MDKind::Location:
            return "Location";
        default:
            throw std::logic_error("unreachable");
    }
}

class Metadata {
public:
    virtual ~Metadata() = default;

    MDKind GetMDKind() const {
        return kind_;
    }

    virtual void Print(std::ostream& out) const = 0;
    virtual void PrintAsOperand(std::ostream& out) const {
        out << '!';
        Print(out);
    }

protected:
    Metadata(const MDKind& kind)  // NOLINT
        : kind_{kind} {}

private:
    const MDKind kind_;
};
class MDAnnotation : public Metadata {
public:
    explicit MDAnnotation(std::string_view annot)
        : Metadata(MDKind::Annotation)
        , annot_{annot} {}
    static std::unique_ptr<MDAnnotation> Create(std::string_view annot) {
        return std::unique_ptr<MDAnnotation>(new MDAnnotation(annot));
    }
    void Print(std::ostream& out) const override {
        out << fmt::format("\"{}\"", annot_);
    }
    std::string_view GetAnnotation() const {
        return annot_;
    }

private:
    std::string annot_;
};
class MDQubit : public Metadata {
public:
    explicit MDQubit(const Qubit& q)
        : Metadata(MDKind::Qubit)
        , q_{q} {}
    static std::unique_ptr<MDQubit> Create(const Qubit& q) {
        return std::unique_ptr<MDQubit>(new MDQubit(q));
    }
    void Print(std::ostream& out) const override {
        q_.Print(out);
    }
    Qubit GetQubit() const {
        return q_;
    }

private:
    Qubit q_;
};
class MDRegister : public Metadata {
public:
    explicit MDRegister(const Register& r)
        : Metadata(MDKind::Register)
        , r_{r} {}
    static std::unique_ptr<MDRegister> Create(const Register& r) {
        return std::unique_ptr<MDRegister>(new MDRegister(r));
    }
    void Print(std::ostream& out) const override {
        r_.Print(out);
    }
    Register GetRegister() const {
        return r_;
    }

private:
    Register r_;
};
class DIBreakPoint : public Metadata {
public:
    explicit DIBreakPoint(std::string_view name)
        : Metadata(MDKind::BreakPoint)
        , name_{name} {}
    static std::unique_ptr<DIBreakPoint> Create(std::string_view name) {
        return std::unique_ptr<DIBreakPoint>(new DIBreakPoint(name));
    }
    void Print(std::ostream& out) const override {
        out << fmt::format("DIBreakPoint(name: {})", name_);
    }
    std::string_view GetName() const {
        return name_;
    }

private:
    std::string name_;
};
class DILocation : public Metadata {
public:
    explicit DILocation(std::string_view file, std::uint32_t line)
        : Metadata(MDKind::Location)
        , file_{file}
        , line_{line} {}
    static std::unique_ptr<DILocation> Create(std::string_view file, std::uint32_t line) {
        return std::unique_ptr<DILocation>(new DILocation(file, line));
    }
    void Print(std::ostream& out) const override {
        out << fmt::format("DILocation(file: {}, line: {})", file_, line_);
    }
    std::string_view GetFile() const {
        return file_;
    }
    std::uint32_t GetLine() const {
        return line_;
    }

private:
    std::string file_;
    std::uint32_t line_;
};
}  // namespace qret::ir

#endif  // QRET_IR_METADATA_H
