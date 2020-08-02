// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright (C) 2012
//
// Authority FX, Inc.
// www.authorityfx.com

#include "stdafx.h"
#include "afx_desaturate.h"

#define pluginGuid ("com.authorityfx:desaturatePlugin")
#define pluginName ("AFX Desaturate")
#define PLUGIN_LICENSE_NAME ("desaturate")

bool ThisClass::pluginIsRegistered;
std::stringstream ThisClass::licenseStatus;

FuRegisterClass(COMPANY_ID ".afx_desaturate", CT_Tool)
  //REG_TileID,          AFX_Convolve_Tile,
  REGS_Name,          pluginName,
  REGS_Category,        "Authority FX",
  REGS_OpIconString,      "ADes",
  REGS_OpDescription,      "Configurable Desaturation",
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
  InMode = AddInput("Mode", "Mode", LINKID_DataType, CLSID_DataType_Number,
                    INPID_InputControl, COMBOCONTROL_ID, CCS_AddString,
                    "CIELAB L*", CCS_AddString, "Custom", CCS_AddString,
                    "HSI Intensity", CCS_AddString, "HSL Lightness",
                    CCS_AddString, "HSV Value", CCS_AddString, "Luma",
                    INP_Default, 0.0, INP_DoNotifyChanged, TRUE, TAG_DONE);

  InRed    = AddInput("Red", "Red", LINKID_DataType, CLSID_DataType_Number,
                   INPID_InputControl, COLORSUPPRESSIONCONTROL_ID,
                   IC_ControlGroup, 1, IC_ControlID,
                   COLORSUPPRESSIONCONTROL_RED, INP_Default, 1.0, TAG_DONE);
  InYellow = AddInput(
      "Yellow", "Yellow", LINKID_DataType, CLSID_DataType_Number,
      INPID_InputControl, COLORSUPPRESSIONCONTROL_ID, IC_ControlGroup, 1,
      IC_ControlID, COLORSUPPRESSIONCONTROL_YELLOW, INP_Default, 1.0, TAG_DONE);
  InGreen = AddInput("Green", "Green", LINKID_DataType, CLSID_DataType_Number,
                     INPID_InputControl, COLORSUPPRESSIONCONTROL_ID,
                     IC_ControlGroup, 1, IC_ControlID,
                     COLORSUPPRESSIONCONTROL_GREEN, INP_Default, 1.0, TAG_DONE);
  InCyan  = AddInput("Cyan", "Cyan", LINKID_DataType, CLSID_DataType_Number,
                    INPID_InputControl, COLORSUPPRESSIONCONTROL_ID,
                    IC_ControlGroup, 1, IC_ControlID,
                    COLORSUPPRESSIONCONTROL_CYAN, INP_Default, 1.0, TAG_DONE);
  InBlue  = AddInput("Blue", "Blue", LINKID_DataType, CLSID_DataType_Number,
                    INPID_InputControl, COLORSUPPRESSIONCONTROL_ID,
                    IC_ControlGroup, 1, IC_ControlID,
                    COLORSUPPRESSIONCONTROL_BLUE, INP_Default, 1.0, TAG_DONE);
  InMagenta =
      AddInput("Magenta", "Magenta", LINKID_DataType, CLSID_DataType_Number,
               INPID_InputControl, COLORSUPPRESSIONCONTROL_ID, IC_ControlGroup,
               1, IC_ControlID, COLORSUPPRESSIONCONTROL_MAGENTA, INP_Default,
               1.0, TAG_DONE);

  InDesaturation = AddInput(
      "Desaturation", "Desaturation", LINKID_DataType, CLSID_DataType_Number,
      INPID_InputControl, SLIDERCONTROL_ID, INP_Default, 1.0, INP_MinAllowed,
      0.0, INP_MaxAllowed, 1.0, INP_MinScale, 0.0, INP_MaxScale, 1.0, TAG_DONE);

  InGain = AddInput("Gain", "Gain", LINKID_DataType, CLSID_DataType_Number,
                    INPID_InputControl, SLIDERCONTROL_ID, INP_Default, 1.0,
                    INP_MinAllowed, 0.0, INP_MinScale, 0.0, INP_MaxScale, 2.0,
                    TAG_DONE);

  InLift = AddInput("Lift", "Lift", LINKID_DataType, CLSID_DataType_Number,
                    INPID_InputControl, SLIDERCONTROL_ID, INP_Default, 0.0,
                    INP_MinAllowed, -1.0, INP_MaxAllowed, 1.0, INP_MinScale,
                    -1.0, INP_MaxScale, 1.0, TAG_DONE);

  InGamma = AddInput("Gamma", "Gamma", LINKID_DataType, CLSID_DataType_Number,
                     INPID_InputControl, SLIDERCONTROL_ID, INP_Default, 1.0,
                     INP_MinAllowed, 0.0, INP_MaxAllowed, 10.0, INP_MinScale,
                     0.0, INP_MaxScale, 3.0, TAG_DONE);

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
  Number* mode = (Number*)InMode->GetSourceAttrs(time, NULL, REQF_SecondaryTime,
                                                 REQF_SecondaryTime);

  if (mode && in == InMode) {
    bool showColorControls = *mode == 1.0;

    InRed->SetAttrs(IC_Visible, showColorControls, TAG_DONE);
    InYellow->SetAttrs(IC_Visible, showColorControls, TAG_DONE);
    InGreen->SetAttrs(IC_Visible, showColorControls, TAG_DONE);
    InCyan->SetAttrs(IC_Visible, showColorControls, TAG_DONE);
    InBlue->SetAttrs(IC_Visible, showColorControls, TAG_DONE);
    InMagenta->SetAttrs(IC_Visible, showColorControls, TAG_DONE);
  }

  if (mode)
    mode->Recycle();

  BaseClass::NotifyChanged(in, param, time, tags);
}

void ThisClass::Process(Request* req)
{
  // Get input images
  imgInput  = (Image*)InImage->GetValue(req);
  imgOutput = NewImage(IMG_Like, imgInput, TAG_DONE);

  width  = imgInput->Width;
  height = imgInput->Height;

  mode = (int)(*InMode->GetValue(req));

  RedAmount     = *InRed->GetValue(req);
  YellowAmount  = *InYellow->GetValue(req);
  GreenAmount   = *InGreen->GetValue(req);
  CyanAmount    = *InCyan->GetValue(req);
  BlueAmount    = *InBlue->GetValue(req);
  MagentaAmount = *InMagenta->GetValue(req);

  gain         = *InGain->GetValue(req);
  lift         = *InLift->GetValue(req);
  gamma        = *InGamma->GetValue(req);
  desaturation = *InDesaturation->GetValue(req);

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
  PixPtr ip(imgInput);
  PixPtr op(imgOutput);
  FltPixel p;

  for (int y = region.y1; y <= region.y2; y++) {
    ip.GotoXY(0, y);
    op.GotoXY(0, y);

    for (int x = region.x1; x <= region.x2; x++) {
      p >>= ip;

      float red   = p.R;
      float green = p.G;
      float blue  = p.B;

      float out = 0.0f;

      switch (mode) {
        case 0:  // CIELAB
        {
          double X = 0.412453 * red + 0.357580 * green + 0.180423 * blue;
          double Y = 0.212671 * red + 0.715160 * green + 0.072169 * blue;
          double Z = 0.019334 * red + 0.119193 * green + 0.950227 * blue;

          double Xn = 0.950456;
          double Yn = 1.0;
          double Zn = 1.088754;

          double L;

          if (Y / Yn > 0.008856)
            L = 116.0 * pow(Y / Yn, 1.0 / 3.0) - 16.0;
          else
            L = 903.3 * Y / Yn;

          double fy  = (L + 16.0) / 116.0;
          double dd3 = 108.0 / 841.0;

          if (fy > 6.0 / 29.0) {
            X = Xn * pow(fy, 3.0);
            Y = Yn * pow(fy, 3.0);
            Z = Zn * pow(fy, 3.0);
          }
          else {
            X = (fy - 16.0 / 116.0) * dd3 * Xn;
            Y = (fy - 16.0 / 116.0) * dd3 * Yn;
            Z = (fy - 16.0 / 116.0) * dd3 * Zn;
          }

          red   = (float)(3.240479 * X - 1.537150 * Y - 0.498535 * Z);
          green = (float)(-0.969256 * X + 1.875992 * Y + 0.041556 * Z);
          blue  = (float)(0.055648 * X - 0.204043 * Y + 1.057311 * Z);

          out = (red + green + blue) / 3.0f;
        } break;
        case 1:  // Custom
        {
          double M         = std::max<float>(std::max<float>(red, green), blue);
          double m         = std::min<float>(std::min<float>(red, green), blue);
          double chroma    = M - m;
          double lightness = (red + green + blue) / 3.0f;

          double hue;

          if (M == red)
            hue = fmodf((green - blue) / chroma + 6.0f, 6.0f);
          else if (M == green)
            hue = (blue - red) / chroma + 2.0f;
          else if (M == blue)
            hue = (red - green) / chroma + 4.0f;

          double grayGain = (RedAmount + YellowAmount + GreenAmount +
                             CyanAmount + BlueAmount + MagentaAmount) /
                            6.0;

          double levels;

          if (hue >= 0.0f && hue < 1.0f)
            levels =
                (RedAmount * (1.0f - hue) + YellowAmount * hue) * lightness;
          else if (hue >= 1.0 && hue < 2.0f)
            levels = (YellowAmount * (1.0f - (hue - 1.0f)) +
                      GreenAmount * (hue - 1.0f)) *
                     lightness;
          else if (hue >= 2.0 && hue < 3.0f)
            levels = (GreenAmount * (1.0f - (hue - 2.0f)) +
                      CyanAmount * (hue - 2.0f)) *
                     lightness;
          else if (hue >= 3.0 && hue < 4.0f)
            levels = (CyanAmount * (1.0f - (hue - 3.0f)) +
                      BlueAmount * (hue - 3.0f)) *
                     lightness;
          else if (hue >= 4.0 && hue < 5.0f)
            levels = (BlueAmount * (1.0f - (hue - 4.0f)) +
                      MagentaAmount * (hue - 4.0f)) *
                     lightness;
          else if (hue >= 5.0 && hue < 6.0f)
            levels = (MagentaAmount * (1.0f - (hue - 5.0f)) +
                      RedAmount * (hue - 5.0f)) *
                     lightness;

          out =
              (float)(levels * chroma + lightness * grayGain * (1.0f - chroma));

        } break;
        case 2:  // Intensity
          out = (red + green + blue) / 3.0f;
          break;
        case 3:  // Lightness
          out = 0.5f * (std::max<float>(std::max<float>(red, green), blue) +
                        std::min<float>(std::min<float>(red, green), blue));
          break;
        case 4:  // Value
          out = std::max<float>(std::max<float>(red, green), blue);
          break;
        case 5:  // Luma
          out = 0.299f * red + 0.587f * green + 0.114f * blue;
          break;
      }

      p.R = out * desaturation + p.R * (1.0f - desaturation);
      p.G = out * desaturation + p.G * (1.0f - desaturation);
      p.B = out * desaturation + p.B * (1.0f - desaturation);

      // Apply lift and gain
      p.R = lift + p.R * (gain - lift);
      p.G = lift + p.G * (gain - lift);
      p.B = lift + p.B * (gain - lift);

      // Apply gamma
      p.R /= std::max<float>(0.001, p.A);
      p.R = (float)pow((double)p.R, 1.0 / gamma);
      p.R *= std::max<float>(0.001, p.A);

      p.G /= std::max<float>(0.001, p.A);
      p.G = (float)pow((double)p.G, 1.0 / gamma);
      p.G *= std::max<float>(0.001, p.A);

      p.B /= std::max<float>(0.001, p.A);
      p.B = (float)pow((double)p.B, 1.0 / gamma);
      p.B *= std::max<float>(0.001, p.A);

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
