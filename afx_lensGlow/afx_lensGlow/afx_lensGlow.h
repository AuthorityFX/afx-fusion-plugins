// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright (C) 2012
//
// Authority FX, Inc.
// www.authorityfx.com

#ifndef _AFX_LENSGLOW_H_
#define _AFX_LENSGLOW_H_

#define MODULENAME "afx_lensGlow"
#define BaseClass ThreadedOperator
#define ThisClass AFX_LensGlow

class ThisClass : public BaseClass {
  FuDeclareClass(ThisClass, BaseClass);

  static bool pluginIsRegistered;
  static std::stringstream licenseStatus;

  Input* InIntensity;
  Input* InGlowSize;
  Input* InLockXY;
  Input* InXSize;
  Input* InYSize;
  Input* InGlowSoftness;
  Input* InRedScale;
  Input* InGreenScale;
  Input* InBlueScale;
  Input* InThreshold;
  Input* InCenterKernel;
  Input* InGammaSpace;
  Input* InRepeatBorders;

  Input* InImage;
  Input* InPreMask;
  Output* OutImage;

  Image* inImg;
  Image* PreMask_Img;
  Image* outImg;

  int width;
  int height;

  float intensity;
  float glow_size;
  float sizeX;
  float sizeY;
  bool lock_XY;
  float glow_softness;
  float red_scale;
  float green_scale;
  float blue_scale;
  float threshold;
  bool center_kernel;
  int gamma_space;
  bool repeat_borders;

  void clampOutput(Bounds region);
  void grabInput(Bounds region, ImageContainerA* dest,
                 ImageContainerA* maskedDest);
  void setOutput(Bounds region, ImageContainerA* output, ImageContainerA* src);
  bool process_CPU();
  bool process_GPU();
  void softenGlow(ImageContainerA& input, ImageContainerA& output,
                  float blurSigma);
  void applyConvolution(ImageContainerA& input, ImageContainerA& kernel,
                        ImageContainerA& output);
  void extendGlowSourceBorders(ImageContainerA& input, ImageContainerA& output,
                               IppiSize kernelSize);
  void recenterKernel(ImageContainerA& input, ImageContainerA& output,
                      int paddingLeft, int paddingTop);
  void resizeKernel(ImageContainerA& input, ImageContainerA& output);
  void noLicenseOutput(Bounds region);

public:
  ThisClass(const Registry* reg, const ScriptVal& table, const TagList& tags);
  virtual void Process(Request* req);
  virtual void NotifyChanged(Input* in, Parameter* param, TimeStamp time,
                             TagList* tags = NULL);
  static bool RegInit(Registry* regnode);
};

#endif
