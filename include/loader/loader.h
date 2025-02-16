// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/loader/loader.h - Loader flow control class definition ---===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Loader class, which controls flow
/// of WASM loading.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/module.h"
#include "common/configure.h"
#include "common/errinfo.h"
#include "common/log.h"
#include "loader/filemgr.h"
#include "loader/ldmgr.h"
#include "loader/serialize.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

namespace WasmEdge {
namespace Loader {

namespace {
template <typename T> inline ASTNodeAttr NodeAttrFromAST() noexcept;
template <> inline ASTNodeAttr NodeAttrFromAST<AST::CustomSection>() noexcept {
  return ASTNodeAttr::Sec_Custom;
}
template <> inline ASTNodeAttr NodeAttrFromAST<AST::TypeSection>() noexcept {
  return ASTNodeAttr::Sec_Type;
}
template <> inline ASTNodeAttr NodeAttrFromAST<AST::ImportSection>() noexcept {
  return ASTNodeAttr::Sec_Import;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::FunctionSection>() noexcept {
  return ASTNodeAttr::Sec_Function;
}
template <> inline ASTNodeAttr NodeAttrFromAST<AST::TableSection>() noexcept {
  return ASTNodeAttr::Sec_Table;
}
template <> inline ASTNodeAttr NodeAttrFromAST<AST::MemorySection>() noexcept {
  return ASTNodeAttr::Sec_Memory;
}
template <> inline ASTNodeAttr NodeAttrFromAST<AST::GlobalSection>() noexcept {
  return ASTNodeAttr::Sec_Global;
}
template <> inline ASTNodeAttr NodeAttrFromAST<AST::ExportSection>() noexcept {
  return ASTNodeAttr::Sec_Export;
}
template <> inline ASTNodeAttr NodeAttrFromAST<AST::StartSection>() noexcept {
  return ASTNodeAttr::Sec_Start;
}
template <> inline ASTNodeAttr NodeAttrFromAST<AST::ElementSection>() noexcept {
  return ASTNodeAttr::Sec_Element;
}
template <> inline ASTNodeAttr NodeAttrFromAST<AST::CodeSection>() noexcept {
  return ASTNodeAttr::Sec_Code;
}
template <> inline ASTNodeAttr NodeAttrFromAST<AST::DataSection>() noexcept {
  return ASTNodeAttr::Sec_Data;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::DataCountSection>() noexcept {
  return ASTNodeAttr::Sec_DataCount;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::Component::AliasSection>() noexcept {
  return ASTNodeAttr::Sec_Alias;
}
template <>
inline ASTNodeAttr
NodeAttrFromAST<AST::Component::CoreInstanceSection>() noexcept {
  return ASTNodeAttr::Sec_CoreInstance;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::Component::InstanceSection>() noexcept {
  return ASTNodeAttr::Sec_Instance;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::Component::TypeSection>() noexcept {
  return ASTNodeAttr::Sec_CompType;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::Component::StartSection>() noexcept {
  return ASTNodeAttr::Sec_CompStart;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::Component::CanonSection>() noexcept {
  return ASTNodeAttr::Sec_Canon;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::Component::ImportSection>() noexcept {
  return ASTNodeAttr::Sec_CompImport;
}
template <>
inline ASTNodeAttr NodeAttrFromAST<AST::Component::ExportSection>() noexcept {
  return ASTNodeAttr::Sec_CompExport;
}

} // namespace

/// Loader flow control class.
class Loader {
public:
  Loader(const Configure &Conf,
         const AST::Module::IntrinsicsTable *IT = nullptr) noexcept
      : Conf(Conf), Ser(Conf), LMgr(IT), IntrinsicsTable(IT) {}
  ~Loader() noexcept = default;

  /// Load data from file path.
  static Expect<std::vector<Byte>>
  loadFile(const std::filesystem::path &FilePath);

  Expect<std::variant<AST::Component::Component, AST::Module>>
  parseWasmUnit(const std::filesystem::path &FilePath);
  Expect<std::variant<AST::Component::Component, AST::Module>>
  parseWasmUnit(Span<const uint8_t> Code);

  /// Parse component from file path.
  Expect<std::unique_ptr<AST::Component::Component>>
  parseComponent(const std::filesystem::path &FilePath);

  /// Parse component from byte code.
  Expect<std::unique_ptr<AST::Component::Component>>
  parseComponent(Span<const uint8_t> Code);

  /// Parse module from file path.
  Expect<std::unique_ptr<AST::Module>>
  parseModule(const std::filesystem::path &FilePath);

  /// Parse module from byte code.
  Expect<std::unique_ptr<AST::Module>> parseModule(Span<const uint8_t> Code);

  /// Serialize module into byte code.
  Expect<std::vector<Byte>> serializeModule(const AST::Module &Mod);

  /// Reset status.
  void reset() noexcept {
    FMgr.reset();
    LMgr.reset();
  }

private:
  /// \name Helper functions to print error log when loading AST nodes
  /// @{
  inline Unexpected<ErrCode> logLoadError(ErrCode Code, uint64_t Off,
                                          ASTNodeAttr Node) const noexcept {
    spdlog::error(Code);
    spdlog::error(ErrInfo::InfoLoading(Off));
    spdlog::error(ErrInfo::InfoAST(Node));
    return Unexpect(Code);
  }
  inline Unexpected<ErrCode> logNeedProposal(ErrCode Code, Proposal Prop,
                                             uint64_t Off,
                                             ASTNodeAttr Node) const noexcept {
    spdlog::error(Code);
    spdlog::error(ErrInfo::InfoProposal(Prop));
    spdlog::error(ErrInfo::InfoLoading(Off));
    spdlog::error(ErrInfo::InfoAST(Node));
    return Unexpect(Code);
  }
  /// @}

  Expect<std::variant<AST::Component::Component, AST::Module>> loadUnit();
  Expect<std::pair<std::vector<Byte>, std::vector<Byte>>> loadPreamble();

  /// \name Load AST Component functions
  /// @{
  Expect<void> loadComponent(AST::Component::Component &Comp);
  /// @}

  /// \name Load AST Module functions
  /// @{
  Expect<void> loadModule(AST::Module &Mod);
  Expect<void> loadCompiled(AST::Module &Mod);
  /// @}

  /// \name Load AST section node helper functions
  /// @{
  Expect<void> loadUniversalWASM(AST::Module &Mod);
  Expect<void> loadModuleAOT(AST::AOTSection &AOTSection);

  Expect<uint32_t> loadSectionSize(ASTNodeAttr Node);
  template <typename T, typename L>
  Expect<void> loadSectionContent(T &Sec, L &&Func) {
    Sec.setStartOffset(FMgr.getOffset());
    if (auto Res = loadSectionSize(NodeAttrFromAST<T>())) {
      // Set the section size.
      Sec.setContentSize(*Res);
      auto StartOffset = FMgr.getOffset();
      auto ResContent = Func();
      if (!ResContent) {
        return Unexpect(ResContent);
      }
      // Check the read size match the section size.
      auto EndOffset = FMgr.getOffset();
      if (EndOffset - StartOffset != Sec.getContentSize()) {
        return logLoadError(ErrCode::Value::SectionSizeMismatch, EndOffset,
                            NodeAttrFromAST<T>());
      }
    } else {
      return Unexpect(Res);
    }
    return {};
  }
  template <typename T, typename L>
  Expect<void> loadSectionContentVec(T &Sec, L &&Func) {
    uint32_t VecCnt = 0;
    // Read the vector size.
    if (auto Res = FMgr.readU32()) {
      VecCnt = *Res;
      if (VecCnt / 2 > FMgr.getRemainSize()) {
        return logLoadError(ErrCode::Value::IntegerTooLong,
                            FMgr.getLastOffset(), NodeAttrFromAST<T>());
      }
      Sec.getContent().resize(VecCnt);
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          NodeAttrFromAST<T>());
    }

    // Sequently create the AST node T and read data.
    for (uint32_t I = 0; I < VecCnt; ++I) {
      if (auto Res = Func(Sec.getContent()[I]); !Res) {
        spdlog::error(ErrInfo::InfoAST(NodeAttrFromAST<T>()));
        return Unexpect(Res);
      }
    }
    return {};
  }

  template <typename T, typename C, typename L>
  Expect<void> loadVec(std::vector<C> &Vec, L &&Func) {
    uint32_t VecCnt = 0;
    // Read the vector size.
    if (auto Res = FMgr.readU32()) {
      VecCnt = *Res;
      if (VecCnt / 2 > FMgr.getRemainSize()) {
        return logLoadError(ErrCode::Value::IntegerTooLong,
                            FMgr.getLastOffset(), NodeAttrFromAST<T>());
      }
      Vec.resize(VecCnt);
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          NodeAttrFromAST<T>());
    }

    // Sequently create the AST node T and read data.
    for (uint32_t I = 0; I < VecCnt; ++I) {
      if (auto Res = Func(Vec[I]); !Res) {
        spdlog::error(ErrInfo::InfoAST(NodeAttrFromAST<T>()));
        return Unexpect(Res);
      }
    }

    return {};
  }

  /// \name Load AST nodes functions
  /// @{
  Expect<void> loadSection(AST::CustomSection &Sec);
  Expect<void> loadSection(AST::TypeSection &Sec);
  Expect<void> loadSection(AST::ImportSection &Sec);
  Expect<void> loadSection(AST::FunctionSection &Sec);
  Expect<void> loadSection(AST::TableSection &Sec);
  Expect<void> loadSection(AST::MemorySection &Sec);
  Expect<void> loadSection(AST::GlobalSection &Sec);
  Expect<void> loadSection(AST::ExportSection &Sec);
  Expect<void> loadSection(AST::StartSection &Sec);
  Expect<void> loadSection(AST::ElementSection &Sec);
  Expect<void> loadSection(AST::CodeSection &Sec);
  Expect<void> loadSection(AST::DataSection &Sec);
  Expect<void> loadSection(AST::DataCountSection &Sec);
  Expect<void> loadSection(AST::Component::CoreInstanceSection &Sec);
  Expect<void> loadSection(AST::Component::InstanceSection &Sec);
  Expect<void> loadSection(AST::Component::AliasSection &Sec);
  Expect<void> loadSection(AST::Component::TypeSection &Sec);
  Expect<void> loadSection(AST::Component::CanonSection &Sec);
  Expect<void> loadSection(AST::Component::ImportSection &Sec);
  Expect<void> loadSection(AST::Component::ExportSection &Sec);
  static Expect<void> loadSection(FileMgr &VecMgr, AST::AOTSection &Sec);
  Expect<void> loadImport(AST::Component::Import &Im);
  Expect<void> loadExport(AST::Component::Export &Ex);
  Expect<void> loadCanonical(AST::Component::Canon &C);
  Expect<void> loadCanonical(AST::Component::Lift &C);
  Expect<void> loadCanonical(AST::Component::Lower &C);
  Expect<void> loadCanonicalOption(AST::Component::CanonOpt &C);
  Expect<void> loadSegment(AST::TableSegment &TabSeg);
  Expect<void> loadSegment(AST::GlobalSegment &GlobSeg);
  Expect<void> loadSegment(AST::ElementSegment &ElemSeg);
  Expect<void> loadSegment(AST::CodeSegment &CodeSeg);
  Expect<void> loadSegment(AST::DataSegment &DataSeg);
  Expect<void> loadDesc(AST::ImportDesc &ImpDesc);
  Expect<void> loadDesc(AST::ExportDesc &ExpDesc);
  Expect<ValType> loadHeapType(TypeCode TC, ASTNodeAttr From);
  Expect<ValType> loadRefType(ASTNodeAttr From);
  Expect<ValType> loadValType(ASTNodeAttr From);
  Expect<void> loadLimit(AST::Limit &Lim);
  Expect<void> loadType(AST::FunctionType &FuncType);
  Expect<void> loadType(AST::MemoryType &MemType);
  Expect<void> loadType(AST::TableType &TabType);
  Expect<void> loadType(AST::GlobalType &GlobType);

  Expect<void> loadType(AST::Component::DefType &Ty);
  Expect<void> loadType(AST::Component::FuncType &Ty);
  Expect<void> loadType(AST::Component::InstanceType &Ty);
  Expect<void> loadType(AST::Component::ComponentType &Ty);
  Expect<void> loadType(AST::Component::ResultList &Ty);
  Expect<void> loadType(AST::Component::Record &RecTy);
  Expect<void> loadType(AST::Component::VariantTy &Ty);
  Expect<void> loadType(AST::Component::List &Ty);
  Expect<void> loadType(AST::Component::Tuple &Ty);
  Expect<void> loadType(AST::Component::Flags &Ty);
  Expect<void> loadType(AST::Component::Enum &Ty);
  Expect<void> loadType(AST::Component::Option &Ty);
  Expect<void> loadType(AST::Component::Result &Ty);
  Expect<void> loadType(AST::Component::Own &Ty);
  Expect<void> loadType(AST::Component::Borrow &Ty);
  Expect<void> loadType(AST::Component::LabelValType &Ty);
  Expect<void> loadType(AST::Component::ValueType &Ty);
  Expect<void> loadType(AST::Component::CoreType &Ty);
  Expect<void> loadType(AST::Component::CoreDefType &Ty);
  Expect<void> loadType(AST::Component::ModuleType &Ty);
  Expect<void> loadCase(AST::Component::Case &C);
  Expect<void> loadLabel(std::string &Label);
  template <typename T>
  Expect<std::optional<T>> loadOption(std::function<Expect<void>(T &)> F) {
    auto RTag = FMgr.readByte();
    if (!RTag) {
      return Unexpect(RTag);
    }
    switch (*RTag) {
    case 0x01: {
      T V;
      if (auto Res = F(V); !Res) {
        return Unexpect(Res);
      }
      return std::optional<T>{V};
    }
    case 0x00:
      return std::nullopt;
    default:
      return logLoadError(ErrCode::Value::MalformedDefType,
                          FMgr.getLastOffset(), ASTNodeAttr::DefType);
    }
  }
  Expect<void> loadModuleDecl(AST::Component::ModuleDecl &Decl);
  Expect<void> loadExportDecl(AST::Component::CoreExportDecl &Decl);
  Expect<void> loadComponentDecl(AST::Component::ComponentDecl &Decl);
  Expect<void> loadImportDecl(AST::Component::ImportDecl &Decl);
  Expect<void> loadInstanceDecl(AST::Component::InstanceDecl &Decl);
  Expect<void> loadExternDesc(AST::Component::ExternDesc &Desc);
  Expect<void> loadImportExportName(std::string &Name);
  Expect<void> loadImportExportNameWithLen(std::string &Name);
  Expect<void> loadStart(AST::Component::Start &S);
  Expect<void> loadCoreInstance(AST::Component::CoreInstanceExpr &InstanceExpr);
  Expect<void> loadInstance(AST::Component::InstanceExpr &InstanceExpr);
  Expect<void> loadInstantiateArg(AST::Component::CoreInstantiateArg &Arg);
  Expect<void>
  loadInstantiateArg(AST::Component::InstantiateArg<
                     AST::Component::SortIndex<AST::Component::Sort>> &Arg);
  Expect<void>
  loadInlineExport(AST::Component::InlineExport<AST::Component::CoreSort> &Exp);
  Expect<void>
  loadInlineExport(AST::Component::InlineExport<AST::Component::Sort> &Exp);
  Expect<void> loadAlias(AST::Component::Alias &Alias);
  Expect<void> loadSort(AST::Component::Sort &Sort);
  Expect<void> loadAliasTarget(AST::Component::AliasTarget &AliasTarget);
  Expect<void> loadCoreSort(AST::Component::CoreSort &Sort);
  Expect<void>
  loadSortIndex(AST::Component::SortIndex<AST::Component::Sort> &SortIdx);
  Expect<void> loadCoreSortIndex(
      AST::Component::SortIndex<AST::Component::CoreSort> &SortIdx);
  Expect<void> loadExpression(AST::Expression &Expr,
                              std::optional<uint64_t> SizeBound = std::nullopt);
  Expect<OpCode> loadOpCode();
  Expect<AST::InstrVec> loadInstrSeq(std::optional<uint64_t> SizeBound);
  Expect<void> loadInstruction(AST::Instruction &Instr);
  /// @}

  /// \name Loader members
  /// @{
  const Configure Conf;
  const Serializer Ser;
  FileMgr FMgr;
  LDMgr LMgr;
  const AST::Module::IntrinsicsTable *IntrinsicsTable;
  std::recursive_mutex Mutex;
  bool HasDataSection;

  /// Input data type enumeration.
  enum class InputType : uint8_t { WASM, UniversalWASM, SharedLibrary };
  InputType WASMType = InputType::WASM;
  /// @}

  // Metadata
  std::vector<Byte> ModuleVersion = {0x01, 0x00, 0x00, 0x00};
  // spec says 0x0a, but it's actually 0x0d, where cargo component compiled out
  std::vector<Byte> ComponentVersion = {0x0d, 0x00, 0x01, 0x00};
};

} // namespace Loader
} // namespace WasmEdge
