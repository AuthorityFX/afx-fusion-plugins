// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright (C) 2012
//
// Authority FX, Inc.
// www.authorityfx.com

#ifndef _AFX_DESATURATE_H_
#define _AFX_DESATURATE_H_

#define MODULENAME "afx_desaturate"
#define BaseClass ThreadedOperator
#define ThisClass AFX_Desaturate

class ThisClass : public BaseClass {
  FuDeclareClass(ThisClass, BaseClass);

  static bool pluginIsRegistered;
  static std::stringstream licenseStatus;

  Input* InMode;

  Input* InRed;
  Input* InYellow;
  Input* InGreen;
  Input* InCyan;
  Input* InBlue;
  Input* InMagenta;

  Input* InGain;
  Input* InLift;
  Input* InGamma;
  Input* InDesaturation;

  Input* InImage;
  Output* OutImage;

  Image* imgInput;
  Image* imgOutput;

  int width;
  int height;

  int mode;
  double RedAmount;
  double YellowAmount;
  double GreenAmount;
  double CyanAmount;
  double BlueAmount;
  double MagentaAmount;
  double gain;
  double lift;
  double gamma;
  double desaturation;

  void processChunk(Bounds region);

  bool process_GPU();
  bool process_CPU();
  void noLicenseOutput(Bounds region);

  static LicenseChecker license;

public:
  ThisClass(const Registry* reg, const ScriptVal& table, const TagList& tags);
  virtual void Process(Request* req);
  virtual void NotifyChanged(Input* in, Parameter* param, TimeStamp time,
                             TagList* tags = NULL);
  static bool RegInit(Registry* regnode);
};

#endif
