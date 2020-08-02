// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright (C) 2012
//
// Authority FX, Inc.
// www.authorityfx.com

#ifndef _AFX_DEFOCUS_H_
#define _AFX_DEFOCUS_H_

#include "FuPlugIn.h"

#define MODULENAME "afx_defocus"
#define BaseClass ThreadedOperator
#define ThisClass AFX_Defocus

#define PI (3.141592653589793238462643383280)
#define TPI (6.283185307179586476925286766559)
#define SUPER_SAMPLE_AMOUNT (8)

class ThisClass : public BaseClass {
  FuDeclareClass(ThisClass, BaseClass);

  static bool pluginIsRegistered;
  static std::stringstream licenseStatus;

  Input* InShape;
  Input* InSize;
  Input* InRotation;
  Input* InCurvature;

  Input* InImage;
  Output* OutImage;

  Image* imgInput;
  Image* imgOutput;

  int width;
  int height;

  int shape;
  double curvature;
  double radius;
  double rotation;

  void grabInput(Bounds region, ImageContainerA* dest);
  void setOutput(Bounds region, ImageContainerA* src);

  bool process_GPU();
  bool process_CPU();
  void noLicenseOutput(Bounds region);

  static LicenseChecker license;

public:
  ThisClass(const Registry* reg, const ScriptVal& table, const TagList& tags);

  virtual void Process(Request* req);
  static bool RegInit(Registry* regnode);
};

#endif
