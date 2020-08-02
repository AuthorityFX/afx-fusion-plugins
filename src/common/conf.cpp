// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright (C) 2012
//
// Authority FX, Inc.
// www.authorityfx.com

#include "conf.h"

#include <fstream>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/variant.hpp>
#include <exception>
#include <typeinfo>

/**************************
BASE  OptionBase definition
**************************/

OptionBase::OptionBase(std::string name) { create(""); }

void OptionBase::create(std::string name)
{
  name_   = name;
  exists_ = false;
}

void OptionBase::set_exists(bool exists) { exists_ = exists; }

void OptionBase::set_value(value_variant value) { value_ = value; }

std::string OptionBase::get_name() { return name_; }

bool OptionBase::get_exists() { return exists_; }

/**************************
 template Option template definition
***************************/

template <typename Type>
void Option<Type>::find_option(std::string line, int variant_type)
{
  // Conver to upper case
  boost::to_upper(line);
  // find pos of options
  size_t pos = line.find(get_name());

  // If option exists
  if (pos != std::string::npos) {
    // substring option param
    std::string param = line.substr(pos + get_name().length() + 1,
                                    line.length() - get_name().length() - 1);

    // If type is bool
    switch (variant_type) {
      case 0:

        if (param.find("TRUE") != std::string::npos ||
            param.find("ON") != std::string::npos) {
          set_value(true);
          set_exists(true);
        }
        else if (param.find("FALSE") != std::string::npos ||
                 param.find("OFF") != std::string::npos) {
          set_value(false);
          set_exists(true);
        }
        break;

      case 1:

        try {
          int value = boost::lexical_cast<int>(param);
          set_value(value);
          set_exists(true);
        }
        catch (std::exception& e) {
        }
        break;

      case 2:

        try {
          float value = boost::lexical_cast<float>(param);
          set_value(value);
          set_exists(true);
        }
        catch (std::exception& e) {
        }
        break;

      case 3:

      {
        set_value(param);
        set_exists(true);
      } break;
    }
  }
}

template <typename Type>
Type Option<Type>::get_value()
{
  if (this->get_exists() == true) {
    return boost::get<Type>(value_);
  }
  else {
    return boost::lexical_cast<Type>(0);
  }
}

/**************************
Conf definition
***************************/

Conf::Conf()
{
  omp_threading_.create("OMP_THREADING");
  num_omp_threads_.create("NUM_OMP_THREADS");

  options_.push_back(&omp_threading_);
  options_.push_back(&num_omp_threads_);
}

void Conf::parse()
{
  std::ifstream conf_file;
  // Open conf file
  conf_file.open("../afx-plugins.conf", std::ifstream::in);

  if (conf_file.is_open()) {
    // Read file
    char line[50];
    while (conf_file.getline(line, 50)) {
      // Loop through bool list
      for (std::list<list_variant>::iterator it = options_.begin();
           it != options_.end(); it++) {
        int variant_type = it->which();
        switch (it->which()) {
          case 0:
            boost::get<Option<bool>*>(*it)->find_option(line, variant_type);
            break;
          case 1:
            boost::get<Option<int>*>(*it)->find_option(line, variant_type);
            break;
          case 2:
            boost::get<Option<float>*>(*it)->find_option(line, variant_type);
            break;
          case 3:
            boost::get<Option<std::string>*>(*it)->find_option(line,
                                                               variant_type);
            break;
        }
      }
    }
  }
}

bool Conf::omp_threading_value() { return omp_threading_.get_value(); }

bool Conf::omp_threading_exists() { return omp_threading_.get_exists(); }

int Conf::num_omp_threads_value() { return num_omp_threads_.get_value(); }

bool Conf::num_omp_threads_exists() { return num_omp_threads_.get_exists(); }
