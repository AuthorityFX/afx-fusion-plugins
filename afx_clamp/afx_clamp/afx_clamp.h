// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright (C) 2012
//
// Authority FX, Inc.
// www.authorityfx.com

#ifndef _AFX_CLAMP_H_
#define _AFX_CLAMP_H_

#define MODULENAME "afx_clamp"
#define BaseClass ThreadedOperator
#define ThisClass AFX_Clamp

class ThisClass : public BaseClass {
  FuDeclareClass(ThisClass, BaseClass);

  static bool pluginIsRegistered;
  static std::stringstream licenseStatus;

  Input* InEnableRGB;
  Input* InMaxType;
  Input* InRGBMin;
  Input* InRGBMax;

  Input* InEnableAlpha;
  Input* InAlphaMin;
  Input* InAlphaMax;

  Input* InImage;
  Output* OutImage;

  Image* imgInput;
  Image* imgOutput;

  int width;
  int height;

  int mode;
  bool rgbEnabled;
  float rgbMin;
  float rgbMax;
  bool alphaEnabled;
  float alphaMin;
  float alphaMax;

  void processChunk(Bounds region);

  bool process_GPU();
  bool process_CPU();
  void noLicenseOutput(Bounds region);

public:
  ThisClass(const Registry* reg, const ScriptVal& table, const TagList& tags);
  virtual void Process(Request* req);
  virtual void NotifyChanged(Input* in, Parameter* param, TimeStamp time,
                             TagList* tags = NULL);
  static bool RegInit(Registry* regnode);
};

#endif
