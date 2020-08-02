// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright (C) 2012
//
// Authority FX, Inc.
// www.authorityfx.com

#include "stdafx.h"
#include "afx_zDefocus.h"

#define pluginGuid ("com.authorityfx:zDefocusPlugin")
#define pluginName ("AFX Z-Defocus")
#define PLUGIN_LICENSE_NAME ("z_defocus")

bool ThisClass::pluginIsRegistered;
std::stringstream ThisClass::licenseStatus;

FuRegisterClass(COMPANY_ID ".afx_zDefocus", CT_Tool)
  //REG_TileID,          AFX_Convolve_Tile,
  REGS_Name,          pluginName,
  REGS_Category,        "Authority FX",
  REGS_OpIconString,      "AZDe",
  REGS_OpDescription,      "Applies a lens blur according to a depth map",
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
  InShape = AddInput(
      "Shape", "Shape", LINKID_DataType, CLSID_DataType_Number,
      INPID_InputControl, COMBOCONTROL_ID, CCS_AddString, "Triangle (3)",
      CCS_AddString, "Square (4)", CCS_AddString, "Pentagon (5)", CCS_AddString,
      "Hexagon (6)", CCS_AddString, "Heptagon (7)", CCS_AddString,
      "Octagon (8)", CCS_AddString, "Nonagon (9)", CCS_AddString,
      "Decagon (10)", CCS_AddString, "Circle", INP_Default, 3.0, TAG_DONE);

  InSize = AddInput("Aperture Size", "ApertureSize", LINKID_DataType,
                    CLSID_DataType_Number, INPID_InputControl, SLIDERCONTROL_ID,
                    INP_MinAllowed, 0.0, INP_MaxAllowed, 2.0, INP_MinScale, 0.0,
                    INP_MaxScale, 1.0, INP_Default, 0.1, TAG_DONE);

  InRotation = AddInput("Aperture Blade Rotation", "BladeRotation",
                        LINKID_DataType, CLSID_DataType_Number,
                        INPID_InputControl, SCREWCONTROL_ID, INP_Default, 0.0,
                        INP_MinScale, 0.0, INP_MaxScale, 90.0, TAG_DONE);

  InCurvature =
      AddInput("Aperture Blade Curvature", "BladeCurvature", LINKID_DataType,
               CLSID_DataType_Number, INPID_InputControl, SLIDERCONTROL_ID,
               INP_MinAllowed, -1.0, INP_MaxAllowed, 1.0, INP_MinScale, -1.0,
               INP_MaxScale, 1.0, INP_Default, 0.0, TAG_DONE);

  InDistance = AddInput("Focal Distance", "FocalDistance", LINKID_DataType,
                        CLSID_DataType_Number, INPID_InputControl,
                        SLIDERCONTROL_ID, INP_MinAllowed, 0.0, INP_MinScale,
                        0.0, INP_MaxScale, 10.0, INP_Default, 5.0, TAG_DONE);

  AddInput("Lens Tilt", "LensTilt", LINKID_DataType, CLSID_DataType_Number,
           INPID_InputControl, LABELCONTROL_ID, LBLC_DropDownButton, TRUE,
           LBLC_NumInputs, 2, LBLC_NestLevel, 1, TAG_DONE);
  InTiltHoriz =
      AddInput("Horizontal Tilt", "HorizontalTilt", LINKID_DataType,
               CLSID_DataType_Number, INPID_InputControl, SLIDERCONTROL_ID,
               INP_MinAllowed, -1.0, INP_MaxAllowed, 1.0, INP_MinScale, -1.0,
               INP_MaxScale, 1.0, INP_Default, 0.0, TAG_DONE);
  InTiltVert =
      AddInput("Vertical Tilt", "VerticalTilt", LINKID_DataType,
               CLSID_DataType_Number, INPID_InputControl, SLIDERCONTROL_ID,
               INP_MinAllowed, -1.0, INP_MaxAllowed, 1.0, INP_MinScale, -1.0,
               INP_MaxScale, 1.0, INP_Default, 0.0, TAG_DONE);

  InQuality =
      AddInput("Quality", "Quality", LINKID_DataType, CLSID_DataType_Number,
               INPID_InputControl, SLIDERCONTROL_ID, INP_Integer, TRUE,
               INP_MinAllowed, 1.0, INP_MaxAllowed, 128.0, INP_MinScale, 1.0,
               INP_MaxScale, 40.0, INP_Default, 10.0, TAG_DONE);

  InDistribution =
      AddInput("Layer Distribution", "LayerDistribution", LINKID_DataType,
               CLSID_DataType_Number, INPID_InputControl, SLIDERCONTROL_ID,
               INP_MinAllowed, 0.01, INP_MaxAllowed, 100.0, INP_MinScale, 0.2,
               INP_MaxScale, 5.0, INP_Default, 1.0, TAG_DONE);

  InDepthMapChannel = AddInput(
      "Depth Map Channel", "DepthMapChannel", LINKID_DataType,
      CLSID_DataType_Number, INPID_InputControl, COMBOCONTROL_ID, CCS_AddString,
      "Alpha", CCS_AddString, "Intensity", CCS_AddString, "Incoming Z",
      INP_Default, 2.0, INP_DoNotifyChanged, TRUE, TAG_DONE);

  InDepthMapRangeLow = AddInput(
      "Depth Map Range Low", "DMRangeLow", LINKID_DataType,
      CLSID_DataType_Number, INPID_InputControl, RANGECONTROL_ID,
      IC_ControlGroup, 10, IC_ControlID, RANGECONTROL_LOW, INP_MinAllowed, 0.0,
      INP_MinScale, 0.0, INP_MaxScale, 5.0, INP_Default, 0.0, TAG_DONE);
  InDepthMapRangeHigh = AddInput(
      "Depth Map Range High", "DMRangeHigh", LINKID_DataType,
      CLSID_DataType_Number, INPID_InputControl, RANGECONTROL_ID,
      IC_ControlGroup, 10, IC_ControlID, RANGECONTROL_HIGH, INP_MinAllowed, 0.0,
      INP_MinScale, 0.0, INP_MaxScale, 5.0, INP_Default, 5.0, TAG_DONE);

  InLayerInter =
      AddInput("Layer Interpolation", "LayerInterpolation", LINKID_DataType,
               CLSID_DataType_Number, INPID_InputControl, CHECKBOXCONTROL_ID,
               INP_Default, 0.0, TAG_DONE);

  InLockLayers = AddInput("Lock Layers", "LockLayers", LINKID_DataType,
                          CLSID_DataType_Number, INPID_InputControl,
                          CHECKBOXCONTROL_ID, INP_Default, 1.0, TAG_DONE);

  InOuputMatte = AddInput("Output Matte", "OutputMatte", LINKID_DataType,
                          CLSID_DataType_Number, INPID_InputControl,
                          CHECKBOXCONTROL_ID, INP_Default, 0.0, TAG_DONE);

  // Add image
  InImage = AddInput("Input", "Input", LINKID_DataType, CLSID_DataType_Image,
                     LINK_Main, 1, TAG_DONE);

  InDepthMap = AddInput("Depth Map", "DepthMap", LINKID_DataType,
                        CLSID_DataType_Image, INP_Priority, MASK_PRI, LINK_Main,
                        2, INP_Required, TRUE, TAG_DONE);

  // Output image
  OutImage = AddOutput("Output", "Output", LINKID_DataType,
                       CLSID_DataType_Image, LINK_Main, 1, TAG_DONE);
}

void ThisClass::NotifyChanged(Input* in, Parameter* param, TimeStamp time,
                              TagList* tags)
{
  Number* depthMapChannel = (Number*)InDepthMapChannel->GetSourceAttrs(
      time, NULL, REQF_SecondaryTime, REQF_SecondaryTime);

  // Depth map input is not required if we selected the incomming Z channel
  if (depthMapChannel && in == InDepthMapChannel) {
    int dmc = (int)(*depthMapChannel + 0.5);

    if (dmc == 2) {
      InDepthMap->SetAttrs(INP_Required, FALSE, TAG_DONE);
    }
    else {
      InDepthMap->SetAttrs(INP_Required, TRUE, TAG_DONE);
    }
  }

  if (depthMapChannel)
    depthMapChannel->Recycle();

  BaseClass::NotifyChanged(in, param, time, tags);
}

inline double depthTransform(double x, double density)
{
  double alpha = density;  // Tightness of the layer map curve
  return ((x >= 0) ? 1.0 : -1.0) * (1.0 - 1.0 / (abs(alpha * x) + 1.0));
}

inline double inverseDepthTransform(double x, double density)
{
  double alpha = density;
  return ((x >= 0) ? 1.0 : -1.0) / alpha * (1.0 / (1.0 - abs(x)) - 1.0);
}

void ThisClass::Process(Request* req)
{
  // Get input images
  imgInput    = (Image*)InImage->GetValue(req);
  imgDepthMap = (Image*)InDepthMap->GetValue(req);

  outputMatte = (int)*InOuputMatte->GetValue(req) >= 0.5;

  if (outputMatte) {
    imgOutput =
        NewImage(IMG_Like, imgInput, IMG_Channel, CHAN_COVERAGE, TAG_DONE);
  }
  else {
    imgOutput = NewImage(IMG_Like, imgInput, TAG_DONE);
  }

  width  = imgInput->Width;
  height = imgInput->Height;

  shape              = (int)*InShape->GetValue(req);
  curvature          = *InCurvature->GetValue(req);
  radius             = *InSize->GetValue(req);
  rotation           = *InRotation->GetValue(req);
  distance           = *InDistance->GetValue(req);
  density            = *InDistribution->GetValue(req);
  quality            = (int)*InQuality->GetValue(req);
  lockLayers         = *InLockLayers->GetValue(req) >= 0.5;
  layerInterpolation = *InLayerInter->GetValue(req) >= 0.5;
  horizTilt          = (float)*InTiltHoriz->GetValue(req);
  vertTilt           = (float)*InTiltVert->GetValue(req);
  zChannel           = (int)(*InDepthMapChannel->GetValue(req) + 0.5);
  depthMapRangeLow   = *InDepthMapRangeLow->GetValue(req);
  depthMapRangeHigh  = *InDepthMapRangeHigh->GetValue(req);

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

bool ThisClass::process_GPU() { return false; }

bool ThisClass::process_CPU()
{
  // set progress bar
  ThisClass::ProgressCount = 1;

  if (Status != STATUS_OK)
    false;

  double maxKernelRadius = std::min<int>(width, height) * 0.1f * (float)radius;

  ImageContainerA layerResult[6];
  ImageContainerA finalResult[4];

  for (int i = 0; i < 6; i++) layerResult[i].create_image(width, height);
  for (int i = 0; i < 5; i++) finalResult[i].create_image(width, height);

  Bounds bounds;
  bounds.x1 = bounds.y1 = 0;

  // Calculate range of depth map values
  // Deprecated. This is a fake range. It does not limit the kernel size, but it
  // does have an effect on kernel sizes, so don't change it.
  float depthMin = depthMapRangeLow, depthMax = depthMapRangeHigh;
  IppiSize roi;
  roi.width  = imgDepthMap->Width;
  roi.height = imgDepthMap->Height;
  // sippiMinMax_32f_C1R((Ipp32f*)maskImage->getPixelData(),
  // maskImage->getRowBytes(), roi, &depthMin, &depthMax);

  ImageContainerA matte;
  ImageContainerA preMatte;
  matte.create_image(width, height);
  preMatte.create_image(width, height);

  if (outputMatte)
    memset(matte.getImagePtr(), 0,
           matte.getRowSizeBytes() * matte.getSize().height);

  for (int layerNum = 0; layerNum < quality; layerNum++) {
    double upperLayerThreshold;
    double lowerLayerThreshold;
    double blurAmount;

    if (Status != STATUS_OK)
      return false;

    // Calculate layer thresholds
    if (lockLayers) {
      upperLayerThreshold =
          depthMax - (depthMax - depthMin) * ((double)layerNum / quality);
      lowerLayerThreshold =
          depthMax - (depthMax - depthMin) * ((double)(layerNum + 1) / quality);
      blurAmount =
          abs((lowerLayerThreshold + upperLayerThreshold) * 0.5 - distance) /
          (depthMax - depthMin);
    }
    else {
      upperLayerThreshold =
          depthMax - (depthMax - depthMin) *
                         ((double)(layerNum) + ((quality % 2) ? 0.0 : 1.0)) /
                         (double)(quality | 1);
      lowerLayerThreshold =
          depthMax - (depthMax - depthMin) *
                         ((double)(layerNum) + ((quality % 2) ? 1.0 : 2.0)) /
                         (double)(quality | 1);
      blurAmount =
          abs(inverseDepthTransform(
              (lowerLayerThreshold + upperLayerThreshold) * 0.5, density)) /
          (depthMax - depthMin);
    }

    if (Status != STATUS_OK)
      return false;

    double kernelRadius = std::max<double>(
        maxKernelRadius * std::min<double>(blurAmount, 1.5), 0.75);

    // Calculate kernel and image sizes
    int kernelSizeX = ((int)(kernelRadius * 2) + 1) | 1;
    int kernelSizeY = ((int)(kernelRadius * 2) + 1) | 1;

    int imagePaddingX = kernelSizeX / 2;
    int imagePaddingY = kernelSizeY / 2;

    // Create kernel
    ImageContainerA kernel;
    kernel.create_image(kernelSizeX, kernelSizeY);
    createKernel(kernel, kernelRadius, curvature, rotation, shape);

    if (Status != STATUS_OK)
      return false;

    // Create image planes.
    // Planes 0 through 2 are the RGB channels
    // Plane 3 is the alpha channel
    // Plane 4 is the layer mask
    // Plane 5 is the inpainting mask
    // Plane 6 is the layer's matte
    ImageContainerA source[7];
    ImageContainerA temp;

    for (int i = 0; i < 7; i++)
      source[i].create_image(width + 2 * imagePaddingX,
                             height + 2 * imagePaddingY);

    temp.create_image(width + 2 * imagePaddingX, height + 2 * imagePaddingY);

    bounds.x2 = width + 2 * imagePaddingX - 1;
    bounds.y2 = height + 2 * imagePaddingY - 1;

    bool layerContainsAnything = true;

    Multithreader mtgi(bounds);
    mtgi.processInChunks(boost::bind(&ThisClass::grabInput, this, _1, source,
                                     lowerLayerThreshold, upperLayerThreshold,
                                     layerNum, &layerContainsAnything));

    if (Status != STATUS_OK)
      return false;

    if (layerContainsAnything) {
      /////////////////////
      // Do in painting here
      /////////////////////

      // Apply box blur to layer masks for layer inerpolation
      if (layerInterpolation) {
        int boxBlurSize = (int)kernelRadius + 4;
        bounds.x2       = width - 1;
        bounds.y2       = height - 1;
        Multithreader mt1(bounds);
        mt1.processInChunks(boost::bind(&ThisClass::boxBlurHoriz, this, _1,
                                        &source[4], &temp, boxBlurSize));
        bounds.x2 = height - 1;
        bounds.y2 = width - 1;
        Multithreader mt2(bounds);
        mt2.processInChunks(boost::bind(&ThisClass::boxBlurVert, this, _1,
                                        &temp, &source[4], boxBlurSize));
      }

      if (Status != STATUS_OK)
        return false;

      // Perform bluring
      for (int i = 0; i < 5; i++) {
        ippiConvValid_32f_C1R(source[i].getImagePtr(),
                              source[i].getRowSizeBytes(), source[i].getSize(),
                              kernel.getImagePtr(), kernel.getRowSizeBytes(),
                              kernel.getSize(), layerResult[i].getImagePtr(),
                              layerResult[i].getRowSizeBytes());
      }

      if (outputMatte) {
        ippiConvValid_32f_C1R(source[6].getImagePtr(),
                              source[6].getRowSizeBytes(), source[6].getSize(),
                              kernel.getImagePtr(), kernel.getRowSizeBytes(),
                              kernel.getSize(), layerResult[5].getImagePtr(),
                              layerResult[5].getRowSizeBytes());
      }

      if (Status != STATUS_OK)
        return false;

      bounds.x2 = width - 1;
      bounds.y2 = height - 1;

      // Apply layer to final result buffer
      {
        float matteKernelRadius =
            pow(kernelRadius / std::min<int>(width, height), 2);
        Multithreader mt(bounds);
        mt.processInChunks(boost::bind(&ThisClass::blendLayerToOutput, this, _1,
                                       layerResult, finalResult, &matte,
                                       matteKernelRadius, layerNum));
      }
    }

    kernel.dispose();
    for (int i = 0; i < 7; i++) source[i].dispose();
    temp.dispose();
  }

  bounds.x2 = width - 1;
  bounds.y2 = height - 1;

  {
    Multithreader mt(bounds);
    mt.processInChunks(
        boost::bind(&ThisClass::setOutput, this, _1, finalResult, &matte));
  }

  return true;
}

void ThisClass::grabInput(Bounds region, ImageContainerA* dest,
                          double lowerLayerThreshold,
                          double upperLayerThreshold, int layerNum,
                          bool* layerContainsAnything)
{
  float color[4];
  float mask;

  bool maskExists = (imgDepthMap != 0);
  PixPtr ip(imgInput), mp(maskExists ? imgDepthMap : imgInput);
  FltPixel p;
  FltPixel m;

  Image* depthMap = imgDepthMap ? imgDepthMap : imgInput;

  int sourceWidth  = width;
  int sourceHeight = height;

  int mapWidth  = depthMap->Width;
  int mapHeight = depthMap->Height;

  int widthPadding  = (dest->getSize().width - sourceWidth) / 2;
  int heightPadding = (dest->getSize().height - sourceHeight) / 2;

  int maskWidthPadding  = (dest->getSize().width - mapWidth) / 2;
  int maskHeightPadding = (dest->getSize().height - mapHeight) / 2;

  for (int y = region.y1; y <= region.y2; y++) {
    int clampedY =
        std::max<int>(0, std::min<int>(sourceHeight - 1, y - heightPadding));
    int clampedMapY =
        std::max<int>(0, std::min<int>(mapHeight - 1, y - maskHeightPadding));
    ip.GotoXY(region.x1, clampedY);
    mp.GotoXY(region.x1, clampedMapY);

    for (int x = region.x1; x <= region.x2; x++) {
      int clampedX =
          std::max<int>(0, std::min<int>(sourceWidth - 1, x - widthPadding));

      if (x >= widthPadding && (x - widthPadding) < sourceWidth - 1)
        p >>= ip;
      else
        p = ip;

      if (maskExists) {
        if (x >= maskWidthPadding && (x - maskWidthPadding) < mapWidth - 1)
          m >>= mp;
        else
          m = mp;
      }

      color[0] = p.R;
      color[1] = p.G;
      color[2] = p.B;
      color[3] = p.A;

      if (!maskExists) {
        color[0] = 0.0f;
        color[1] = 0.0f;
        color[2] = 0.0f;
        color[3] = 0.0f;
      }

      switch (zChannel) {
        case 0:
        default:
          mask = m.A;
          break;
        case 1:
          mask = (m.R + m.G + m.B) / 3.0;
          break;
        case 2:
          mask = p.Z;
          break;
      }

      float originalMask = mask;

      if (mask == 0.0f)
        mask = depthMapRangeHigh;

      mask += (float)(horizTilt * (double)(x - sourceWidth / 2) /
                      (double)sourceWidth * 10.0);
      mask += (float)(vertTilt * (double)(y - sourceHeight / 2) /
                      (double)sourceHeight * 10.0);

      double transformedMask;

      if (lockLayers)
        transformedMask = mask;
      else
        transformedMask = depthTransform(mask - distance, density);

      float preMatteVal = radius * abs(mask - distance);

      // Store pixel values in memory
      if (transformedMask > upperLayerThreshold && layerNum != 0) {
        // This pixel is behind the current layer.
        // We don't care about it.
        dest[0].pixel_at(x, y) = color[0];
        dest[1].pixel_at(x, y) = color[1];
        dest[2].pixel_at(x, y) = color[2];
        dest[3].pixel_at(x, y) = color[3];
        dest[4].pixel_at(x, y) = 0.0f;
        dest[5].pixel_at(x, y) = 1.0f;
        dest[6].pixel_at(x, y) = preMatteVal;
      }
      else if (transformedMask < lowerLayerThreshold &&
               layerNum != quality - 1) {
        // This pixel is in front of the current layer.
        // We will need to do inpainting on the current layer at this pixel
        dest[0].pixel_at(x, y) = (Ipp32f)color[0];
        dest[1].pixel_at(x, y) = (Ipp32f)color[1];
        dest[2].pixel_at(x, y) = (Ipp32f)color[2];
        dest[3].pixel_at(x, y) = (Ipp32f)color[3];
        dest[4].pixel_at(x, y) = 1.0;
        dest[5].pixel_at(x, y) = 0.0f;
        dest[6].pixel_at(x, y) = preMatteVal;
      }
      else {
        if (!*layerContainsAnything)
          (*layerContainsAnything) = true;
        // This pixel is on the plane of the current layer
        dest[0].pixel_at(x, y) = (Ipp32f)color[0];
        dest[1].pixel_at(x, y) = (Ipp32f)color[1];
        dest[2].pixel_at(x, y) = (Ipp32f)color[2];
        dest[3].pixel_at(x, y) = (Ipp32f)color[3];
        dest[4].pixel_at(x, y) = 1.0;
        dest[5].pixel_at(x, y) = 0.0f;
        dest[6].pixel_at(x, y) = preMatteVal;
      }
    }
  }
}

void ThisClass::boxBlurHoriz(Bounds region, ImageContainerA* input,
                             ImageContainerA* output, int blurSize)
{
  double accumulator;
  double normalizingFactor = 1.0 / (blurSize * 2 + 1);
  int width                = input->getSize().width;

  for (int y = region.y1; y <= region.y2; y++) {
    accumulator = 0.0;

    for (int i = 0; i < blurSize * 2 + 1; i++) {
      accumulator +=
          input->pixel_at(std::max<int>(0, region.x1 - blurSize + i), y);
    }

    for (int x = region.x1; x <= region.x2; x++) {
      output->pixel_at(x, y) = (float)(accumulator * normalizingFactor);
      accumulator +=
          input->pixel_at(std::min<int>(width - 1, x + blurSize + 1), y) -
          input->pixel_at(std::max<int>(0, x - blurSize), y);
    }
  }
}

void ThisClass::boxBlurVert(Bounds region, ImageContainerA* input,
                            ImageContainerA* output, int blurSize)
{
  double accumulator;
  double normalizingFactor = 1.0 / (blurSize * 2 + 1);
  int height               = input->getSize().height;

  for (int x = region.y1; x < region.y2; x++) {
    accumulator = 0.0;

    for (int i = 0; i < blurSize * 2 + 1; i++) {
      accumulator +=
          input->pixel_at(x, std::max<int>(0, region.x1 - blurSize + i));
    }

    for (int y = region.x1; y < region.x2; y++) {
      output->pixel_at(x, y) = (float)(accumulator * normalizingFactor);
      accumulator +=
          input->pixel_at(x, std::min<int>(height - 1, y + blurSize + 1)) -
          input->pixel_at(x, std::max<int>(0, y - blurSize));
    }
  }
}

void ThisClass::setOutput(Bounds region, ImageContainerA* src,
                          ImageContainerA* matte)
{
  PixPtr op(imgOutput);
  FltPixel p;

  for (int y = region.y1; y <= region.y2; y++) {
    op.GotoXY(region.x1, y);

    for (int x = region.x1; x <= region.x2; x++) {
      p = op;

      p.R = src[0].pixel_at(x, y);
      p.G = src[1].pixel_at(x, y);
      p.B = src[2].pixel_at(x, y);
      p.A = src[3].pixel_at(x, y);

      if (matte)
        p.Coverage = matte->pixel_at(x, y);

      op >>= p;
    }
  }
}

void ThisClass::createKernel(ImageContainerA& kernel, double circumradius,
                             double curvature, double rotation, int shape)
{
  memset(kernel.getImagePtr(), 0,
         kernel.getRowSizeBytes() * kernel.getSize().height);

  std::list<Edge> edgeList;

  double bladeCurveRadius = circumradius / curvature;
  int apertureNumBlades   = shape + 3;

  if (shape == 8) {
    apertureNumBlades = std::max<int>((int)(circumradius * PI) + 1, 15);
    curvature         = 0.0;
    rotation          = 0.0;
  }

  double inradius = circumradius * cos(PI / apertureNumBlades);

  Bounds outerSSBounds;
  Bounds innerSSBounds;
  outerSSBounds.x1 = outerSSBounds.x2 = outerSSBounds.y1 = outerSSBounds.y2 =
      -1;

  // Calculate inner boundary region where super sampling should not be applied
  //(a rectangle that inscribes the polygon)
  innerSSBounds.x1 =
      kernel.getSize().width / 2 - (int)(inradius / sqrtf(2.0f)) + 2;
  innerSSBounds.x2 =
      kernel.getSize().width / 2 + (int)(inradius / sqrtf(2.0f)) - 2;
  innerSSBounds.y1 =
      kernel.getSize().height / 2 - (int)(inradius / sqrtf(2.0f)) + 2;
  innerSSBounds.y2 =
      kernel.getSize().height / 2 + (int)(inradius / sqrtf(2.0f)) - 2;

  // Generate list of edges for the kernel polygon
  for (int i = 0; i < apertureNumBlades; i++) {
    float x1 =
        (float)(circumradius * cos(i * TPI / apertureNumBlades +
                                   (double)(rotation + 90.0) / 360.0 * TPI) +
                kernel.getSize().width * 0.5);
    float y1 =
        (float)(circumradius * sin(i * TPI / apertureNumBlades +
                                   (double)(rotation + 90.0) / 360.0 * TPI) +
                kernel.getSize().height * 0.5);

    float x2 =
        (float)(circumradius * cos((i + 1) * TPI / apertureNumBlades +
                                   (double)(rotation + 90.0) / 360.0 * TPI) +
                kernel.getSize().width * 0.5);
    float y2 =
        (float)(circumradius * sin((i + 1) * TPI / apertureNumBlades +
                                   (double)(rotation + 90.0) / 360.0 * TPI) +
                kernel.getSize().height * 0.5);

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Adding the code for curved aperture blades here

    if (curvature != 0.0) {
      Vector2D point1(x1, y1);
      Vector2D point2(x2, y2);

      Vector2D kernelCenter(kernel.getSize().width * 0.5,
                            kernel.getSize().height * 0.5);

      // Calculate the center of the blade arc
      Vector2D edgeBisector = (point2 - point1) * 0.5 + point1;
      Vector2D arcBisector =
          (edgeBisector - kernelCenter).normalize() * circumradius +
          kernelCenter;
      Vector2D bladeCurveCenter =
          (kernelCenter - arcBisector) * (1.0 / curvature - 1.0) + kernelCenter;

      // Calculate radial vectors from the blade arc center to the arc endpoints
      Vector2D radius1 = point1 - bladeCurveCenter;
      Vector2D radius2 = point2 - bladeCurveCenter;

      // Calculate length and starting position of the arc
      double bladeAngle =
          acos(Vector2D::dot(radius1.normalize(), radius2.normalize()));
      double bladeArcLength  = abs(bladeCurveRadius * bladeAngle);
      double bladeStartAngle = (curvature > 0.0) ? atan2(radius1.y, radius1.x)
                                                 : atan2(radius2.y, radius2.x);

      // Calculate how many subdivisions of the arc we should create (one for
      // every two pixels of arc length)
      int numBladeCurvePieces = std::max<int>((int)(bladeArcLength * 0.5), 1);

      // Vector2D bladeArcBisector = (edgeBisector - kernelCenter) *
      // (bladeCurveRadius / inradius) + bladeCurveCenter;
      Vector2D bladeArcBisector(
          radius1.norm() * cos(0.5 * bladeAngle + bladeStartAngle) +
              bladeCurveCenter.x,
          radius1.norm() * sin(0.5 * bladeAngle + bladeStartAngle) +
              bladeCurveCenter.y);
      double newInradius = (bladeArcBisector - kernelCenter).norm();

      innerSSBounds.x1 =
          kernel.getSize().width / 2 - (int)(newInradius / sqrtf(2.0f)) + 2;
      innerSSBounds.x2 =
          kernel.getSize().width / 2 + (int)(newInradius / sqrtf(2.0f)) - 2;
      innerSSBounds.y1 =
          kernel.getSize().height / 2 - (int)(newInradius / sqrtf(2.0f)) + 2;
      innerSSBounds.y2 =
          kernel.getSize().height / 2 + (int)(newInradius / sqrtf(2.0f)) - 2;

      if (kernel.getSize().width == 1 && kernel.getSize().height == 1) {
        innerSSBounds.x1 = innerSSBounds.y1 = 0;
        innerSSBounds.x2 = innerSSBounds.y2 = 1;
      }

      // Generate an edge for each small piece of the blade curve
      for (int j = 0; j < numBladeCurvePieces; j++) {
        float bx1 =
            (float)(radius1.norm() * cos(j * bladeAngle / numBladeCurvePieces +
                                         bladeStartAngle) +
                    bladeCurveCenter.x);
        float by1 =
            (float)(radius1.norm() * sin(j * bladeAngle / numBladeCurvePieces +
                                         bladeStartAngle) +
                    bladeCurveCenter.y);

        float bx2 = (float)(radius1.norm() *
                                cos((j + 1) * bladeAngle / numBladeCurvePieces +
                                    bladeStartAngle) +
                            bladeCurveCenter.x);
        float by2 = (float)(radius1.norm() *
                                sin((j + 1) * bladeAngle / numBladeCurvePieces +
                                    bladeStartAngle) +
                            bladeCurveCenter.y);

        if (by1 == by2)
          continue;

        float temp;

        // Swap points to make sure the first point has a lower y value
        if (by1 > by2) {
          temp = by1;
          by1  = by2;
          by2  = temp;
          temp = bx1;
          bx1  = bx2;
          bx2  = temp;
        }

        // Calculate outer boundary region where super sampling should be
        // applied (a rectangle that circumscribes the polygon)
        outerSSBounds.x1 = std::min<int>(outerSSBounds.x1, (int)bx1);
        outerSSBounds.x1 = std::min<int>(outerSSBounds.x1, (int)bx2);
        outerSSBounds.y1 = std::min<int>(outerSSBounds.y1, (int)by1);
        outerSSBounds.x2 = std::max<int>(outerSSBounds.x2, (int)bx1);
        outerSSBounds.x2 = std::max<int>(outerSSBounds.x2, (int)bx2);
        outerSSBounds.y2 = std::max<int>(outerSSBounds.y2, (int)by2);

        Edge e        = {bx1, by1, bx2, by2};
        bool inserted = false;

        // Add edge to list with insertion sort
        for (std::list<Edge>::iterator it = edgeList.begin();
             it != edgeList.end(); it++) {
          if (by1 < it->y1) {
            edgeList.insert(it, e);
            inserted = true;
            break;
          }
        }

        if (!inserted)
          edgeList.insert(edgeList.end(), e);
      }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    else {
      if (y1 == y2)
        continue;

      float temp;

      // Swap points to make sure the first point has a lower y value
      if (y1 > y2) {
        temp = y1;
        y1   = y2;
        y2   = temp;
        temp = x1;
        x1   = x2;
        x2   = temp;
      }

      // Calculate outer boundary region where super sampling should be applied
      //(a rectangle that circumscribes the polygon)
      outerSSBounds.x1 = std::min<int>(outerSSBounds.x1, (int)x1);
      outerSSBounds.x1 = std::min<int>(outerSSBounds.x1, (int)x2);
      outerSSBounds.y1 = std::min<int>(outerSSBounds.y1, (int)y1);
      outerSSBounds.x2 = std::max<int>(outerSSBounds.x2, (int)x1);
      outerSSBounds.x2 = std::max<int>(outerSSBounds.x2, (int)x2);
      outerSSBounds.y2 = std::max<int>(outerSSBounds.y2, (int)y2);

      Edge e        = {x1, y1, x2, y2};
      bool inserted = false;

      // Add edge to list with insertion sort
      for (std::list<Edge>::iterator it = edgeList.begin();
           it != edgeList.end(); it++) {
        if (y1 < it->y1) {
          edgeList.insert(it, e);
          inserted = true;
          break;
        }
      }

      if (!inserted)
        edgeList.insert(edgeList.end(), e);
    }
  }

  // Ensure outer bounding box is within the bounds of the image
  outerSSBounds.x1 = std::max<int>(outerSSBounds.x1, 0);
  outerSSBounds.x2 =
      std::min<int>(outerSSBounds.x2, kernel.getSize().width - 1);
  outerSSBounds.y1 = std::max<int>(outerSSBounds.y1, 0);
  outerSSBounds.y2 =
      std::min<int>(outerSSBounds.y2, kernel.getSize().height - 1);

  PolygonFiller pf;
  pf.process(edgeList, kernel, outerSSBounds, innerSSBounds);

  // Normalize kernel
  Ipp64f kernelSum;
  ippiSum_32f_C1R(kernel.getImagePtr(), kernel.getRowSizeBytes(),
                  kernel.getSize(), &kernelSum, ippAlgHintFast);
  ippiMulC_32f_C1IR((Ipp32f)(1.0 / kernelSum), kernel.getImagePtr(),
                    kernel.getRowSizeBytes(), kernel.getSize());
}

void ThisClass::blendLayerToOutput(Bounds region, ImageContainerA* layer,
                                   ImageContainerA* output,
                                   ImageContainerA* matte, float kernelSize,
                                   int layerNum)
{
  // Process color channels
  for (int channelNum = 0; channelNum < 4; channelNum++) {
    for (int y = region.y1; y <= region.y2; y++)
      for (int x = region.x1; x <= region.x2; x++) {
        float alpha = layer[4].pixel_at(x, y);
        output[channelNum].pixel_at(x, y) =
            output[channelNum].pixel_at(x, y) * (1.0f - alpha) +
            layer[channelNum].pixel_at(x, y) * alpha;
      }
  }

  // Add to matte
  if (matte) {
    for (int y = region.y1; y <= region.y2; y++)
      for (int x = region.x1; x <= region.x2; x++) {
        float alpha           = layer[4].pixel_at(x, y);
        matte->pixel_at(x, y) = matte->pixel_at(x, y) * (1.0f - alpha) +
                                layer[5].pixel_at(x, y) * alpha;
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
