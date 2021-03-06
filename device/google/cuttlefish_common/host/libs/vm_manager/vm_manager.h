/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

#include <common/libs/utils/subprocess.h>
#include <host/libs/config/cuttlefish_config.h>

namespace vm_manager {

// Superclass of every guest VM manager. It provides a static getter that
// returns the requested vm manager as a singleton.
class VmManager {
 public:
  // Returns the most suitable vm manager as a singleton. It may return nullptr
  // if the requested vm manager is not supported by the current version of the
  // host packages
  static VmManager* Get(const std::string& vm_manager_name,
                        const vsoc::CuttlefishConfig* config);
  static bool IsValidName(const std::string& name);
  static void ConfigureBootDevices(vsoc::CuttlefishConfig* config);
  static bool IsVmManagerSupported(const std::string& name);
  static std::vector<std::string> GetValidNames();

  virtual ~VmManager() = default;

  virtual cvd::Command StartCommand() = 0;
  virtual bool Stop() = 0;

  virtual bool ValidateHostConfiguration(
      std::vector<std::string>* config_commands) const;

 protected:
  static bool UserInGroup(const std::string& group,
                          std::vector<std::string>* config_commands);
  const vsoc::CuttlefishConfig* config_;
  VmManager(const vsoc::CuttlefishConfig* config);

private:
  struct VmManagerHelper {
    // The singleton implementation
    std::function<VmManager*(const vsoc::CuttlefishConfig*)> builder;
    // Whether the host packages support this vm manager
    std::function<bool()> support_checker;
    std::function<void(vsoc::CuttlefishConfig*)> configure_boot_devices;
  };
  // Asociates a vm manager helper to every valid vm manager name
  static std::map<std::string, VmManagerHelper> vm_manager_helpers_;
};

}  // namespace vm_manager
