// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright (C) 2012
//
// Authority FX, Inc.
// www.authorityfx.com

#ifndef LICENSECHECKER_H
#define LICENSECHECKER_H

#include <iostream>
#include <list>
#include <string>

enum LicenseResult {
  LR_GOOD         = 0,
  LR_NOT_LICENSED = -1,
  LR_NO_FILE      = -2,
  LR_CHECK_UUID1  = -3,
  LR_CHECK_UUID2  = -4,
  LR_ERROR        = -5,
};

enum LicenseType { L_WORKSTATION, L_RENDER, L_TRIAL };

class Plugin {
private:
  std::string _name;
  LicenseType _type;
  int _count;
  bool _floating;

public:
  Plugin(){};
  Plugin(std::string name, LicenseType type, int count, bool floating);

  void set_name(std::string name);
  void set_type(LicenseType type);
  void set_count(int count);
  void set_floating(bool floating);

  std::string get_name();
  LicenseType get_type();
  int get_count();
  bool get_floating();
};

class LicenseChecker {
private:
  std::list<Plugin> _plugins_list;

  std::string _uuid1;
  std::string _uuid2;

  int parse_license(std::string license);

public:
  LicenseChecker();

  LicenseResult decrypt_license();
  LicenseResult check_license(std::string name, LicenseType type);

  // Add more functions here to handle floating licenses
};

#endif  // LICENSECHECKER_H
