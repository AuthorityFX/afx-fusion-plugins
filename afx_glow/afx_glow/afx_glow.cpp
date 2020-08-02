// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright (C) 2012
//
// Authority FX, Inc.
// www.authorityfx.com

#include "stdafx.h"
#include "afx_glow.h"

#define PI (3.14159265358979323846)
#define iterations (75)
#define pluginGuid ("com.authorityfx:glowPlugin")
#define pluginName ("AFX Glow")
#define PLUGIN_LICENSE_NAME ("glow")

bool ThisClass::pluginIsRegistered;
std::stringstream ThisClass::licenseStatus;

FuRegisterClass(COMPANY_ID ".afx_glow", CT_Tool)
  //REG_TileID,        AFX_Convolve_Tile,
  REGS_Name,          pluginName,
  REGS_Category,        "Authority FX",
  REGS_OpIconString,      "AGlo",
  REGS_OpDescription,      "Realistic glow with adjustable falloff",
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
  InIntensity =
      AddInput("Intensity", "intensity", LINKID_DataType, CLSID_DataType_Number,
               INPID_InputControl, SLIDERCONTROL_ID, INP_Default, 1.0,
               INP_MinAllowed, 0.0, INP_MaxAllowed, 50.0, INP_MinScale, 0.0,
               INP_MaxScale, 2.0, TAG_DONE);

  InInnerSize =
      AddInput("Inner Size", "inner_size", LINKID_DataType,
               CLSID_DataType_Number, INPID_InputControl, SLIDERCONTROL_ID,
               INP_Default, 0.01, INP_MinAllowed, 0.0, INP_MaxAllowed, 1.0,
               INP_MinScale, 0.0, INP_MaxScale, 0.25, TAG_DONE);

  InOuterSize =
      AddInput("Outer Size", "outer_size", LINKID_DataType,
               CLSID_DataType_Number, INPID_InputControl, SLIDERCONTROL_ID,
               INP_Default, 2.0, INP_MinAllowed, 0.0, INP_MaxAllowed, 20.0,
               INP_MinScale, 0.0, INP_MaxScale, 5.0, TAG_DONE);

  InSizeExponent =
      AddInput("Diffusion", "diffusion", LINKID_DataType, CLSID_DataType_Number,
               INPID_InputControl, SLIDERCONTROL_ID, INP_Default, 0.5,
               INP_MinAllowed, 0.0, INP_MaxAllowed, 50.0, INP_MinScale, 0.0,
               INP_MaxScale, 1.0, TAG_DONE);

  InThreshold =
      AddInput("Threshold", "threshold", LINKID_DataType, CLSID_DataType_Number,
               INPID_InputControl, SLIDERCONTROL_ID, INP_Default, 0.75,
               INP_MinAllowed, 0.0, INP_MaxAllowed, 50.0, INP_MinScale, 0.0,
               INP_MaxScale, 2.0, TAG_DONE);

  InQuality =
      AddInput("Quality", "quality", LINKID_DataType, CLSID_DataType_Number,
               INPID_InputControl, SLIDERCONTROL_ID, INP_Default, 0.5,
               INP_MinAllowed, 0.0, INP_MaxAllowed, 1.0, TAG_DONE);

  InTint[0] = AddInput("Red", "red", LINKID_DataType, CLSID_DataType_Number,
                       INPID_InputControl, COLORCONTROL_ID, ICS_Name,
                       "Glow Tint", IC_ControlGroup, 1, IC_ControlID, CHAN_RED,
                       INP_MinAllowed, 0.0, INP_MaxAllowed, 50.0, INP_MinScale,
                       0.0, INP_MaxScale, 1.0, INP_Default, 1.0, TAG_DONE);

  InTint[1] = AddInput("Green", "green", LINKID_DataType, CLSID_DataType_Number,
                       INPID_InputControl, COLORCONTROL_ID, IC_ControlGroup, 1,
                       IC_ControlID, CHAN_GREEN, INP_MinAllowed, 0.0,
                       INP_MaxAllowed, 50.0, INP_MinScale, 0.0, INP_MaxScale,
                       1.0, INP_Default, 1.0, TAG_DONE);

  InTint[2] = AddInput("Blue", "blue", LINKID_DataType, CLSID_DataType_Number,
                       INPID_InputControl, COLORCONTROL_ID, IC_ControlGroup, 1,
                       IC_ControlID, CHAN_BLUE, INP_MinAllowed, 0.0,
                       INP_MaxAllowed, 50.0, INP_MinScale, 0.0, INP_MaxScale,
                       1.0, INP_Default, 1.0, TAG_DONE);

  InGammaSpace =
      AddInput("In Gamma Space", "gamma_space", LINKID_DataType,
               CLSID_DataType_Number, INPID_InputControl, COMBOCONTROL_ID,
               CCS_AddString, "Linear", CCS_AddString, "sRGB / 2.2", TAG_DONE);

  InRepeatBorders =
      AddInput("Repeat Borders", "repeat_borders", LINKID_DataType,
               CLSID_DataType_Number, INPID_InputControl, CHECKBOXCONTROL_ID,
               INP_Default, 1.0, TAG_DONE);

  // Input image
  InImage = AddInput("Input", "input", LINKID_DataType, CLSID_DataType_Image,
                     LINK_Main, 1, TAG_DONE);

  // Pre-mask
  InPreMask =
      AddInput("Pre Mask", "premask", LINKID_DataType, CLSID_DataType_Mask,
               INP_Priority, MASK_PRI, INP_Required, FALSE, TAG_DONE);

  // Output image
  OutImage = AddOutput("Output", "output", LINKID_DataType,
                       CLSID_DataType_Image, LINK_Main, 1, TAG_DONE);
}

void ThisClass::Process(Request* req)
{
  // Get input images
  In_Img      = (Image*)InImage->GetValue(req);
  PreMask_Img = (Image*)InPreMask->GetValue(req);

  // Get parameters
  intensity      = *InIntensity->GetValue(req);
  inner_size     = *InInnerSize->GetValue(req);
  outer_size     = *InOuterSize->GetValue(req);
  falloff        = *InSizeExponent->GetValue(req);
  threshold      = *InThreshold->GetValue(req);
  quality        = *InQuality->GetValue(req);
  tint[0]        = *InTint[0]->GetValue(req);
  tint[1]        = *InTint[1]->GetValue(req);
  tint[2]        = *InTint[2]->GetValue(req);
  gamma_space    = *InGammaSpace->GetValue(req);
  repeat_borders = *InRepeatBorders->GetValue(req) >= 0.5;

  width  = In_Img->Width;
  height = In_Img->Height;

  Out_Img = NewImage(IMG_Like, In_Img, TAG_DONE);

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
          Bounds(0, 0, In_Img->Width - 1, In_Img->Height - 1));
      threader.processInChunks(
          boost::bind(&ThisClass::noLicenseOutput, this, _1));
    }
  }

  OutImage->Set(req, Out_Img);
}

void ThisClass::grabInput(Bounds region, ImageContainerA* input)
{
  bool maskExists = (PreMask_Img != 0);

  PixPtr in_pix(In_Img);
  FltPixel p_in;

  PixPtr mask_pix(maskExists ? PreMask_Img : In_Img);
  FltPixel p_mask;

  float* p_r = 0;
  float* p_g = 0;
  float* p_b = 0;

  float color[3];

  for (int y = region.y1; y <= region.y2; y++) {
    in_pix.GotoXY(region.x1, y);

    if (maskExists)
      mask_pix.GotoXY(region.x1, y);

    // Set pointers
    p_r = (float*)&input[0].pixel_at(region.x1, y);
    p_g = (float*)&input[1].pixel_at(region.x1, y);
    p_b = (float*)&input[2].pixel_at(region.x1, y);

    for (int x = region.x1; x <= region.x2; x++) {
      p_in >>= in_pix;
      if (maskExists == true)
        p_mask >>= mask_pix;

      color[0] = p_in.R;
      color[1] = p_in.G;
      color[2] = p_in.B;

      for (int i = 0; i < 3; i++) color[i] = max(0.0f, color[i]);

      float luma = 0.3f * color[0] + 0.59f * color[1] + 0.11f * color[2];

      // Apply threshold
      if (luma < threshold)
        color[0] = color[1] = color[2] = 0.0f;

      if (luma >= threshold && luma < 1.0 && threshold > 0.0) {
        float scale = (luma - threshold) / (1.0f - threshold) / luma;

        for (int i = 0; i < 3; i++) color[i] *= scale;
      }

      // If input gamma space is linear, convert to 2.2
      if (gamma_space == 0) {
        for (int i = 0; i < 3; i++) color[i] = pow(color[i], 1.0f / 2.2f);
      }

      // Store pixel values in memory
      if (maskExists == true) {
        *p_r = (Ipp32f)(color[0] * p_mask.A);
        *p_g = (Ipp32f)(color[1] * p_mask.A);
        *p_b = (Ipp32f)(color[2] * p_mask.A);
      }
      else {
        *p_r = (Ipp32f)color[0];
        *p_g = (Ipp32f)color[1];
        *p_b = (Ipp32f)color[2];
      }

      // Advance to next pixel
      p_r++;
      p_g++;
      p_b++;
    }
  }
}

void ThisClass::setOutput(Bounds region, ImageContainerA* output)
{
  PixPtr op(Out_Img);
  FltPixel p;

  float* p_r = 0;
  float* p_g = 0;
  float* p_b = 0;

  float color[3];

  for (int y = region.y1; y <= region.y2; y++) {
    op.GotoXY(region.x1, y);

    p_r = (float*)&output[0].pixel_at(region.x1, y);
    p_g = (float*)&output[1].pixel_at(region.x1, y);
    p_b = (float*)&output[2].pixel_at(region.x1, y);

    for (int x = region.x1; x <= region.x2; x++) {
      color[0] = max(*p_r, 0.0f);
      color[1] = max(*p_g, 0.0f);
      color[2] = max(*p_b, 0.0f);

      for (int i = 0; i < 3; i++) {
        if (gamma_space == 0) {
          color[i] = pow(color[i], 2.2f) * pow(tint[i], 2.2f);
        }
        else {
          color[i] *= tint[i];
        }

        color[i] *= intensity;
      }

      p.R = color[0];
      p.G = color[1];
      p.B = color[2];
      p.A = 0.0;

      // Advance to next pixel
      op >>= p;
      p_r++;
      p_g++;
      p_b++;
    }
  }
}

bool ThisClass::process_CPU()
{
  // Set progress bar scale
  ThisClass::ProgressScale = 5;
  ThisClass::ProgressCount = 0;

  // Allocate memory for input and output images
  ImageContainerA glow[3];
  ImageContainerA glowSrc[3];
  for (int i = 0; i < 3; i++) {
    glowSrc[i].create_image(width, height);
    glow[i].create_image(width, height);
  }

  {
    // Copy and threshold input into glowSrc
    Multithreader threader(Bounds(0, 0, width - 1, height - 1));
    threader.processInChunks(
        boost::bind(&ThisClass::grabInput, this, _1, boost::ref(glowSrc)));
  }

  // Exit glow, render cancelled
  if (Status != 0)
    return false;

  // Allocate memory for gaussians
  float** gaussContainer;
  gaussContainer = new float*[iterations];
  int* gaussSize;
  gaussSize = new int[iterations];
  float* sigma;
  sigma = new float[iterations];

  int maxSize = 0;

  // Computer gaussian size
  for (int glowNum = 0; glowNum < iterations; glowNum++) {
    // Lookup value
    float lookUpValue = (float)glowNum / (float)(iterations - 1);
    // Compute glow size
    float glowSize = pow(lookUpValue, 1.0f / falloff) *
                         (max(outer_size, inner_size) - inner_size) +
                     inner_size;

    // Determine gaussian sigma
    sigma[glowNum] = max(min(width, height) * glowSize / 12.0f, 0.01f);

    float new_quality =
        cos((float)PI * 0.5f * lookUpValue) * (1.0f - quality) + quality;

    gaussSize[glowNum] = (int)ceil(
        max(((int)ceil(sigma[glowNum] * new_quality * (6.0f - 1.0f) + 1.0f)) |
                1,
            3) *
        0.5f);

    // Make current glow larger than last glow size
    if (maxSize > gaussSize[glowNum])
      gaussSize[glowNum] = maxSize;

    maxSize = gaussSize[glowNum];

    // Resize guass at gowNum in gauss container
    gaussContainer[glowNum] = new float[gaussSize[glowNum]];
  }

  // Size of kernel image
  int kernelSize = maxSize * 2 - 1;

  // Exit glow, render cancelled
  if (Status != 0)
    return false;

  {
    // Creat gaussian
    Multithreader threader(Bounds(0, 0, iterations - 1, maxSize - 1));
    threader.processLineByLine(boost::bind(
        &ThisClass::createGauss, this, _1, boost::ref(gaussContainer),
        boost::ref(gaussSize), boost::ref(sigma)));
  }

  // Exit glow, render cancelled
  if (Status != 0)
    return false;

  // Allocate memory for kernel image
  ImageContainerA kernel;
  kernel.create_image(kernelSize, kernelSize);

  ThisClass::ProgressCount = 1;

  {
    // Create kernel
    Multithreader threader(Bounds(0, 0, maxSize - 1, maxSize - 1));
    threader.processInChunks(
        boost::bind(&ThisClass::createKernel_CPU, this, _1, boost::ref(kernel),
                    boost::ref(gaussContainer), boost::ref(gaussSize), maxSize,
                    kernelSize));
  }

  // Exit glow, render cancelled
  if (Status != 0)
    return false;

  Ipp64f kernel_sum;
  ippiSum_32f_C1R(kernel.getImagePtr(), kernel.getRowSizeBytes(),
                  kernel.getSize(), &kernel_sum, ippAlgHintAccurate);
  ippiMulC_32f_C1IR(2.0f / kernel_sum, kernel.getImagePtr(),
                    kernel.getRowSizeBytes(), kernel.getSize());

  // De-allocate memory for guasscontainer
  for (int i = 0; i < iterations; i++) {
    delete[] gaussContainer[i];
  }

  delete[] gaussContainer;
  delete[] gaussSize;

  ThisClass::ProgressCount = 2;

  // Allocate memory for expanded borders glowSrc
  ImageContainerA glowSrcBorders[3];

  IppStatus status;

  for (int i = 0; i < 3; i++) {
    // Crear source borders image
    glowSrcBorders[i].create_image(
        glowSrc[i].getSize().width + kernelSize - 1,
        glowSrc[i].getSize().height + kernelSize - 1);

    if (repeat_borders) {
      // Replicate borders or glowSrc
      ippiCopyReplicateBorder_32f_C1R(
          glowSrc[i].getImagePtr(), glowSrc[i].getRowSizeBytes(),
          glowSrc[i].getSize(), glowSrcBorders[i].getImagePtr(),
          glowSrcBorders[i].getRowSizeBytes(), glowSrcBorders[i].getSize(),
          maxSize - 1, maxSize - 1);
    }
    else {
      // Copy constant boarder
      ippiCopyConstBorder_32f_C1R(
          glowSrc[i].getImagePtr(), glowSrc[i].getRowSizeBytes(),
          glowSrc[i].getSize(), glowSrcBorders[i].getImagePtr(),
          glowSrcBorders[i].getRowSizeBytes(), glowSrcBorders[i].getSize(),
          maxSize - 1, maxSize - 1, 0.0f);
    }

    // Do Convolution
    status = ippiConvValid_32f_C1R(
        glowSrcBorders[i].getImagePtr(), glowSrcBorders[i].getRowSizeBytes(),
        glowSrcBorders[i].getSize(), kernel.getImagePtr(),
        kernel.getRowSizeBytes(), kernel.getSize(), glow[i].getImagePtr(),
        glow[i].getRowSizeBytes());

    // Exit glow, render cancelled
    if (Status != 0)
      return false;
  }

  if (status != ippStsNoErr)
    Document->PrintF(
        ECONID_Error,
        "AFX Glow: Try decreasing image size, glow size, or quality \n",
        pluginName);

  {
    // Output to host
    Multithreader threader(Bounds(0, 0, width - 1, height - 1));
    threader.processInChunks(
        boost::bind(&ThisClass::setOutput, this, _1, boost::ref(glow)));
  }

  return true;
}

bool ThisClass::process_GPU() { return false; }
void ThisClass::createGauss(Bounds region, float** gaussContainer,
                            int* gaussSize, float* sigma)
{
  for (int y = region.y2; y >= region.y1; y--) {
    for (int glowNum = region.x2; glowNum >= region.x1; glowNum--) {
      if (y > gaussSize[glowNum] - 1)
        break;
      else {
        float a = 1.0f / (sigma[glowNum] * sqrt(2.0f * PI));
        gaussContainer[glowNum][y] =
            a * exp(-pow((float)(gaussSize[glowNum] - 1 - y), 2.0f) /
                    (2.0f * pow(sigma[glowNum], 2.0f)));
      }
    }
  }
}

void ThisClass::createKernel_CPU(Bounds region, ImageContainerA& kernel,
                                 float** gaussContainer, int* gaussSize,
                                 int maxSize, int kernelSize)
{
  float pixelAccumulator = 0;
  int gaussOffset        = 0;

  float* p = 0;

  for (int y = region.y1; y <= region.y2; y++) {
    p = (float*)&kernel.pixel_at(region.x1, y);

    for (int x = region.x1; x <= region.x2; x++) {
      // Set accumulator to zero
      pixelAccumulator = 0;

      // Loop through glow iterations
      for (int glowNum = iterations - 1; glowNum >= 0; glowNum--) {
        // Offset of largest gauss and current gauss
        gaussOffset = maxSize - gaussSize[glowNum];

        if (x >= gaussOffset && y >= gaussOffset) {
          pixelAccumulator += gaussContainer[glowNum][x - gaussOffset] *
                              gaussContainer[glowNum][y - gaussOffset];
        }
        else
          break;
      }

      // Write to all quadrants of the kernel image
      *p = pixelAccumulator;

      kernel.pixel_at(kernelSize - 1 - x, y) = pixelAccumulator;
      kernel.pixel_at(x, kernelSize - 1 - y) = pixelAccumulator;
      kernel.pixel_at(kernelSize - 1 - x, kernelSize - 1 - y) =
          pixelAccumulator;

      p++;
    }
  }
}
void ThisClass::noLicenseOutput(Bounds region)
{
  PixPtr ip(In_Img);
  PixPtr op(Out_Img);
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
