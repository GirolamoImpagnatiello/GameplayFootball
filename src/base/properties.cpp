// Copyright 2019 Google LLC & Bastiaan Konings
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// written by bastiaan konings schuiling 2008 - 2014
// this work is public domain. the code is undocumented, scruffy, untested, and should generally not be used for anything important.
// i do not offer support, so don't ask. to be used for inspiration :)

#include "properties.hpp"

#include "defines.hpp"
#include "utils.hpp"
#include "log.hpp"

#include <fstream>
#include <iostream>
#include <set>

using namespace std;

namespace blunted {

  std::string Properties::emptyString = "";

  Properties::Properties() {
  }

  Properties::~Properties() {
  }

  bool Properties::Exists(const char *name) const {
    auto iter = properties.find(std::string(name));
    if (iter == properties.end()) {
      return false;
    } else {
      return true;
    }
  }

  void Properties::Set(const char *name, const std::string &value) {
    properties[std::string(name)] = value;
    }

  void Properties::SetInt(const char *name, int value) {
    std::string value_str = int_to_str(value);
    Set(name, value_str);
  }

  void Properties::Set(const char *name, real value) {
    std::string value_str = real_to_str(value);
    Set(name, value_str);
  }

  void Properties::SetBool(const char *name, bool value) {
    std::string value_str = value ? "true" : "false";
    Set(name, value_str);
  }

  const std::string &Properties::Get(const char *name, const std::string &defaultValue) const {
    auto iter = properties.find(std::string(name));
    if (iter == properties.end()) {
      return defaultValue;
    } else {
      return iter->second;
    }
  }

  bool Properties::GetBool(const char *name, bool defaultValue) const {
    auto iter = properties.find(std::string(name));
    if (iter == properties.end()) {
      return defaultValue;
    } else {
      if (iter->second.compare("true") == 0) return true; else return false;
    }
  }

  real Properties::GetReal(const char *name, real defaultValue) const {
    auto iter = properties.find(std::string(name));
    if (iter == properties.end()) {
      return defaultValue;
    } else {
      return atof(iter->second.c_str());
    }
  }

  int Properties::GetInt(const char *name, int defaultValue) const {
    auto iter = properties.find(std::string(name));
    if (iter == properties.end()) {
      return defaultValue;
    } else {
      return int(floor(atof(iter->second.c_str())));
    }
  }

  Vector3 Properties::GetVector3(const char *name, Vector3 defaultValue) const {
    if (Exists(name)) {
      const std::string &str = Get(name);
      return GetVectorFromString(str);
    } else {
      return defaultValue;
    }
  }

  void Properties::AddProperties(const Properties *userprops) {
    if (!userprops) return;

    const map_Properties *userpropdata = userprops->GetProperties();
    map_Properties::const_iterator iter = userpropdata->begin();

    while (iter != userpropdata->end()) {
      properties[iter->first] = iter->second;
      iter++;
    }
  }

  void Properties::AddProperties(const Properties &userprops) {
    const map_Properties *userpropdata = userprops.GetProperties();
    map_Properties::const_iterator iter = userpropdata->begin();

    while (iter != userpropdata->end()) {
      properties[iter->first] = iter->second;
      iter++;
    }
  }

  const map_Properties *Properties::GetProperties() const {
    return &properties;
  }

  void Properties::LoadFile(const std::string &filename) {
    std::vector <std::string> content;
    file_to_vector(filename, content);

    int contentSize = content.size();
    for (int i = 0; i < contentSize; i++) {
      int param_start = content.at(i).find_first_of('"') + 1;
      int param_end = content.at(i).find_first_of('"', param_start);
      int value_start = content.at(i).find_first_of('"', param_end + 1) + 1;
      int value_end = content.at(i).find_first_of('"', value_start);

      std::string param, value;

      if (param_start < param_end && value_start < value_end && param_end < value_start) {
        param = content.at(i).substr(param_start, param_end - param_start);
        value = content.at(i).substr(value_start, value_end - value_start);
        this->Set(param.c_str(), value);
      }
    }
  }

  void Properties::SaveFile(const std::string &filename) const {
    // Keep the hand-written sections when settings are saved from the menu.
    // Newly created settings are appended after the existing file.
    ifstream existing(filename.c_str());
    std::vector<std::string> lines;
    std::set<std::string> written;
    std::string line;
    while (std::getline(existing, line)) {
      const size_t firstQuote = line.find('"');
      const size_t secondQuote = firstQuote == std::string::npos ? std::string::npos : line.find('"', firstQuote + 1);
      if (firstQuote != std::string::npos && secondQuote != std::string::npos) {
        const std::string key = line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
        map_Properties::const_iterator found = properties.find(key);
        if (found != properties.end()) {
          if (written.insert(key).second) lines.push_back("\"" + key + "\" \"" + found->second + "\"");
          continue;
        }
      }
      lines.push_back(line);
    }
    existing.close();

    ofstream cfile(filename.c_str(), ios::out);
    for (size_t i = 0; i < lines.size(); ++i) cfile << lines[i] << '\n';
    for (map_Properties::const_iterator iter = properties.begin(); iter != properties.end(); ++iter) {
      if (written.count(iter->first) == 0) cfile << "\"" << iter->first << "\" \"" << iter->second << "\"\n";
    }
  }

  void Properties::Print() const {
    map_Properties::const_iterator iter = properties.begin();
    while (iter != properties.end()) {
      std::string bla = "\"" + iter->first + "\" \"" + iter->second + "\"\n";
      cout << bla.c_str();

      iter++;
    }
  }

}
