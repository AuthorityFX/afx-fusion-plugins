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

#define MODULENAME "afx_zdefocus"
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
  Input* InDistance;

  Input* InTiltHoriz;
  Input* InTiltVert;

  Input* InQuality;
  Input* InDistribution;

  Input* InDepthMapChannel;
  Input* InDepthMapRangeLow;
  Input* InDepthMapRangeHigh;

  Input* InLayerInter;
  Input* InLockLayers;
  Input* InOuputMatte;

  Input* InImage;
  Input* InDepthMap;
  Output* OutImage;

  Image* imgInput;
  Image* imgDepthMap;
  Image* imgOutput;

  bool outputMatte;
  int shape;
  double curvature;
  double radius;
  double rotation;
  double distance;
  double density;
  int quality;
  bool lockLayers;
  bool layerInterpolation;
  float horizTilt;
  float vertTilt;
  int zChannel;

  double depthMapRangeLow;
  double depthMapRangeHigh;

  int width;
  int height;

  void grabInput(Bounds region, ImageContainerA* dest,
                 double lowerLayerThreshold, double upperLayerThreshold,
                 int layerNum, bool* layerContainsAnything);
  void setOutput(Bounds region, ImageContainerA* src, ImageContainerA* matte);
  void boxBlurHoriz(Bounds region, ImageContainerA* input,
                    ImageContainerA* output, int blurSize);
  void boxBlurVert(Bounds region, ImageContainerA* input,
                   ImageContainerA* output, int blurSize);
  void createKernel(ImageContainerA& kernel, double circumradius,
                    double curvature, double rotation, int shape);
  void blendLayerToOutput(Bounds region, ImageContainerA* layer,
                          ImageContainerA* output, ImageContainerA* matte,
                          float kernelSize, int layerNum);

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
