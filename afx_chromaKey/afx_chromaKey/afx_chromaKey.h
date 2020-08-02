// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright (C) 2012
//
// Authority FX, Inc.
// www.authorityfx.com

#ifndef _AFX_CHROMA_KEY_H_
#define _AFX_CHROMA_KEY_H_

#define MODULENAME "afx_chromaKey"
#define BaseClass ThreadedOperator
#define ThisClass AFX_ChromaKey

class ThisClass : public ThreadedOperator {
  FuDeclareClass(ThisClass, ThreadedOperator);

  static bool pluginIsRegistered;
  static std::stringstream licenseStatus;

  Input* InImage;
  Input* InGarbageMatte;
  Input* InSpillMatte;
  Input* InScreenMatte;
  Input* InIgnoreMatte;
  Output* OutImage;

  Input* InScreenColor;
  Input* InCustomColor[3];
  Input* InScreenThreshold;
  Input* InScreenBias;
  Input* InSatRestoration;
  Input* InValRestoration;
  Input* InSoftness;
  Input* InSpillAlgorithm;
  Input* InSpillSuppression;
  Input* InExponent;
  Input* InAAThreshold;
  Input* InMatteBlur;
  Input* InSourceFrame;
  Input* InSpecifySourceFrame;

  Image* In_Img;
  Image* Source_Img;
  Image* GarbageMatte_Img;
  Image* SpillMatte_Img;
  Image* ScreenMatte_Img;
  Image* IgnoreMatte_Img;
  Image* Out_Img;

  int width;
  int height;

  float screen_color;
  float custom_color[3];
  float screen_threshold;
  float screen_bias;
  float sat_restoration;
  float val_restoration;
  float softness;
  int spill_algorithm;
  float spill_suppression;
  float exponent;
  float aa_threshold;
  float matte_blur;
  bool specify_source;
  float source_frame;

  bool process_CPU();
  void grabInput(Bounds region, ImageContainerA* input, float* ref_hsv,
                 double* sum, double* sumSqrs, unsigned int& num);
  void setOutput(Bounds region, ImageContainerA& matte, float* mean);

  void buildMatte(Bounds region, ImageContainerA& matte, ImageContainerA* input,
                  float* mean, float* stdDev);

  void RGBtoHSV(float* RGB, float* HSV);
  void HSVtoRGB(float* HSV, float* RGB);
  void RGBtoLab(float* RGB, float* Lab);

  float spillSupression(float* RGB, int algorithm);

  void noLicenseOutput(Bounds region);

public:
  ThisClass(const Registry* reg, const ScriptVal& table, const TagList& tags);
  virtual void Process(Request* req);
  virtual void NotifyChanged(Input* in, Parameter* param, TimeStamp time,
                             TagList* tags = NULL);
  static bool RegInit(Registry* regnode);
};

#endif
