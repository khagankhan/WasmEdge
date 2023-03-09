// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "aot/compiler.h"
#include "common/configure.h"
#include "common/errinfo.h"
#include "common/filesystem.h"
#include "loader/loader.h"
#include "runtime/instance/module.h"
#include "validator/validator.h"
#include "vm/vm.h"

#include <gtest/gtest.h>
#include <iostream>
#include <vector>

namespace {

char Module1Path[] = "mixcallTestData/module1.wasm";
char Module1UniPath[] = "mixcallTestData/module1-uni.wasm";
char Module2Path[] = "mixcallTestData/module2.wasm";
char Module2UniPath[] = "mixcallTestData/module2-uni.wasm";
std::vector<uint8_t> Module1Wasm = {
    0x0,  0x61, 0x73, 0x6d, 0x1,  0x0,  0x0,  0x0,  0x1,  0x13, 0x4,  0x60,
    0x2,  0x7f, 0x7f, 0x0,  0x60, 0x2,  0x7c, 0x7c, 0x0,  0x60, 0x1,  0x7f,
    0x0,  0x60, 0x1,  0x7c, 0x0,  0x2,  0x59, 0x4,  0x6,  0x6d, 0x6f, 0x64,
    0x75, 0x6c, 0x65, 0xd,  0x69, 0x6d, 0x70, 0x6c, 0x5f, 0x70, 0x72, 0x69,
    0x6e, 0x74, 0x41, 0x64, 0x64, 0x0,  0x0,  0x6,  0x6d, 0x6f, 0x64, 0x75,
    0x6c, 0x65, 0xd,  0x69, 0x6d, 0x70, 0x6c, 0x5f, 0x70, 0x72, 0x69, 0x6e,
    0x74, 0x44, 0x69, 0x76, 0x0,  0x1,  0x4,  0x68, 0x6f, 0x73, 0x74, 0xd,
    0x68, 0x6f, 0x73, 0x74, 0x5f, 0x70, 0x72, 0x69, 0x6e, 0x74, 0x49, 0x33,
    0x32, 0x0,  0x2,  0x4,  0x68, 0x6f, 0x73, 0x74, 0xd,  0x68, 0x6f, 0x73,
    0x74, 0x5f, 0x70, 0x72, 0x69, 0x6e, 0x74, 0x46, 0x36, 0x34, 0x0,  0x3,
    0x3,  0x5,  0x4,  0x0,  0x1,  0x2,  0x3,  0x7,  0x2d, 0x4,  0x8,  0x70,
    0x72, 0x69, 0x6e, 0x74, 0x41, 0x64, 0x64, 0x0,  0x4,  0x8,  0x70, 0x72,
    0x69, 0x6e, 0x74, 0x44, 0x69, 0x76, 0x0,  0x5,  0x8,  0x70, 0x72, 0x69,
    0x6e, 0x74, 0x49, 0x33, 0x32, 0x0,  0x6,  0x8,  0x70, 0x72, 0x69, 0x6e,
    0x74, 0x46, 0x36, 0x34, 0x0,  0x7,  0xa,  0x21, 0x4,  0x8,  0x0,  0x20,
    0x0,  0x20, 0x1,  0x10, 0x0,  0xb,  0x8,  0x0,  0x20, 0x0,  0x20, 0x1,
    0x10, 0x1,  0xb,  0x6,  0x0,  0x20, 0x0,  0x10, 0x2,  0xb,  0x6,  0x0,
    0x20, 0x0,  0x10, 0x3,  0xb,  0x0,  0x8e, 0x1,  0x4,  0x6e, 0x61, 0x6d,
    0x65, 0x1,  0x3d, 0x4,  0x0,  0xd,  0x69, 0x6d, 0x70, 0x6c, 0x5f, 0x70,
    0x72, 0x69, 0x6e, 0x74, 0x41, 0x64, 0x64, 0x1,  0xd,  0x69, 0x6d, 0x70,
    0x6c, 0x5f, 0x70, 0x72, 0x69, 0x6e, 0x74, 0x44, 0x69, 0x76, 0x2,  0xd,
    0x68, 0x6f, 0x73, 0x74, 0x5f, 0x70, 0x72, 0x69, 0x6e, 0x74, 0x49, 0x33,
    0x32, 0x3,  0xd,  0x68, 0x6f, 0x73, 0x74, 0x5f, 0x70, 0x72, 0x69, 0x6e,
    0x74, 0x46, 0x36, 0x34, 0x2,  0x29, 0x8,  0x0,  0x2,  0x0,  0x0,  0x1,
    0x0,  0x1,  0x2,  0x0,  0x0,  0x1,  0x0,  0x2,  0x1,  0x0,  0x0,  0x3,
    0x1,  0x0,  0x0,  0x4,  0x2,  0x0,  0x0,  0x1,  0x0,  0x5,  0x2,  0x0,
    0x0,  0x1,  0x0,  0x6,  0x1,  0x0,  0x0,  0x7,  0x1,  0x0,  0x0,  0x4,
    0x1d, 0x4,  0x0,  0x5,  0x74, 0x79, 0x70, 0x65, 0x30, 0x1,  0x5,  0x74,
    0x79, 0x70, 0x65, 0x31, 0x2,  0x5,  0x74, 0x79, 0x70, 0x65, 0x32, 0x3,
    0x5,  0x74, 0x79, 0x70, 0x65, 0x33};
std::vector<uint8_t> Module2Wasm = {
    0x0,  0x61, 0x73, 0x6d, 0x1,  0x0,  0x0,  0x0,  0x1,  0x13, 0x4,  0x60,
    0x1,  0x7f, 0x0,  0x60, 0x1,  0x7c, 0x0,  0x60, 0x2,  0x7f, 0x7f, 0x0,
    0x60, 0x2,  0x7c, 0x7c, 0x0,  0x2,  0x2b, 0x2,  0x4,  0x68, 0x6f, 0x73,
    0x74, 0xd,  0x68, 0x6f, 0x73, 0x74, 0x5f, 0x70, 0x72, 0x69, 0x6e, 0x74,
    0x49, 0x33, 0x32, 0x0,  0x0,  0x4,  0x68, 0x6f, 0x73, 0x74, 0xd,  0x68,
    0x6f, 0x73, 0x74, 0x5f, 0x70, 0x72, 0x69, 0x6e, 0x74, 0x46, 0x36, 0x34,
    0x0,  0x1,  0x3,  0x3,  0x2,  0x2,  0x3,  0x7,  0x21, 0x2,  0xd,  0x69,
    0x6d, 0x70, 0x6c, 0x5f, 0x70, 0x72, 0x69, 0x6e, 0x74, 0x41, 0x64, 0x64,
    0x0,  0x2,  0xd,  0x69, 0x6d, 0x70, 0x6c, 0x5f, 0x70, 0x72, 0x69, 0x6e,
    0x74, 0x44, 0x69, 0x76, 0x0,  0x3,  0xa,  0x15, 0x2,  0x9,  0x0,  0x20,
    0x0,  0x20, 0x1,  0x6a, 0x10, 0x0,  0xb,  0x9,  0x0,  0x20, 0x0,  0x20,
    0x1,  0xa3, 0x10, 0x1,  0xb,  0x0,  0x4e, 0x4,  0x6e, 0x61, 0x6d, 0x65,
    0x1,  0x1f, 0x2,  0x0,  0xd,  0x68, 0x6f, 0x73, 0x74, 0x5f, 0x70, 0x72,
    0x69, 0x6e, 0x74, 0x49, 0x33, 0x32, 0x1,  0xd,  0x68, 0x6f, 0x73, 0x74,
    0x5f, 0x70, 0x72, 0x69, 0x6e, 0x74, 0x46, 0x36, 0x34, 0x2,  0x15, 0x4,
    0x0,  0x1,  0x0,  0x0,  0x1,  0x1,  0x0,  0x0,  0x2,  0x2,  0x0,  0x0,
    0x1,  0x0,  0x3,  0x2,  0x0,  0x0,  0x1,  0x0,  0x4,  0xf,  0x2,  0x0,
    0x5,  0x74, 0x79, 0x70, 0x65, 0x30, 0x1,  0x5,  0x74, 0x79, 0x70, 0x65,
    0x31};

void HexToFile(std::vector<uint8_t> &Wasm, const char *Path) {
  std::ofstream TFile(std::filesystem::u8path(Path), std::ios_base::binary);
  for (auto &Hex : Wasm) {
    TFile << Hex;
  }
  TFile.close();
}

class HostPrintI32 : public WasmEdge::Runtime::HostFunction<HostPrintI32> {
public:
  WasmEdge::Expect<void> body(const WasmEdge::Runtime::CallingFrame &,
                              uint32_t Val) {
    std::cout << "-- Host Function: print I32 " << Val << std::endl;
    return {};
  }
};

class HostPrintF64 : public WasmEdge::Runtime::HostFunction<HostPrintF64> {
public:
  WasmEdge::Expect<void> body(const WasmEdge::Runtime::CallingFrame &,
                              double Val) {
    std::cout << "-- Host Function: print F64 " << Val << std::endl;
    return {};
  }
};

class HostModule : public WasmEdge::Runtime::Instance::ModuleInstance {
public:
  HostModule() : ModuleInstance("host") {
    addHostFunc("host_printI32", std::make_unique<HostPrintI32>());
    addHostFunc("host_printF64", std::make_unique<HostPrintF64>());
  }
  ~HostModule() noexcept override = default;
};

bool compileModule(const WasmEdge::Configure &Conf, std::string_view InPath,
                   std::string_view OutPath) {
  WasmEdge::Loader::Loader Load(Conf);
  WasmEdge::Validator::Validator Valid(Conf);
  WasmEdge::AOT::Compiler Compiler(Conf);

  auto Mod = Load.parseModule(InPath);
  auto Data = Load.loadFile(InPath);
  if (!Mod || !Data) {
    return false;
  }
  if (auto Res = Valid.validate(*(*Mod).get()); !Res) {
    return false;
  }
  if (auto Res = Compiler.compile(*Data, *(*Mod).get(), OutPath); !Res) {
    return false;
  }
  return true;
}

TEST(MixCallTest, Call__InterpCallAOT) {
  WasmEdge::Configure Conf;
  WasmEdge::VM::VM VM(Conf);
  WasmEdge::Expect<void> Res;
  HostModule HostMod;
  std::vector<WasmEdge::ValVariant> FuncArgs;
  std::vector<WasmEdge::ValType> FuncArgTypes;

  // Prepare module2 file
  HexToFile(Module2Wasm, Module2Path);

  // Compile the `module2` into AOT mode.
  EXPECT_TRUE(compileModule(Conf, Module2Path, Module2UniPath));

  // Register the `module2` and the host module.
  Res = VM.registerModule(HostMod);
  EXPECT_TRUE(Res);
  Res = VM.registerModule("module", Module2UniPath);
  EXPECT_TRUE(Res);

  // Prepare Module1 file
  HexToFile(Module1Wasm, Module1Path);
  // Instantiate `module1`.
  Res = VM.loadWasm(Module1Path);
  EXPECT_TRUE(Res);
  Res = VM.validate();
  EXPECT_TRUE(Res);
  Res = VM.instantiate();
  EXPECT_TRUE(Res);

  // Run `printAdd`
  FuncArgs = {uint32_t(1234), uint32_t(5678)};
  FuncArgTypes = {WasmEdge::ValTypeCode::I32, WasmEdge::ValTypeCode::I32};
  auto Ret = VM.execute("printAdd", FuncArgs, FuncArgTypes);
  EXPECT_TRUE(Ret);
  EXPECT_EQ((*Ret).size(), 0);

  // Run `printDiv`
  FuncArgs = {double(9876.0), double(4321.0)};
  FuncArgTypes = {WasmEdge::ValTypeCode::F64, WasmEdge::ValTypeCode::F64};
  Ret = VM.execute("printDiv", FuncArgs, FuncArgTypes);
  EXPECT_TRUE(Ret);
  EXPECT_EQ((*Ret).size(), 0);

  // Run `printI32`
  FuncArgs = {uint32_t(87654321)};
  FuncArgTypes = {WasmEdge::ValTypeCode::I32};
  Ret = VM.execute("printI32", FuncArgs, FuncArgTypes);
  EXPECT_TRUE(Ret);
  EXPECT_EQ((*Ret).size(), 0);

  // Run `printF64`
  FuncArgs = {double(5566.1122)};
  FuncArgTypes = {WasmEdge::ValTypeCode::F64};
  Ret = VM.execute("printF64", FuncArgs, FuncArgTypes);
  EXPECT_TRUE(Ret);
  EXPECT_EQ((*Ret).size(), 0);
}

TEST(MixCallTest, Call__AOTCallInterp) {
  WasmEdge::Configure Conf;
  WasmEdge::VM::VM VM(Conf);
  WasmEdge::Expect<void> Res;
  HostModule HostMod;
  std::vector<WasmEdge::ValVariant> FuncArgs;
  std::vector<WasmEdge::ValType> FuncArgTypes;

  // Prepare Module1 file
  HexToFile(Module1Wasm, Module1Path);

  // Compile the `module1` into AOT mode.
  EXPECT_TRUE(compileModule(Conf, Module1Path, Module1UniPath));

  // Prepare Module2 file
  HexToFile(Module2Wasm, Module2Path);

  // Register the `module2` and the host module.
  Res = VM.registerModule(HostMod);
  EXPECT_TRUE(Res);
  Res = VM.registerModule("module", Module2Path);
  EXPECT_TRUE(Res);

  // Instantiate `module1`.
  Res = VM.loadWasm(Module1UniPath);
  EXPECT_TRUE(Res);
  Res = VM.validate();
  EXPECT_TRUE(Res);
  Res = VM.instantiate();
  EXPECT_TRUE(Res);

  // Run `printAdd`
  FuncArgs = {uint32_t(1234), uint32_t(5678)};
  FuncArgTypes = {WasmEdge::ValTypeCode::I32, WasmEdge::ValTypeCode::I32};
  auto Ret = VM.execute("printAdd", FuncArgs, FuncArgTypes);
  EXPECT_TRUE(Ret);
  EXPECT_EQ((*Ret).size(), 0);

  // Run `printDiv`
  FuncArgs = {double(9876.0), double(4321.0)};
  FuncArgTypes = {WasmEdge::ValTypeCode::F64, WasmEdge::ValTypeCode::F64};
  Ret = VM.execute("printDiv", FuncArgs, FuncArgTypes);
  EXPECT_TRUE(Ret);
  EXPECT_EQ((*Ret).size(), 0);

  // Run `printI32`
  FuncArgs = {uint32_t(87654321)};
  FuncArgTypes = {WasmEdge::ValTypeCode::I32};
  Ret = VM.execute("printI32", FuncArgs, FuncArgTypes);
  EXPECT_TRUE(Ret);
  EXPECT_EQ((*Ret).size(), 0);

  // Run `printF64`
  FuncArgs = {double(5566.1122)};
  FuncArgTypes = {WasmEdge::ValTypeCode::F64};
  Ret = VM.execute("printF64", FuncArgs, FuncArgTypes);
  EXPECT_TRUE(Ret);
  EXPECT_EQ((*Ret).size(), 0);
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
