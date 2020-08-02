// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright (C) 2012
//
// Authority FX, Inc.
// www.authorityfx.com

#ifndef _AFX_SOFTCLIP_H_
#define _AFX_SOFTCLIP_H_

#define MODULENAME "afx_softClip"
#define BaseClass ThreadedOperator
#define ThisClass AFX_SoftClip

class ThisClass : public BaseClass {
  FuDeclareClass(ThisClass, BaseClass);

  static bool pluginIsRegistered;
  static std::stringstream licenseStatus;

  Input* InClipChannel;
  Input* InMax;
  Input* InKnee;

  Input* InImage;
  Output* OutImage;

  Image* imgInput;
  Image* imgOutput;

  int mode;
  double clipMax;
  double knee;

  void processChunk(Bounds region);

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
