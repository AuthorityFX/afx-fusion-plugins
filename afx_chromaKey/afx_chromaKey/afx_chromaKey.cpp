// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright (C) 2012
//
// Authority FX, Inc.
// www.authorityfx.com

#include "stdafx.h"
#include "afx_chromaKey.h"

#define pluginGuid ("com.authorityfx:chromaKeyPlugin")
#define pluginName ("AFX Chroma Key")
#define PLUGIN_LICENSE_NAME ("chroma_key")

bool ThisClass::pluginIsRegistered;
std::stringstream ThisClass::licenseStatus;

enum suppresion_method { light = 0, medium = 1, hard = 2, extreme = 3 };

// Boost thread mutex
boost::mutex _threadLock;

FuRegisterClass(COMPANY_ID ".afx_chromaKey", CT_Tool)
  //REG_TileID, AFX_Convolve_Tile,
  REGS_Name, pluginName,
  REGS_Category, "Authority FX",
  REGS_OpIconString, "AKey",
  REGS_OpDescription, "Inteligent chroma keyer for best possible mattes.",
  REGS_VersionString, "Version 1.1",
  REGS_Company, "Authority FX, Inc.",
  REGS_URL, "http://www.authorityfx.com/",
  REG_NoObjMatCtrls, TRUE,
  REG_NoMotionBlurCtrls, TRUE,
  REG_NoChannelCtrls, TRUE,
  TAG_DONE);

bool ThisClass::RegInit(Registry* regnode)
{
  LicenseType appType;
  // App type. Render or GUI
  FusionApp* app = GetApp();
  if (app->GetAttrB(PA_IsEngine))
    appType = L_RENDER;
  else if (app->GetAttrB(PA_IsPost))
    appType = L_WORKSTATION;
  else
    appType = L_WORKSTATION;

  pluginIsRegistered = false;

  // Create license object
  LicenseChecker checker;
  // Decrypt
  LicenseResult status = checker.decrypt_license();

  if (status == LR_GOOD) {
    status = checker.check_license(PLUGIN_LICENSE_NAME, appType);
    if (status == LR_GOOD) {
      pluginIsRegistered = true;
    }
  }

  if (status != LR_GOOD) {
    switch (status) {
      case LR_NO_FILE:
        licenseStatus << pluginName << ": no license file." << std::endl;
        break;
      case LR_NOT_LICENSED:
        licenseStatus << pluginName << ": plugin is not licensed." << std::endl;
        break;
      case LR_ERROR:
        licenseStatus << pluginName << ": licensing error." << std::endl;
        break;
    }
  }

  // Initialize IPP threading
  ippInit();

  return BaseClass::RegInit(regnode);
}

ThisClass::ThisClass(const Registry* reg, const ScriptVal& table,
                     const TagList& tags)
    : BaseClass(reg, table, tags)
{
  InScreenColor = AddInput(
      "Screen Color", "screen_color", LINKID_DataType, CLSID_DataType_Number,
      INPID_InputControl, MULTIBUTTONCONTROL_ID, MBTNC_AddButton, "Green",
      MBTNC_AddButton, "Blue", MBTNC_AddButton, "Custom", MBTNC_StretchToFit,
      TRUE, INP_DoNotifyChanged, TRUE, INP_Default, 0.0, TAG_DONE);

  InCustomColor[0] = AddInput(
      "Red", "red", LINKID_DataType, CLSID_DataType_Number, INPID_InputControl,
      COLORCONTROL_ID, ICS_Name, "Custom Color", IC_ControlGroup, 1,
      IC_ControlID, CHAN_RED, INP_MinAllowed, 0.0, INP_MaxAllowed, 50.0,
      INP_MinScale, 0.0, INP_MaxScale, 1.0, INP_Default, 0.0, TAG_DONE);

  InCustomColor[1] = AddInput(
      "Green", "green", LINKID_DataType, CLSID_DataType_Number,
      INPID_InputControl, COLORCONTROL_ID, IC_ControlGroup, 1, IC_ControlID,
      CHAN_GREEN, INP_MinAllowed, 0.0, INP_MaxAllowed, 50.0, INP_MinScale, 0.0,
      INP_MaxScale, 1.0, INP_Default, 1.0, TAG_DONE);

  InCustomColor[2] = AddInput(
      "Blue", "blue", LINKID_DataType, CLSID_DataType_Number,
      INPID_InputControl, COLORCONTROL_ID, IC_ControlGroup, 1, IC_ControlID,
      CHAN_BLUE, INP_MinAllowed, 0.0, INP_MaxAllowed, 50.0, INP_MinScale, 0.0,
      INP_MaxScale, 1.0, INP_Default, 0.0, TAG_DONE);

  InScreenThreshold =
      AddInput("Screen Threshold", "screen_threshold", LINKID_DataType,
               CLSID_DataType_Number, INPID_InputControl, SLIDERCONTROL_ID,
               INP_Default, 1.0, INP_MinAllowed, 0.0, INP_MaxAllowed, 100.0,
               INP_MinScale, 0.0, INP_MaxScale, 2.0, TAG_DONE);

  InValRestoration =
      AddInput("Shadow Restoration", "shadow_restoration", LINKID_DataType,
               CLSID_DataType_Number, INPID_InputControl, SLIDERCONTROL_ID,
               INP_Default, 0.1, INP_MinAllowed, 0.0, INP_MaxAllowed, 100.0,
               INP_MinScale, 0.0, INP_MaxScale, 2.0, TAG_DONE);

  InSatRestoration =
      AddInput("Gray Restoration", "gray_restoration", LINKID_DataType,
               CLSID_DataType_Number, INPID_InputControl, SLIDERCONTROL_ID,
               INP_Default, 1.0, INP_MinAllowed, 0.0, INP_MaxAllowed, 100.0,
               INP_MinScale, 0.0, INP_MaxScale, 2.0, TAG_DONE);

  InSoftness = AddInput(
      "Softness", "softness", LINKID_DataType, CLSID_DataType_Number,
      INPID_InputControl, SLIDERCONTROL_ID, INP_Default, 0.5, INP_MinAllowed,
      0.0, INP_MaxAllowed, 1.0, INP_MinScale, 0.0, INP_MaxScale, 1.0, TAG_DONE);

  InExponent =
      AddInput("Cleanup", "Cleanup", LINKID_DataType, CLSID_DataType_Number,
               INPID_InputControl, SLIDERCONTROL_ID, INP_Default, 2.0,
               INP_MinAllowed, 0.0, INP_MaxAllowed, 50.0, INP_MinScale, 0.0,
               INP_MaxScale, 5.0, TAG_DONE);

  InSpillAlgorithm =
      AddInput("Spill Algorithm", "spill_algorithm", LINKID_DataType,
               CLSID_DataType_Number, INPID_InputControl, COMBOCONTROL_ID,
               CCS_AddString, "Light", CCS_AddString, "Medium", CCS_AddString,
               "Hard", CCS_AddString, "Extreme", TAG_DONE);

  InSpillSuppression =
      AddInput("Spill Suppression", "spill_suppression", LINKID_DataType,
               CLSID_DataType_Number, INPID_InputControl, SLIDERCONTROL_ID,
               INP_Default, 1.0, INP_MinAllowed, 0.0, INP_MaxAllowed, 1.0,
               INP_MinScale, 0.0, INP_MaxScale, 1.0, TAG_DONE);

  InAAThreshold =
      AddInput("Anti-Aliasing Threshold", "aa_threshold", LINKID_DataType,
               CLSID_DataType_Number, INPID_InputControl, SLIDERCONTROL_ID,
               INP_Default, 0.1, INP_MinAllowed, 0.01, INP_MaxAllowed, 1.0,
               INP_MinScale, 0.0, INP_MaxScale, 1.0, TAG_DONE);

  InMatteBlur = AddInput(
      "Matte Blur", "matte_blur", LINKID_DataType, CLSID_DataType_Number,
      INPID_InputControl, SLIDERCONTROL_ID, INP_Default, 0.0, INP_MinAllowed,
      0.0, INP_MaxAllowed, 1.0, INP_MinScale, 0.0, INP_MaxScale, 1.0, TAG_DONE);

  InSpecifySourceFrame =
      AddInput("Specify Source Frame", "specify_source_frame", LINKID_DataType,
               CLSID_DataType_Number, INPID_InputControl, CHECKBOXCONTROL_ID,
               INP_Default, 0.0, INP_DoNotifyChanged, TRUE, TAG_DONE);

  InSourceFrame = AddInput("Source Frame", "source_frame", LINKID_DataType,
                           CLSID_DataType_Number, INPID_InputControl,
                           SLIDERCONTROL_ID, INP_Default, 1.0, INP_MinScale,
                           0.0, INP_MaxScale, 50.0, TAG_DONE);

  // Garbage-matte
  InGarbageMatte = AddInput("Garbage Matte", "garbage_matte", LINKID_DataType,
                            CLSID_DataType_Mask, INP_Priority, MASK_PRI,
                            INP_Required, FALSE, TAG_DONE);

  // Spill-matte
  InSpillMatte = AddInput("Spill Matte", "spill_matte", LINKID_DataType,
                          CLSID_DataType_Mask, INP_Priority, MASK_PRI,
                          INP_Required, FALSE, TAG_DONE);

  // Screen-matte
  InScreenMatte = AddInput("Screen Matte", "screen_matte", LINKID_DataType,
                           CLSID_DataType_Mask, INP_Priority, MASK_PRI,
                           INP_Required, FALSE, TAG_DONE);

  // Ignore-matte
  InIgnoreMatte = AddInput("Ignore Matte", "ignore_matte", LINKID_DataType,
                           CLSID_DataType_Mask, INP_Priority, MASK_PRI,
                           INP_Required, FALSE, TAG_DONE);

  // Input image
  InImage = AddInput("Input", "input", LINKID_DataType, CLSID_DataType_Image,
                     LINK_Main, 1, TAG_DONE);

  // Output image
  OutImage = AddOutput("Output", "output", LINKID_DataType,
                       CLSID_DataType_Image, LINK_Main, 1, TAG_DONE);
}

void ThisClass::Process(Request* req)
{
  screen_color = *InScreenColor->GetValue(req);
  for (int i = 0; i < 3; i++)
    custom_color[i] = *InCustomColor[i]->GetValue(req);
  screen_threshold = *InScreenThreshold->GetValue(req);
  sat_restoration  = *InSatRestoration->GetValue(req);
  val_restoration  = *InValRestoration->GetValue(req);
  softness         = *InSoftness->GetValue(req);

  float spillComboBoxValue = *InSpillAlgorithm->GetValue(req);

  if (spillComboBoxValue <= 0.5)
    spill_algorithm = light;
  else if (spillComboBoxValue <= 1.5)
    spill_algorithm = medium;
  else if (spillComboBoxValue <= 2.5)
    spill_algorithm = hard;
  else if (spillComboBoxValue <= 3.5)
    spill_algorithm = extreme;
  else
    spill_algorithm = hard;

  spill_suppression = *InSpillSuppression->GetValue(req);
  exponent          = *InExponent->GetValue(req);
  aa_threshold      = *InAAThreshold->GetValue(req);
  matte_blur        = *InMatteBlur->GetValue(req);
  specify_source    = *InSpecifySourceFrame->GetValue(req) >= 0.5;
  source_frame      = *InSourceFrame->GetValue(req);

  // Get images
  In_Img = (Image*)InImage->GetValue(req);

  if (specify_source) {
    Source_Img = 0;
    try {
      Source_Img = (Image*)InImage->GetSourceAttrs(
          source_frame, NULL, REQF_SecondaryTime, REQF_SecondaryTime);
      if (Source_Img == 0)
        throw true;
    }
    catch (bool err) {
      if (err)
        Source_Img = (Image*)InImage->GetValue(req);
    }

    ScreenMatte_Img = 0;
    try {
      ScreenMatte_Img = (Image*)InScreenMatte->GetSourceAttrs(
          source_frame, NULL, REQF_SecondaryTime, REQF_SecondaryTime);
      if (ScreenMatte_Img == 0)
        throw true;
    }
    catch (bool err) {
      if (err)
        ScreenMatte_Img = (Image*)InScreenMatte->GetValue(req);
    }
  }
  else {
    Source_Img      = (Image*)InImage->GetValue(req);
    ScreenMatte_Img = (Image*)InScreenMatte->GetValue(req);
  }

  GarbageMatte_Img = (Image*)InGarbageMatte->GetValue(req);
  SpillMatte_Img   = (Image*)InSpillMatte->GetValue(req);
  ScreenMatte_Img  = (Image*)InScreenMatte->GetValue(req);
  IgnoreMatte_Img  = (Image*)InIgnoreMatte->GetValue(req);

  width  = In_Img->Width;
  height = In_Img->Height;

  // Create Out_Img
  Out_Img = NewImage(IMG_Like, In_Img, TAG_DONE);

  if (pluginIsRegistered == true) {
    if (process_CPU()) {
    }
  }
  else {
    // Not licensed - output red
    Document->PrintF(ECONID_Error, "%s\n", licenseStatus.str().c_str());
    {
      Multithreader threader(Bounds(0, 0, width - 1, height - 1));
      threader.processInChunks(
          boost::bind(&ThisClass::noLicenseOutput, this, _1));
    }
  }

  OutImage->Set(req, Out_Img);
}

bool ThisClass::process_CPU()
{
  ImageContainerA matte;
  matte.create_image(width, height);

  ImageContainerA input[3];
  for (int i = 0; i < 3; i++) input[i].create_image(width, height);

  float ref_rgb[3];
  float ref_hsv[3];

  if (screen_color <= 0.5) {
    ref_rgb[0] = 0.0f;
    ref_rgb[1] = 1.0f;
    ref_rgb[2] = 0.0f;
  }
  else if (screen_color <= 1.5) {
    ref_rgb[0] = 0.0f;
    ref_rgb[1] = 0.0f;
    ref_rgb[2] = 1.0f;
  }
  else if (screen_color <= 2.5) {
    for (int i = 0; i < 3; i++) ref_rgb[i] = custom_color[i];
  }

  RGBtoHSV(ref_rgb, ref_hsv);

  double sum[3]     = {0, 0, 0};
  double sumSqrs[3] = {0, 0, 0};
  unsigned int num  = 0;

  {
    // Get input
    Multithreader threader(Bounds(0, 0, width - 1, height - 1));
    threader.processInChunks(boost::bind(&ThisClass::grabInput, this, _1, input,
                                         ref_hsv, sum, sumSqrs,
                                         boost::ref(num)));
  }

  float mean[3];
  float stdDev[3];

  if (num == 0)
    num = 1;

  for (int i = 0; i < 3; i++) {
    mean[i]   = sum[i] / num;
    stdDev[i] = sqrt((sumSqrs[i] - pow(sum[i], 2.0) / num) / num);
  }

  {
    // Build matte
    Multithreader threader(Bounds(0, 0, width - 1, height - 1));
    threader.processInChunks(boost::bind(&ThisClass::buildMatte, this, _1,
                                         boost::ref(matte), input, mean,
                                         stdDev));
  }

  // Anti-Alias matte
  ImageContainerA matte_aa(width, height);
  {
    MorphAA mlaa;
    mlaa.doAA_A(matte, matte_aa, aa_threshold);
  }

  // Filter matte with gaussian
  ImageContainerA matte_soft;
  if (matte_blur > 0) {
    float sigma    = matte_blur * min(width, height) / 192.0f;
    int kernelSize = (int)floor(sigma * 6.0f) | 3;
    int bufSize;
    ippiFilterGaussGetBufferSize_32f_C1R(matte.getSize(), kernelSize, &bufSize);
    Ipp8u* buf;
    buf = (Ipp8u*)ippMalloc(sizeof(Ipp8u) * bufSize);
    matte_soft.create_image(width, height);
    ippiFilterGaussBorder_32f_C1R(
        matte_aa.getImagePtr(), matte_aa.getRowSizeBytes(),
        matte_soft.getImagePtr(), matte_soft.getRowSizeBytes(),
        matte_aa.getSize(), kernelSize, sigma, ippBorderRepl, 0.0f, buf);
    ippFree(buf);
  }

  {
    // Output to host
    Multithreader threader(Bounds(0, 0, width - 1, height - 1));
    threader.processInChunks(
        boost::bind(&ThisClass::setOutput, this, _1,
                    boost::ref(matte_blur > 0 ? matte_soft : matte_aa), mean));
  }

  return true;
}

void ThisClass::grabInput(Bounds region, ImageContainerA* input, float* ref_hsv,
                          double* sum, double* sumSqrs, unsigned int& num)
{
  bool screenMatteExists = (ScreenMatte_Img != 0);

  PixPtr ip(In_Img);
  PixPtr in_source(Source_Img);
  PixPtr sp(screenMatteExists ? ScreenMatte_Img : In_Img);
  FltPixel p;
  FltPixel source;
  FltPixel s;

  float color[3];

  float hsv[3];

  double l_sum[3]     = {0, 0, 0};
  double l_sumSqrs[3] = {0, 0, 0};
  unsigned int l_num  = 0;

  float* p_r_input = 0;
  float* p_g_input = 0;
  float* p_b_input = 0;

  for (int y = region.y1; y <= region.y2; y++) {
    ip.GotoXY(region.x1, y);
    in_source.GotoXY(region.x1, y);
    if (screenMatteExists)
      sp.GotoXY(region.x1, y);

    p_r_input = (float*)&input[0].pixel_at(region.x1, y);
    p_g_input = (float*)&input[1].pixel_at(region.x1, y);
    p_b_input = (float*)&input[2].pixel_at(region.x1, y);

    for (int x = region.x1; x <= region.x2; x++) {
      p >>= ip;
      source >>= in_source;
      if (screenMatteExists)
        s >>= sp;

      float screenMatte = screenMatteExists ? s.A : 1.0f;

      color[0] = p.R;
      color[1] = p.G;
      color[2] = p.B;

      // Convert HSV and convert pixel to HSV
      RGBtoHSV(color, hsv);

      // Copy to input
      *p_r_input = hsv[0];
      *p_g_input = hsv[1];
      *p_b_input = hsv[2];

      color[0] = source.R;
      color[1] = source.G;
      color[2] = source.B;

      // Convert HSV and convert pixel to HSV
      RGBtoHSV(color, hsv);

      // Center hue about 0.5 to allow for wrap around hues
      float centered_hue = fmod(hsv[0] - ref_hsv[0] + 0.5f, 1.0f);
      if (centered_hue < 0)
        centered_hue += 1.0f;

      // Compare difference of centered hue and 0.5
      if (abs(centered_hue - 0.5f) < 0.1 && hsv[1] > 0.1 && hsv[2] > 0.1 &&
          screenMatte > 0) {
        l_num++;

        for (int i = 0; i < 3; i++) {
          l_sum[i] += hsv[i];
          l_sumSqrs[i] += pow(hsv[i], 2.0f);
        }
      }

      p_r_input++;
      p_g_input++;
      p_b_input++;
    }
  }

  // Lock mutex
  boost::mutex::scoped_lock lock(_threadLock);

  // Merge local variables to main variables

  num += l_num;

  for (int i = 0; i < 3; i++) {
    sumSqrs[i] += l_sumSqrs[i];
    sum[i] += l_sum[i];
  }

  lock.unlock();
}

void ThisClass::setOutput(Bounds region, ImageContainerA& matte, float* mean)
{
  bool garbageMatteExists = (GarbageMatte_Img != 0);
  bool spillMatteExists   = (SpillMatte_Img != 0);
  bool ignoreMatteExists  = (IgnoreMatte_Img != 0);
  PixPtr mp(garbageMatteExists ? GarbageMatte_Img : In_Img);
  PixPtr sp(spillMatteExists ? SpillMatte_Img : In_Img);
  PixPtr igp(ignoreMatteExists ? IgnoreMatte_Img : In_Img);
  PixPtr ip(In_Img);
  PixPtr op(Out_Img);
  FltPixel p;
  FltPixel m;
  FltPixel s;
  FltPixel i;

  float* a = 0;

  for (int y = region.y1; y <= region.y2; y++) {
    ip.GotoXY(region.x1, y);
    op.GotoXY(region.x1, y);
    if (garbageMatteExists)
      mp.GotoXY(region.x1, y);
    if (spillMatteExists)
      sp.GotoXY(region.x1, y);
    if (ignoreMatteExists)
      igp.GotoXY(region.x1, y);

    a = (float*)&matte.pixel_at(region.x1, y);

    for (int x = region.x1; x <= region.x2; x++) {
      p >>= ip;
      if (garbageMatteExists)
        m >>= mp;
      if (spillMatteExists)
        s >>= sp;
      if (ignoreMatteExists)
        i >>= igp;

      float garbageMatte = garbageMatteExists ? (1.0f - m.A) : 1.0f;
      float spillMatte   = spillMatteExists ? s.A : 1.0f;
      float ignoreMatte  = ignoreMatteExists ? i.A : 0.0f;

      float green = p.G;
      float blue  = p.B;

      float RGB[3];
      float HSV[3];

      RGB[0] = p.R;
      RGB[1] = p.G;
      RGB[2] = p.B;

      if (spill_suppression > 0 && (*a) > 0 && spillMatte > 0) {
        RGBtoHSV(RGB, HSV);

        HSV[0] = fmod((double)HSV[0] - (double)mean[0] + (1.0 / 3.0), 1.0);
        if (HSV[0] < 0)
          HSV[0] += 1.0f;

        HSVtoRGB(HSV, RGB);

        RGB[1] = RGB[1] - spill_suppression * spillMatte *
                              spillSupression(RGB, spill_algorithm);

        RGBtoHSV(RGB, HSV);

        HSV[0] = fmod((double)HSV[0] + (double)mean[0] - (1.0 / 3.0), 1.0);
        if (HSV[0] < 0)
          HSV[0] += 1.0f;

        HSVtoRGB(HSV, RGB);

        /*

        if (screen_color <= 0.5)
        {
                float average = (RGB[0] + RGB[2]) / 2.0f;
                if ( RGB[1] > average)
                        RGB[1] = RGB[1] - spill_suppression * spillMatte *
        spillSupression(RGB, spill_algorithm);
        }
        else if (screen_color <= 1.5)
        {
                float average = (RGB[0] + RGB[1]) / 2.0f;
                if ( RGB[2] > average)
                        RGB[1] = RGB[1] - spill_suppression * spillMatte *
        spillSupression(RGB, spill_algorithm);
        }

        */
      }

      float alpha = ignoreMatte + (1 - ignoreMatte) * (*a) * garbageMatte;

      p.R = RGB[0] * alpha;
      p.G = RGB[1] * alpha;
      p.B = RGB[2] * alpha;

      p.A = alpha;

      op >>= p;
      a++;
    }
  }
}

void ThisClass::buildMatte(Bounds region, ImageContainerA& matte,
                           ImageContainerA* input, float* mean, float* stdDev)
{
  float* m = 0;

  float hsv[3];

  const float PI = 3.14159265358979323846f;

  for (int y = region.y1; y <= region.y2; y++) {
    m = (float*)&matte.pixel_at(region.x1, y);

    for (int x = region.x1; x <= region.x2; x++) {
      for (int i = 0; i < 3; i++) {
        hsv[i] = input[i].pixel_at(x, y);
      }

      float matte;

      // Center hue about 0.5 to allow for wrap around hues
      float hue = fmod(hsv[0] - mean[0] + 0.5f, 1.0f);
      if (hue < 0)
        hue += 1.0f;

      float r =
          sqrt(pow(abs((hue - 0.5f) / (screen_threshold * 6.0f * stdDev[0])),
                   exponent)

               + pow(abs((hsv[1] - mean[1]) / (6.0f * stdDev[1])), exponent)

               + pow(abs((hsv[2] - mean[2]) / (6.0f * stdDev[2])), exponent));

      if (hsv[1] < sat_restoration) {
        r *= pow(sat_restoration / hsv[1], 2.0f);
      }

      if (hsv[2] < val_restoration) {
        r *= pow(val_restoration / hsv[2], 2.0f);
      }

      r     = max(min(max(1.0f - softness, 0.01f) * (r - 1) + (PI / 2.0f), PI),
              (PI / 2.0f));
      matte = max(cos(r) + 1.0f, 0.0f);

      *m = 1.0 - matte;

      m++;
    }
  }
}
void ThisClass::RGBtoHSV(float* RGB, float* HSV)
{
  float* r;
  float* g;
  float* b;

  r = &RGB[0];
  g = &RGB[1];
  b = &RGB[2];

  float H;

  float m = min(*r, min(*g, *b));
  float M = max(*r, max(*g, *b));
  float C = M - m;

  if (M == *r) {
    H = (*g - *b) / C;

    if (H < 0.0)
      H += 6.0f;
  }
  else if (M == *g)
    H = ((*b - *r) / C) + 2.0f;
  else if (M == *b)
    H = ((*r - *g) / C) + 4.0f;
  else
    H = 0;

  HSV[0] = max(min(H / 6.0f, 1.0f), 0.0f);
  HSV[1] = C / M;
  HSV[2] = M;

  if (M == 0) {
    HSV[0] = 0;
    HSV[1] = 0;
    HSV[2] = 0;
  }
}

void ThisClass::HSVtoRGB(float* HSV, float* RGB)
{
  float H = HSV[0];
  float S = HSV[1];
  float V = HSV[2];

  float R = 0;
  float G = 0;
  float B = 0;

  float C = V * S;

  double Ho = 6.0f * H;

  float X = C * (1.0 - abs(fmod(Ho, 2.0) - 1.0));

  if (Ho >= 0 && Ho < 1) {
    R = C;
    G = X;
    B = 0;
  }
  else if (Ho >= 1 && Ho < 2) {
    R = X;
    G = C;
    B = 0;
  }
  else if (Ho >= 2 && Ho < 3) {
    R = 0;
    G = C;
    B = X;
  }
  else if (Ho >= 3 && Ho < 4) {
    R = 0;
    G = X;
    B = C;
  }
  else if (Ho >= 4 && Ho < 5) {
    R = X;
    G = 0;
    B = C;
  }
  else if (Ho >= 5) {
    R = C;
    G = 0;
    B = X;
  }

  float m = V - C;

  RGB[0] = R + m;
  RGB[1] = G + m;
  RGB[2] = B + m;
}

void ThisClass::RGBtoLab(float* RGB, float* Lab)
{
  float X = 0.412453f * RGB[0] + 0.357580f * RGB[1] + 0.180423f * RGB[2];
  float Y = 0.212671f * RGB[0] + 0.715160f * RGB[1] + 0.072169f * RGB[2];
  float Z = 0.019334f * RGB[0] + 0.119193f * RGB[1] + 0.950227f * RGB[2];

  float Xn = 0.950456f;
  float Yn = 1.0f;
  float Zn = 1.088754f;

  float YYn = Y / Yn;
  float XXn = X / Xn;
  float ZZn = Z / Zn;

  if (YYn > 0.008856f) {
    Lab[0] = 116.0f * pow(YYn, 1.0f / 3.0f) - 16.0f;
    Lab[1] = 500.0f * (pow(XXn, 3.0f) - pow(YYn, 3.0f));
    Lab[2] = 200.0f * (pow(YYn, 3.0f) - pow(ZZn, 3.0f));
  }
  else {
    float A = 16.0f / 116.0f;

    Lab[0] = 903.3f * YYn;
    Lab[1] = 500.0f * ((7.787f * XXn + A) - (7.787f * YYn + A));
    Lab[2] = 200.0f * ((7.787f * YYn + A) - (7.787f * ZZn + A));
  }
}
float ThisClass::spillSupression(float* RGB, int algorithm)
{
  float temp        = 0;
  float suppression = 0;

  switch (algorithm) {
    case light:

      temp = RGB[1] - max(RGB[0], RGB[2]);
      if (temp > 0)
        suppression = temp;

      break;

    case medium:

      temp = RGB[1] - max(RGB[0], RGB[2]);
      if (temp > 0)
        suppression = max(0.0f, RGB[1] - (RGB[0] + RGB[2]) / 2.0f);

      break;

    case hard:

      temp = RGB[1] - (RGB[0] + RGB[2]) / 2.0f;
      if (temp > 0)
        suppression = temp;

      break;

    case extreme:

      temp = RGB[1] - min(RGB[0], RGB[2]);
      if (temp > 0)
        suppression = temp;

      break;
  }

  return suppression;
}
void ThisClass::noLicenseOutput(Bounds region)
{
  PixPtr ip(In_Img);
  PixPtr op(Out_Img);
  FltPixel p;

  float intensity;

  for (int y = region.y1; y <= region.y2; y++) {
    ip.GotoXY(region.x1, y);
    op.GotoXY(region.x1, y);

    for (int x = region.x1; x <= region.x2; x++) {
      p >>= ip;

      intensity = (p.R + p.G + p.B) / 3.0f;

      p.R = 1.0f;
      p.G = intensity;
      p.B = intensity;

      op >>= p;
    }
  }
}

void ThisClass::NotifyChanged(Input* in, Parameter* param, TimeStamp time,
                              TagList* tags)
{
  if (param && in == InScreenColor) {
    bool showCustomColor = *((Number*)param) > 1.5;

    for (int i = 0; i < 3; i++) {
      InCustomColor[i]->SetAttrs(IC_Visible, showCustomColor, TAG_DONE);
    }
  }
  else if (param && in == InSpecifySourceFrame) {
    bool showSourceFrame = *((Number*)param) >= 0.5;

    InSourceFrame->SetAttrs(IC_Visible, showSourceFrame, TAG_DONE);
  }

  BaseClass::NotifyChanged(in, param, time, tags);
}
