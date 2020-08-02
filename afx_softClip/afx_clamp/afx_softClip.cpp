// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright (C) 2012
//
// Authority FX, Inc.
// www.authorityfx.com

#include "stdafx.h"
#include "afx_softClip.h"

#define pluginGuid ("com.authorityfx:softClipPlugin")
#define pluginName ("AFX Soft Clip")
#define PLUGIN_LICENSE_NAME ("soft_clip")

bool ThisClass::pluginIsRegistered;
std::stringstream ThisClass::licenseStatus;

// Register the plugin
FuRegisterClass(COMPANY_ID ".afx_softClip", CT_Tool)
  //REG_TileID,          AFX_Convolve_Tile,
  REGS_Name,          pluginName,
  REGS_Category,        "Authority FX",
  REGS_OpIconString,      "ASC",
  REGS_OpDescription,      "Nonlinear clipping of out of range values",
  REGS_VersionString,      "Version 1.0",
  REGS_Company,        "Authority FX, Inc.",
  REGS_URL,          "http://www.authorityfx.com/",
  REG_NoObjMatCtrls,      TRUE,
  REG_NoMotionBlurCtrls,    TRUE,
  REG_NoChannelCtrls,      TRUE,
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
  InClipChannel =
      AddInput("Clip Channel", "ClipChannel", LINKID_DataType,
               CLSID_DataType_Number, INPID_InputControl, COMBOCONTROL_ID,
               CCS_AddString, "Lightness", CCS_AddString, "Luma", CCS_AddString,
               "RGB", CCS_AddString, "Value", INP_Default, 3.0, TAG_DONE);

  InMax = AddInput("Max", "Max", LINKID_DataType, CLSID_DataType_Number,
                   INPID_InputControl, SLIDERCONTROL_ID, INP_MinAllowed, 0.0,
                   INP_MinScale, 0.0, INP_MaxScale, 2.0, INP_Default, 1.0,
                   TAG_DONE);

  InKnee = AddInput("Knee Sharpness", "Knee", LINKID_DataType,
                    CLSID_DataType_Number, INPID_InputControl, SLIDERCONTROL_ID,
                    INP_MinAllowed, 0.0, INP_MaxAllowed, 1.0, INP_MinScale, 0.0,
                    INP_MaxScale, 1.0, INP_Default, 0.5, TAG_DONE);

  // Add image
  InImage = AddInput("Input", "Input", LINKID_DataType, CLSID_DataType_Image,
                     LINK_Main, 1, TAG_DONE);

  // Output image
  OutImage = AddOutput("Output", "Output", LINKID_DataType,
                       CLSID_DataType_Image, LINK_Main, 1, TAG_DONE);
}

void ThisClass::Process(Request* req)
{
  imgInput  = (Image*)InImage->GetValue(req);
  imgOutput = NewImage(IMG_Like, imgInput, TAG_DONE);

  mode    = (int)*InClipChannel->GetValue(req);
  clipMax = *InMax->GetValue(req);
  knee    = *InKnee->GetValue(req);

  if (pluginIsRegistered) {
    if (process_GPU()) {
    }
    else if (process_CPU()) {
    }
  }
  else {
    // Not licensed - output red
    Document->PrintF(ECONID_Error, "%s\n", licenseStatus.str().c_str());
    {
      Multithreader threader(
          Bounds(0, 0, imgInput->Width - 1, imgInput->Height - 1));
      threader.processInChunks(
          boost::bind(&ThisClass::noLicenseOutput, this, _1));
    }
  }

  OutImage->Set(req, imgOutput);
}

bool ThisClass::process_CPU()
{
  Multithreader threader(
      Bounds(0, 0, imgInput->Width - 1, imgInput->Height - 1));
  threader.processInChunks(boost::bind(&ThisClass::processChunk, this, _1));

  if (Status != STATUS_OK)
    return false;

  return true;
}

bool ThisClass::process_GPU() { return false; }

// The soft clip curve. Where the magic happens
// clip is the maximum value (output approaches clip as input approaches
// infinity) knee is the sharpness of the transision between the linear region
// and the saturated region
inline float softClip(double value, double clip, double knee)
{
  if (value <= 0.0)
    return (float)value;

  double mappedKnee = tan(knee * 3.1415926535897926536 * 0.5);

  if (knee >= 1.0) {
    // Maximum knee sharpness. Just clamp the value.
    return std::min<float>(value, clip);
  }
  else {
    return (float)(clip *
                   pow(tanh(pow(value / clip, mappedKnee)), 1.0 / mappedKnee));
  }
}

void ThisClass::processChunk(Bounds region)
{
  PixPtr ip(imgInput), op(imgOutput);
  FltPixel p;

  for (int y = region.y1; y <= region.y2; y++) {
    ip.GotoXY(0, y);
    op.GotoXY(0, y);

    for (int x = region.x1; x <= region.x2; x++) {
      p >>= ip;

      switch (mode) {
        case 0: {
          float srcLightness =
              0.5f * (std::max<float>(p.R, std::max<float>(p.G, p.B)) +
                      std::min<float>(p.R, std::max<float>(p.G, p.B)));
          float finalLightness = softClip(srcLightness, clipMax, knee);
          float scalingFactor =
              finalLightness / std::max<float>(srcLightness, 0.00001f);

          p.R *= scalingFactor;
          p.G *= scalingFactor;
          p.B *= scalingFactor;
        } break;

        case 1: {
          float srcLuma       = (0.299f * p.R + 0.587f * p.G + 0.114f * p.B);
          float finalLuma     = softClip(srcLuma, clipMax, knee);
          float scalingFactor = finalLuma / std::max<float>(srcLuma, 0.00001f);

          p.R *= scalingFactor;
          p.G *= scalingFactor;
          p.B *= scalingFactor;
        } break;

        case 2:
          // Apply upper clamp
          p.R = softClip(p.R, clipMax, knee);
          p.G = softClip(p.G, clipMax, knee);
          p.B = softClip(p.B, clipMax, knee);
          break;

        case 3: {
          float srcValue   = std::max<float>(p.R, std::max<float>(p.G, p.B));
          float finalValue = softClip(srcValue, clipMax, knee);
          float scalingFactor =
              finalValue / std::max<float>(srcValue, 0.00001f);

          p.R *= scalingFactor;
          p.G *= scalingFactor;
          p.B *= scalingFactor;
        } break;
      }

      op >>= p;
    }
  }
}

void ThisClass::noLicenseOutput(Bounds region)
{
  PixPtr ip(imgInput);
  PixPtr op(imgOutput);
  FltPixel p;

  for (int y = region.y1; y <= region.y2; y++) {
    ip.GotoXY(region.x1, y);
    op.GotoXY(region.x1, y);

    for (int x = region.x1; x <= region.x2; x++) {
      p >>= ip;
      p.R = 1.0f;
      p.G = p.B = (p.R + p.G + p.B) / 3.0f;
      op >>= p;
    }
  }
}
