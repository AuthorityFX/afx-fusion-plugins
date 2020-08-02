// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright (C) 2012
//
// Authority FX, Inc.
// www.authorityfx.com

#ifndef _AFX_GLOW_H_
#define _AFX_GLOW_H_

#define MODULENAME "afx_glow"
#define BaseClass ThreadedOperator
#define ThisClass AFX_Glow

class ThisClass : public BaseClass {
  FuDeclareClass(ThisClass, BaseClass);

  static bool pluginIsRegistered;
  static std::stringstream licenseStatus;

  Input* InImage;
  Input* InPreMask;
  Output* OutImage;

  Input* InIntensity;
  Input* InInnerSize;
  Input* InOuterSize;
  Input* InSizeExponent;
  Input* InThreshold;
  Input* InQuality;
  Input* InTint[3];
  Input* InGammaSpace;
  Input* InAlphaMultiply;
  Input* InRepeatBorders;

  Image* In_Img;
  Image* PreMask_Img;
  Image* Out_Img;

  int width;
  int height;

  float intensity;
  float inner_size;
  float outer_size;
  float falloff;
  float threshold;
  float quality;
  float tint[3];
  int gamma_space;
  bool alpha_divide;
  bool repeat_borders;

  void grabInput(Bounds region, ImageContainerA* glowSrc);
  void setOutput(Bounds region, ImageContainerA* output);
  bool process_GPU();
  bool process_CPU();
  void createGauss(Bounds region, float** gaussContainer, int* gaussSize,
                   float* sigma);
  void createKernel_CPU(Bounds region, ImageContainerA& kernel,
                        float** gaussContainer, int* gaussSize, int maxSize,
                        int kernelSize);
  void setOutputKernel(Bounds region, ImageContainerA& kernel);
  void noLicenseOutput(Bounds region);

public:
  ThisClass(const Registry* reg, const ScriptVal& table, const TagList& tags);
  virtual void Process(Request* req);
  static bool RegInit(Registry* regnode);
};

#endif
