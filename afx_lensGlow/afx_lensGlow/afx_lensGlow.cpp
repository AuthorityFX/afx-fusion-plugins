// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright (C) 2012
//
// Authority FX, Inc.
// www.authorityfx.com

#include "stdafx.h"
#include "afx_lensGlow.h"

#define pluginGuid ("com.authorityfx:lensGlowPlugin")
#define pluginName ("AFX Lens Glow")
#define PLUGIN_LICENSE_NAME ("lens_glow")

bool ThisClass::pluginIsRegistered;
std::stringstream ThisClass::licenseStatus;

FuRegisterClass(COMPANY_ID ".afx_lensGlow", CT_Tool)
  //REG_TileID,        AFX_Convolve_Tile,
  REGS_Name,          pluginName,
  REGS_Category,        "Authority FX",
  REGS_OpIconString,      "ALG",
  REGS_OpDescription,      "Atmospheric lens glow with chromatic aberation",
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
  InIntensity = AddInput(
      "Intensity", "intensity", LINKID_DataType, CLSID_DataType_Number,
      INPID_InputControl, SLIDERCONTROL_ID, INP_MinAllowed, 0.0, INP_MaxAllowed,
      100.0, INP_MinScale, 0.0, INP_MaxScale, 2.0, INP_Default, 1.0, TAG_DONE);

  InGlowSize = AddInput("Size", "size", LINKID_DataType, CLSID_DataType_Number,
                        INPID_InputControl, SLIDERCONTROL_ID, INP_MinAllowed,
                        0.0, INP_MaxAllowed, 2.0, INP_MinScale, 0.0,
                        INP_MaxScale, 1.0, INP_Default, 0.25, TAG_DONE);

  InLockXY =
      AddInput("Lock X/Y Size", "lock_xy", LINKID_DataType,
               CLSID_DataType_Number, INPID_InputControl, CHECKBOXCONTROL_ID,
               INP_Default, 1.0, INP_DoNotifyChanged, TRUE, TAG_DONE);
  InXSize = AddInput("X Size", "x_size", LINKID_DataType, CLSID_DataType_Number,
                     INPID_InputControl, SLIDERCONTROL_ID, INP_MinAllowed, 0.0,
                     INP_MaxAllowed, 2.0, INP_MinScale, 0.0, INP_MaxScale, 2.0,
                     INP_Default, 1.0, TAG_DONE);
  InYSize = AddInput("Y Size", "y_size", LINKID_DataType, CLSID_DataType_Number,
                     INPID_InputControl, SLIDERCONTROL_ID, INP_MinAllowed, 0.0,
                     INP_MaxAllowed, 2.0, INP_MinScale, 0.0, INP_MaxScale, 2.0,
                     INP_Default, 1.0, TAG_DONE);

  InGlowSoftness = AddInput(
      "Softness", "softness", LINKID_DataType, CLSID_DataType_Number,
      INPID_InputControl, SLIDERCONTROL_ID, INP_MinAllowed, 0.0, INP_MaxAllowed,
      5.0, INP_MinScale, 0.0, INP_MaxScale, 1.0, INP_Default, 0.1, TAG_DONE);

  InThreshold = AddInput(
      "Threshold", "threshold", LINKID_DataType, CLSID_DataType_Number,
      INPID_InputControl, SLIDERCONTROL_ID, INP_MinAllowed, 0.0, INP_MaxAllowed,
      100.0, INP_MinScale, 0.0, INP_MaxScale, 1.0, INP_Default, 0.1, TAG_DONE);

  AddInput("RGB Scale", "RGBScale", LINKID_DataType, CLSID_DataType_Number,
           INPID_InputControl, LABELCONTROL_ID, LBLC_DropDownButton, TRUE,
           LBLC_NumInputs, 3, LBLC_NestLevel, 1, TAG_DONE);
  InRedScale = AddInput(
      "Red Scale", "red_scale", LINKID_DataType, CLSID_DataType_Number,
      INPID_InputControl, SLIDERCONTROL_ID, INP_MinAllowed, 0.0, INP_MaxAllowed,
      2.0, INP_MinScale, 0.5, INP_MaxScale, 1.5, INP_Default, 0.9, TAG_DONE);
  InGreenScale = AddInput(
      "Green Scale", "green_scale", LINKID_DataType, CLSID_DataType_Number,
      INPID_InputControl, SLIDERCONTROL_ID, INP_MinAllowed, 0.0, INP_MaxAllowed,
      2.0, INP_MinScale, 0.5, INP_MaxScale, 1.5, INP_Default, 1.0, TAG_DONE);
  InBlueScale = AddInput(
      "Blue Scale", "blue_scale", LINKID_DataType, CLSID_DataType_Number,
      INPID_InputControl, SLIDERCONTROL_ID, INP_MinAllowed, 0.0, INP_MaxAllowed,
      2.0, INP_MinScale, 0.5, INP_MaxScale, 1.5, INP_Default, 1.1, TAG_DONE);

  InGammaSpace =
      AddInput("In Gamma Space", "gamma_space", LINKID_DataType,
               CLSID_DataType_Number, INPID_InputControl, COMBOCONTROL_ID,
               CCS_AddString, "Linear", CCS_AddString, "sRGB / 2.2",
               INP_Integer, TRUE, INP_Default, 0.0, TAG_DONE);

  InCenterKernel =
      AddInput("Barycentric Shift", "barycentric_shift", LINKID_DataType,
               CLSID_DataType_Number, INPID_InputControl, CHECKBOXCONTROL_ID,
               INP_Default, 1.0, TAG_DONE);

  InRepeatBorders =
      AddInput("Repeat Borders", "repeat_borders", LINKID_DataType,
               CLSID_DataType_Number, INPID_InputControl, CHECKBOXCONTROL_ID,
               INP_Default, 1.0, TAG_DONE);

  // Add image
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

void ThisClass::clampOutput(Bounds region)
{
  PixPtr op(outImg);
  FltPixel p;

  for (int y = region.y1; y <= region.y2; y++) {
    op.GotoXY(region.x1, y);

    for (int x = region.x1; x <= region.x2; x++) {
      p = op;

      p.R = std::max<float>(p.R, 0.0f);
      p.G = std::max<float>(p.G, 0.0f);
      p.B = std::max<float>(p.B, 0.0f);
      p.A = 0.0f;

      op >>= p;
    }
  }
}

void ThisClass::NotifyChanged(Input* in, Parameter* param, TimeStamp time,
                              TagList* tags)
{
  if (param && in == InLockXY) {
    bool showXYControls = *((Number*)param) < 0.5;

    InXSize->SetAttrs(IC_Visible, showXYControls, TAG_DONE);
    InYSize->SetAttrs(IC_Visible, showXYControls, TAG_DONE);
  }

  BaseClass::NotifyChanged(in, param, time, tags);
}

void ThisClass::Process(Request* req)
{
  // Get input images
  inImg       = (Image*)InImage->GetValue(req);
  PreMask_Img = (Image*)InPreMask->GetValue(req);

  // Get paramters
  intensity      = *InIntensity->GetValue(req);
  glow_size      = *InGlowSize->GetValue(req);
  sizeX          = *InXSize->GetValue(req);
  sizeY          = *InYSize->GetValue(req);
  lock_XY        = *InLockXY->GetValue(req) > 0.5;
  glow_softness  = *InGlowSoftness->GetValue(req);
  red_scale      = *InRedScale->GetValue(req);
  green_scale    = *InGreenScale->GetValue(req);
  blue_scale     = *InBlueScale->GetValue(req);
  threshold      = *InThreshold->GetValue(req);
  center_kernel  = *InCenterKernel->GetValue(req) > 0.5;
  gamma_space    = *InGammaSpace->GetValue(req);
  repeat_borders = *InRepeatBorders->GetValue(req) >= 0.5;

  width  = inImg->Width;
  height = inImg->Height;

  outImg = NewImage(IMG_Like, inImg, TAG_DONE);

  if (pluginIsRegistered == true) {
    if (process_GPU()) {
    }
    else if (process_CPU()) {
      // Do Blur if GlowBlur is greater than 0
      if (glow_softness > 0.0f) {
        outImg->Blur(outImg, BLUR_Type, BT_Gaussian, BLUR_Alpha, FALSE,
                     BLUR_XSize, glow_softness / 9.0f, BLUR_YSize,
                     glow_softness / 9.0f, TAG_DONE);
      }

      Multithreader threader(Bounds(0, 0, inImg->Width - 1, inImg->Height - 1));
      threader.processInChunks(boost::bind(&ThisClass::clampOutput, this, _1));
    }
  }
  else {
    // Not licensed - output red
    Document->PrintF(ECONID_Error, "%s\n", licenseStatus.str().c_str());
    {
      Multithreader threader(Bounds(0, 0, inImg->Width - 1, inImg->Height - 1));
      threader.processInChunks(
          boost::bind(&ThisClass::noLicenseOutput, this, _1));
    }
  }

  OutImage->Set(req, outImg);
}

void ThisClass::grabInput(Bounds region, ImageContainerA* dest,
                          ImageContainerA* maskedDest)
{
  float color[3];
  float colorThresh[3];

  bool maskExists = (PreMask_Img != 0);
  PixPtr ip(inImg);
  PixPtr mp(maskExists ? PreMask_Img : inImg);

  FltPixel p;
  FltPixel m;

  float* p_dest_r = 0;
  float* p_dest_g = 0;
  float* p_dest_b = 0;

  float* p_maskedDest_r = 0;
  float* p_maskedDest_g = 0;
  float* p_maskedDest_b = 0;

  for (int y = region.y1; y <= region.y2; y++) {
    ip.GotoXY(region.x1, y);

    if (maskExists)
      mp.GotoXY(region.x1, y);

    p_dest_r = (float*)&dest[0].pixel_at(region.x1, y);
    p_dest_g = (float*)&dest[1].pixel_at(region.x1, y);
    p_dest_b = (float*)&dest[2].pixel_at(region.x1, y);

    p_maskedDest_r = (float*)&maskedDest[0].pixel_at(region.x1, y);
    p_maskedDest_g = (float*)&maskedDest[1].pixel_at(region.x1, y);
    p_maskedDest_b = (float*)&maskedDest[2].pixel_at(region.x1, y);

    for (int x = region.x1; x <= region.x2; x++) {
      p >>= ip;
      if (maskExists)
        m >>= mp;

      float maskAlpha = maskExists ? m.A : 1.0f;

      color[0] = p.R;
      color[1] = p.G;
      color[2] = p.B;

      for (int i = 0; i < 3; i++) color[i] = max(0.0f, color[i]);

      float luma = 0.3f * color[0] + 0.59f * color[1] + 0.11f * color[2];

      // Apply threshold
      if (luma < threshold)
        colorThresh[0] = colorThresh[1] = colorThresh[2] = 0.0f;
      else
        for (int i = 0; i < 3; i++) colorThresh[i] = color[i];

      if (luma >= threshold && luma < 1.0 && threshold > 0.0) {
        colorThresh[0] *= (luma - threshold) / ((1.0f - threshold) * luma);
        colorThresh[1] *= (luma - threshold) / ((1.0f - threshold) * luma);
        colorThresh[2] *= (luma - threshold) / ((1.0f - threshold) * luma);
      }

      for (int i = 0; i < 3; i++) {
        color[i]       = max(0.0f, color[i]);
        colorThresh[i] = max(0.0f, colorThresh[i]);
      }

      // If input gamma space is linear, convert to 2.2
      if (gamma_space == 0) {
        for (int i = 0; i < 3; i++)
          colorThresh[i] = pow(colorThresh[i], 1.0f / 2.2f);
      }
      else {
        for (int i = 0; i < 3; i++) color[i] = pow(color[i], 2.2f);
      }

      *p_dest_r = (Ipp32f)color[0];
      *p_dest_g = (Ipp32f)color[1];
      *p_dest_b = (Ipp32f)color[2];

      *p_maskedDest_r = (Ipp32f)(colorThresh[0] * maskAlpha);
      *p_maskedDest_g = (Ipp32f)(colorThresh[1] * maskAlpha);
      *p_maskedDest_b = (Ipp32f)(colorThresh[2] * maskAlpha);

      p_dest_r++;
      p_dest_g++;
      p_dest_b++;

      p_maskedDest_r++;
      p_maskedDest_g++;
      p_maskedDest_b++;
    }
  }
}

void ThisClass::setOutput(Bounds region, ImageContainerA* output,
                          ImageContainerA* src)
{
  PixPtr op(outImg);
  FltPixel p;

  int xOffset[3];
  int yOffset[3];
  bool subsampleX[3];
  bool subsampleY[3];

  float color[3];

  // Calculate vertical and horizontal shift of output. Subsample if there is a
  // need to shift by half a pixel
  for (int i = 0; i < 3; i++) {
    // Divide the difference in dimensions between output and src by 2 (bitshift
    // left by 1)
    xOffset[i] = (output[i].getSize().width - src[i].getSize().width) >> 1;
    yOffset[i] = (output[i].getSize().height - src[i].getSize().height) >> 1;

    // Bitwise and with 1 to determine if the value is odd
    subsampleX[i] = ((output[i].getSize().width - src[i].getSize().width) & 1);
    subsampleY[i] =
        ((output[i].getSize().height - src[i].getSize().height) & 1);
  }

  for (int y = region.y1; y <= region.y2; y++) {
    op.GotoXY(region.x1, y);

    for (int x = region.x1; x <= region.x2; x++) {
      // Read pixel values from output, taking into account subsampling if
      // necessary
      for (int i = 0; i < 3; i++) {
        int cx = x + xOffset[i];
        int cy = y + yOffset[i];

        if (subsampleX[i] && subsampleY[i]) {
          color[i] =
              (output[i].pixel_at(cx, cy) + output[i].pixel_at(cx + 1, cy) +
               output[i].pixel_at(cx, cy + 1) +
               output[i].pixel_at(cx + 1, cy + 1)) *
              0.25f;
        }
        else if (subsampleX[i])
          color[i] =
              (output[i].pixel_at(cx, cy) + output[i].pixel_at(cx + 1, cy)) *
              0.5f;
        else if (subsampleY[i])
          color[i] =
              (output[i].pixel_at(cx, cy) + output[i].pixel_at(cx, cy + 1)) *
              0.5f;
        else
          color[i] = output[i].pixel_at(cx, cy);

        color[i] = std::max<float>(0.0f, color[i]);

        if (gamma_space == 0) {
          color[i] = pow(color[i], 2.2f);
        }

        color[i] *= intensity;
      }

      p.R = std::max<float>(color[0], 0.0f);
      p.G = std::max<float>(color[1], 0.0f);
      p.B = std::max<float>(color[2], 0.0f);
      p.A = 0.0f;

      op >>= p;
    }
  }
}

bool ThisClass::process_CPU()
{
  if (lock_XY)
    sizeX = sizeY = 1.0;

  // set progress bar
  ThisClass::ProgressScale = 10;
  ThisClass::ProgressCount = 1;

  ImageContainerA src[3];
  ImageContainerA croppedSrc[3];
  ImageContainerA maskedSrc[3];
  ImageContainerA kernel[3];
  ImageContainerA srcRepeatedBorders[3];
  ImageContainerA convolved[3];
  ImageContainerA output[3];
  ImageContainerA centredKernel[3];
  ImageContainerA blurred[3];

  // Allocate memory for images
  for (int i = 0; i < 3; i++) {
    src[i].create_image(width, height);
    maskedSrc[i].create_image(width, height);
  }

  {
    Multithreader threader(Bounds(0, 0, width - 1, height - 1));
    threader.processInChunks(
        boost::bind(&ThisClass::grabInput, this, _1, src, maskedSrc));
  }

  float kwFactor, khFactor;
  kwFactor = width * glow_size * sizeX * 0.5f;
  khFactor = height * glow_size * sizeY * 0.5f;

  // Crop kernel if center kernel is enabled
  if (center_kernel) {
    BoundaryCalculator bc;
    Bounds cropBounds = bc.process(src);

    for (int i = 0; i < 3; i++)
      croppedSrc[i].create_image(cropBounds.x2 - cropBounds.x1 + 1,
                                 cropBounds.y2 - cropBounds.y1 + 1);

    ImageCropper cropper;
    cropper.process(src, croppedSrc, 3, cropBounds);

    // Calculate kernel dimensions
    kwFactor = croppedSrc[0].getSize().width * glow_size * sizeX * 0.5f;
    khFactor = croppedSrc[0].getSize().height * glow_size * sizeY * 0.5f;
  }

  // Calculate size of kernel. Ensure kernel is no smaller than 1x1 pixels
  kernel[0].create_image(max((int)(kwFactor * red_scale + 0.5f), 1),
                         max((int)(khFactor * red_scale + 0.5f), 1));
  kernel[1].create_image(max((int)(kwFactor * green_scale + 0.5f), 1),
                         max((int)(khFactor * green_scale + 0.5f), 1));
  kernel[2].create_image(max((int)(kwFactor * blue_scale + 0.5f), 1),
                         max((int)(khFactor * blue_scale + 0.5f), 1));

  // Resize kernel
  for (int i = 0; i < 3; i++) {
    if (center_kernel)
      resizeKernel(croppedSrc[i], kernel[i]);
    else
      resizeKernel(src[i], kernel[i]);
  }

  // Normalize the kernel
  for (int i = 0; i < 3; i++) {
    // Added a max here to prevent divide by zero if kernel sum is small.
    Ipp64f kernel_sum;
    ippiSum_32f_C1R(kernel[i].getImagePtr(), kernel[i].getRowSizeBytes(),
                    kernel[i].getSize(), &kernel_sum, ippAlgHintAccurate);
    ippiMulC_32f_C1IR(1.0f / std::max<float>((float)kernel_sum, 0.05f),
                      kernel[i].getImagePtr(), kernel[i].getRowSizeBytes(),
                      kernel[i].getSize());

    // Ensure that a 1x1 kernel has a pixel value of 1.0
    if (kernel[i].getSize().width == 1 && kernel[i].getSize().height == 1)
      kernel[i].pixel_at(0, 0) = 1.0f;
  }

  // Center the kernel
  if (center_kernel) {
    double centroidX = 0.0, centroidY = 0.0;
    CentroidCalculator cc(src, centroidX, centroidY);

    for (int i = 0; i < 3; i++) {
      float cx = ((float)centroidX / src[i].getSize().width) *
                 kernel[i].getSize().width;
      float cy = ((float)centroidY / src[i].getSize().height) *
                 kernel[i].getSize().height;
      float px        = kernel[i].getSize().width - 2.0f * cx;
      float py        = kernel[i].getSize().height - 2.0f * cy;
      int paddingLeft = (int)max(0.0f, px + 0.5f);
      int paddingTop  = (int)max(0.0f, py + 0.5f);
      int hPadding    = (int)(abs(px) + 0.5f);
      int vPadding    = (int)(abs(py) + 0.5f);
      centredKernel[i].create_image(kernel[i].getSize().width + hPadding,
                                    kernel[i].getSize().height + vPadding);

      recenterKernel(kernel[i], centredKernel[i], paddingLeft, paddingTop);

      kernel[i].dispose();
    }
  }

  // Extend borders of glow source
  for (int i = 0; i < 3; i++) {
    // Bitwise and with -2 to subtract 1 if it's an odd number
    if (center_kernel) {
      srcRepeatedBorders[i].create_image(
          width + (centredKernel[i].getSize().width & -2),
          height + (centredKernel[i].getSize().height & -2));

      extendGlowSourceBorders(maskedSrc[i], srcRepeatedBorders[i],
                              centredKernel[i].getSize());
    }
    else {
      srcRepeatedBorders[i].create_image(
          width + (kernel[i].getSize().width & -2),
          height + (kernel[i].getSize().height & -2));

      extendGlowSourceBorders(maskedSrc[i], srcRepeatedBorders[i],
                              kernel[i].getSize());
    }

    maskedSrc[i].dispose();
  }

  // Determine glow blur radius and kernel size
  float blurSigma = min(width, height) * glow_softness / 12.0f;

  // Perform convolution and soften the glow
  for (int i = 0; i < 3; i++) {
    if (center_kernel) {
      // Allocate memory for convolution result
      convolved[i].create_image(srcRepeatedBorders[i].getSize().width -
                                    centredKernel[i].getSize().width + 1,
                                srcRepeatedBorders[i].getSize().height -
                                    centredKernel[i].getSize().height + 1);

      applyConvolution(srcRepeatedBorders[i], centredKernel[i], convolved[i]);
    }
    else {
      // Allocate memory for convolution result
      convolved[i].create_image(
          srcRepeatedBorders[i].getSize().width - kernel[i].getSize().width + 1,
          srcRepeatedBorders[i].getSize().height - kernel[i].getSize().height +
              1);

      applyConvolution(srcRepeatedBorders[i], kernel[i], convolved[i]);
    }

    // Deallocate images that we don't need anymore
    srcRepeatedBorders[i].dispose();

    if (center_kernel)
      centredKernel[i].dispose();
    else
      kernel[i].dispose();
  }

  {
    Multithreader threader(Bounds(0, 0, width - 1, height - 1));
    threader.processInChunks(
        boost::bind(&ThisClass::setOutput, this, _1, convolved, src));
  }

  return true;
}

bool ThisClass::process_GPU() { return false; }

void ThisClass::resizeKernel(ImageContainerA& input, ImageContainerA& output)
{
  // Define source and destination regions
  IppiRect srcRoi;
  srcRoi.x = srcRoi.y = 0;
  srcRoi.width        = input.getSize().width;
  srcRoi.height       = input.getSize().height;

  IppiRect dstRoi;
  dstRoi.x = dstRoi.y = 0;
  dstRoi.width        = output.getSize().width;
  dstRoi.height       = output.getSize().height;

  int interpolationMethod;
  // Determine interpolation method
  if ((float)dstRoi.width / (float)srcRoi.width >= 1 ||
      (float)dstRoi.height / (float)srcRoi.height >= 1)
    interpolationMethod = IPPI_INTER_LINEAR;
  else
    interpolationMethod = IPPI_INTER_SUPER;

  // Calculate size of buffer to be used by the resize method
  int kernelResizeBufferLength;
  ippiResizeGetBufSize(srcRoi, dstRoi, 1, interpolationMethod,
                       &kernelResizeBufferLength);

  Ipp8u* kernelResizeBuffer = new Ipp8u[kernelResizeBufferLength];

  // Resize the kernel
  IppStatus stat = ippiResizeSqrPixel_32f_C1R(
      input.getImagePtr(), input.getSize(), input.getRowSizeBytes(), srcRoi,
      output.getImagePtr(), output.getRowSizeBytes(), dstRoi,
      (double)output.getSize().width / (double)input.getSize().width,
      (double)output.getSize().height / (double)input.getSize().height, 0, 0,
      interpolationMethod, kernelResizeBuffer);

  delete[] kernelResizeBuffer;
}

void ThisClass::recenterKernel(ImageContainerA& input, ImageContainerA& output,
                               int paddingLeft, int paddingTop)
{
  ippiCopyConstBorder_32f_C1R(input.getImagePtr(), input.getRowSizeBytes(),
                              input.getSize(), output.getImagePtr(),
                              output.getRowSizeBytes(), output.getSize(),
                              paddingTop, paddingLeft, 0.0);
}

void ThisClass::extendGlowSourceBorders(ImageContainerA& input,
                                        ImageContainerA& output,
                                        IppiSize kernelSize)
{
  // >> bit shift to right - divide by 2

  // Calculate padding
  int paddingTop  = (output.getSize().height - input.getSize().height) >> 1;
  int paddingLeft = (output.getSize().width - input.getSize().width) >> 1;

  if (repeat_borders) {
    ippiCopyReplicateBorder_32f_C1R(
        input.getImagePtr(), input.getRowSizeBytes(), input.getSize(),
        output.getImagePtr(), output.getRowSizeBytes(), output.getSize(),
        paddingTop, paddingLeft);
  }
  else {
    ippiCopyConstBorder_32f_C1R(input.getImagePtr(), input.getRowSizeBytes(),
                                input.getSize(), output.getImagePtr(),
                                output.getRowSizeBytes(), output.getSize(),
                                paddingTop, paddingLeft, 0.0f);
  }
}

void ThisClass::applyConvolution(ImageContainerA& input,
                                 ImageContainerA& kernel,
                                 ImageContainerA& output)
{
  IppStatus stat = ippiConvValid_32f_C1R(
      input.getImagePtr(), input.getRowSizeBytes(), input.getSize(),
      kernel.getImagePtr(), kernel.getRowSizeBytes(), kernel.getSize(),
      output.getImagePtr(), output.getRowSizeBytes());
}

void ThisClass::softenGlow(ImageContainerA& input, ImageContainerA& output,
                           float blurSigma)
{
  // Kernel size is 6*sigma, must be odd and must be > 3
  int blurKernelSize = max(((int)ceil(blurSigma * 6.0f)) | 1, 3);
  int gaussBufferSize;

  // Preparations for blur
  ippiFilterGaussGetBufferSize_32f_C1R(input.getSize(), blurKernelSize,
                                       &gaussBufferSize);
  Ipp8u* gaussBuffer = new Ipp8u[gaussBufferSize];

  // Apply the gaussian blur
  ippiFilterGaussBorder_32f_C1R(
      input.getImagePtr(), input.getRowSizeBytes(), output.getImagePtr(),
      output.getRowSizeBytes(), output.getSize(), blurKernelSize,
      (Ipp32f)blurSigma, ippBorderRepl, 0.0, gaussBuffer);

  delete[] gaussBuffer;
}

void ThisClass::noLicenseOutput(Bounds region)
{
  PixPtr ip(inImg);
  PixPtr op(outImg);
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
