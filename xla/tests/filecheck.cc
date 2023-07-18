/* Copyright 2017 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "xla/tests/filecheck.h"

#include <cstdlib>

#include "xla/types.h"
#include "xla/util.h"
#include "tsl/platform/env.h"
#include "tsl/platform/errors.h"
#include "tsl/platform/path.h"
#include "tsl/platform/resource_loader.h"
#include "tsl/platform/subprocess.h"

namespace xla {

StatusOr<bool> RunFileCheck(const std::string& input,
                            absl::string_view pattern) {
  // Generate an input file for the FileCheck pattern.
  std::string pattern_path;
  auto env = tsl::Env::Default();
  if (!env->LocalTempFilename(&pattern_path)) {
    return absl::InternalError("couldn't get a pattern file name");
  }
  TF_RETURN_IF_ERROR(tsl::WriteStringToFile(env, pattern_path, pattern));

  return RunFileCheckWithPatternFile(input, pattern_path);
}

StatusOr<bool> RunFileCheckWithPatternFile(const std::string& input,
                                           const std::string& pattern_file) {
  // Invoke FileCheck to check whether input matches `pattern`.
  std::string file_check_path = tsl::GetDataDependencyFilepath(
      tsl::io::JoinPath("external", "llvm-project", "llvm", "FileCheck"));

  tsl::SubProcess file_check_process;
  file_check_process.SetProgram(file_check_path,
                                {file_check_path, "-v", "-dump-input=fail",
                                 "--dump-input-filter=all", pattern_file});
  file_check_process.SetChannelAction(tsl::CHAN_STDIN, tsl::ACTION_PIPE);
  file_check_process.SetChannelAction(tsl::CHAN_STDERR, tsl::ACTION_PIPE);
  if (!file_check_process.Start()) {
    return absl::InternalError("couldn't start FileCheck");
  }

  std::string standard_error;
  int exit_status = file_check_process.Communicate(
      /*stdin_input=*/&input, /*stdout_output=*/nullptr,
      /*stderr_output=*/&standard_error);

  // FileCheck returns 0 when the inputs match. If matching failed, log
  // the error message generated by FileCheck and the inputs.
  bool succeeded = (exit_status == 0);
  auto env = tsl::Env::Default();
  if (!succeeded) {
    LOG(ERROR) << "Tried to execute FileCheck at " << file_check_path;
    if (!env->FileExists(file_check_path).ok()) {
      LOG(ERROR) << "NOTE: FileCheck binary does not exist!";
    }

    // Log at ERROR level so these show up even if you don't pass --logtostderr.
    LOG(ERROR) << "FileCheck stderr:\n" << standard_error;
    LOG(ERROR) << "FileCheck input was:\n" << input;
  } else if (!standard_error.empty()) {
    LOG(INFO) << "FileCheck stderr:\n" << standard_error;
    LOG(INFO) << "FileCheck input was:\n" << input;
  }
  return succeeded;
}

}  // namespace xla
