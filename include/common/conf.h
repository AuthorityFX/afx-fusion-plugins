// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright (C) 2012
//
// Authority FX, Inc.
// www.authorityfx.com

#ifndef CONF_H
#define CONF_H

#include <iostream>
#include <list>
#include <boost/variant/variant.hpp>

class OptionBase {
protected:
  typedef boost::variant<bool, int, float, std::string> value_variant;

  value_variant value_;

private:
  std::string name_;
  bool exists_;

public:
  OptionBase(){};
  OptionBase(std::string name);

  void create(std::string name);
  void set_name(std::string name);
  void set_exists(bool exists);

  void set_value(value_variant value);

  std::string get_name();
  bool get_exists();

  void verify_param(std::string param);
};

template <typename Type>
class Option : public OptionBase {
public:
  void find_option(std::string line, int variant_type);

  Type get_value();
};

class Conf {
private:
  typedef boost::variant<Option<bool>*, Option<int>*, Option<float>*,
                         Option<std::string>*>
      list_variant;

  std::list<list_variant> options_;

  Option<bool> omp_threading_;
  Option<int> num_omp_threads_;

public:
  Conf();

  bool omp_threading_value();
  bool omp_threading_exists();

  int num_omp_threads_value();
  bool num_omp_threads_exists();

  void parse();
};

#endif  // CONF_H
