// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright (C) 2012
//
// Authority FX, Inc.
// www.authorityfx.com

#include "stdafx.h"
#include "afx_clamp.h"

#define pluginGuid ("com.authorityfx:clampPlugin")
#define pluginName ("AFX Clamp")
#define PLUGIN_LICENSE_NAME ("clamp")

bool ThisClass::pluginIsRegistered;
std::stringstream ThisClass::licenseStatus;

// Register the plugin
FuRegisterClass(COMPANY_ID ".afx_clamp", CT_Tool)
  //REG_TileID, AFX_Convolve_Tile,
  REGS_Name, pluginName,
  REGS_Category, "Authority FX",
  REGS_OpIconString, "ACl",
  REGS_OpDescription, "Clips out of range values",
  REGS_VersionString, "Version 1.0",
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
  InEnableRGB =
      AddInput("Enabled RGB Clamp", "EnableRGB", LINKID_DataType,
               CLSID_DataType_Number, INPID_InputControl, CHECKBOXCONTROL_ID,
               INP_Default, 1.0, INP_DoNotifyChanged, TRUE, TAG_DONE);

  InMaxType =
      AddInput("RGB Max Type", "MaxType", LINKID_DataType,
               CLSID_DataType_Number, INPID_InputControl, COMBOCONTROL_ID,
               CCS_AddString, "Lightness", CCS_AddString, "Luma", CCS_AddString,
               "RGB", CCS_AddString, "Value", INP_Default, 3.0, TAG_DONE);

  InRGBMin =
      AddInput("RGB Min", "RGBMin", LINKID_DataType, CLSID_DataType_Number,
               INPID_InputControl, RANGECONTROL_ID, IC_ControlGroup, 1,
               IC_ControlID, RANGECONTROL_LOW, INP_MinScale, 0.0, INP_MaxScale,
               1.0, INP_Default, 0.0, TAG_DONE);

  InRGBMax =
      AddInput("RGB Max", "RGBMax", LINKID_DataType, CLSID_DataType_Number,
               INPID_InputControl, RANGECONTROL_ID, IC_ControlGroup, 1,
               IC_ControlID, RANGECONTROL_HIGH, INP_MinScale, 0.0, INP_MaxScale,
               1.0, INP_Default, 1.0, TAG_DONE);

  InEnableAlpha =
      AddInput("Enabled Alpha Clamp", "EnableAlpha", LINKID_DataType,
               CLSID_DataType_Number, INPID_InputControl, CHECKBOXCONTROL_ID,
               INP_Default, 0.0, INP_DoNotifyChanged, TRUE, TAG_DONE);

  InAlphaMin =
      AddInput("Alpha Min", "AlphaMin", LINKID_DataType, CLSID_DataType_Number,
               INPID_InputControl, RANGECONTROL_ID, IC_ControlGroup, 2,
               IC_ControlID, RANGECONTROL_LOW, INP_MinScale, 0.0, INP_MaxScale,
               1.0, INP_Default, 0.0, TAG_DONE);

  InAlphaMax =
      AddInput("Alpha Max", "AlphaMax", LINKID_DataType, CLSID_DataType_Number,
               INPID_InputControl, RANGECONTROL_ID, IC_ControlGroup, 2,
               IC_ControlID, RANGECONTROL_HIGH, INP_MinScale, 0.0, INP_MaxScale,
               1.0, INP_Default, 1.0, TAG_DONE);

  // Add image
  InImage = AddInput("Input", "Input", LINKID_DataType, CLSID_DataType_Image,
                     LINK_Main, 1, TAG_DONE);

  // Output image
  OutImage = AddOutput("Output", "Output", LINKID_DataType,
                       CLSID_DataType_Image, LINK_Main, 1, TAG_DONE);
}

void ThisClass::NotifyChanged(Input* in, Parameter* param, TimeStamp time,
                              TagList* tags)
{
  Number* enableRGB = (Number*)InEnableRGB->GetSourceAttrs(
      time, NULL, REQF_SecondaryTime, REQF_SecondaryTime);
  Number* enableAlpha = (Number*)InEnableAlpha->GetSourceAttrs(
      time, NULL, REQF_SecondaryTime, REQF_SecondaryTime);

  if (enableRGB && in == InEnableRGB) {
    bool showRGBControls = *enableRGB == 1.0;

    InMaxType->SetAttrs(IC_Visible, showRGBControls, TAG_DONE);
    InRGBMin->SetAttrs(IC_Visible, showRGBControls, TAG_DONE);
    InRGBMax->SetAttrs(IC_Visible, showRGBControls, TAG_DONE);
  }

  if (enableAlpha && in == InEnableAlpha) {
    bool showAlphaControls = *enableAlpha == 1.0;

    InAlphaMin->SetAttrs(IC_Visible, showAlphaControls, TAG_DONE);
    InAlphaMax->SetAttrs(IC_Visible, showAlphaControls, TAG_DONE);
  }

  if (enableRGB)
    enableRGB->Recycle();
  if (enableAlpha)
    enableAlpha->Recycle();

  BaseClass::NotifyChanged(in, param, time, tags);
}

void ThisClass::Process(Request* req)
{
  // Get input images
  imgInput  = (Image*)InImage->GetValue(req);
  imgOutput = NewImage(IMG_Like, imgInput, TAG_DONE);

  width  = imgInput->Width;
  height = imgInput->Height;

  mode         = (int)(*InMaxType->GetValue(req) + 0.5);
  rgbEnabled   = *InEnableRGB->GetValue(req) > 0.5;
  rgbMin       = *InRGBMin->GetValue(req);
  rgbMax       = *InRGBMax->GetValue(req);
  alphaEnabled = *InEnableAlpha->GetValue(req) > 0.5;
  alphaMin     = *InAlphaMin->GetValue(req);
  alphaMax     = *InAlphaMax->GetValue(req);

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

  // Set output image
  OutImage->Set(req, imgOutput);
}

bool ThisClass::process_GPU() { return false; }

bool ThisClass::process_CPU()
{
  Multithreader threader(Bounds(0, 0, width - 1, height - 1));
  threader.processInChunks(boost::bind(&ThisClass::processChunk, this, _1));

  if (Status != STATUS_OK)
    return false;

  return true;
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

      // RGBA channels
      if (rgbEnabled) {
        switch (mode) {
          case 0:  // Lightness
          {
            float srcLightness =
                0.5 * (std::max<float>(p.R, std::max<float>(p.G, p.B)) +
                       std::min<float>(p.R, std::max<float>(p.G, p.B)));
            float finalLightness = std::min<float>(
                rgbMax,
                std::max<float>(std::max<float>(rgbMin, 0.0f), srcLightness));
            float scalingFactor = finalLightness / srcLightness;

            if (srcLightness > std::max<float>(rgbMin, 0.0f)) {
              p.R *= scalingFactor;
              p.G *= scalingFactor;
              p.B *= scalingFactor;
            }
          } break;

          case 1:  // Luma
          {
            float srcLuma   = (0.2126 * p.R + 0.7152 * p.G + 0.0722 * p.B);
            float finalLuma = std::min<float>(
                rgbMax,
                std::max<float>(std::max<float>(rgbMin, 0.0f), srcLuma));
            float scalingFactor = finalLuma / srcLuma;

            if (srcLuma > std::max<float>(rgbMin, 0.0f)) {
              p.R *= scalingFactor;
              p.G *= scalingFactor;
              p.B *= scalingFactor;
            }
          } break;

          case 2:  // RGB
            // Apply upper clamp
            p.R = std::min<float>(rgbMax, p.R);
            p.G = std::min<float>(rgbMax, p.G);
            p.B = std::min<float>(rgbMax, p.B);
            break;

          case 3:  // Value
          {
            float srcValue   = std::max<float>(p.R, std::max<float>(p.G, p.B));
            float finalValue = std::min<float>(
                rgbMax,
                std::max<float>(std::max<float>(rgbMin, 0.0f), srcValue));
            float scalingFactor = finalValue / srcValue;

            if (srcValue > std::max<float>(rgbMin, 0.0f)) {
              p.R *= scalingFactor;
              p.G *= scalingFactor;
              p.B *= scalingFactor;
            }
          } break;
        }

        // Apply lower clamp
        p.R = std::max<float>(rgbMin, p.R);
        p.G = std::max<float>(rgbMin, p.G);
        p.B = std::max<float>(rgbMin, p.B);
      }

      // Alpha channel clamp
      if (alphaEnabled) {
        p.A = std::max<float>(alphaMin, p.A);
        p.A = std::min<float>(alphaMax, p.A);
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
