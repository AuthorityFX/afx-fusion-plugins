// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright (C) 2012
//
// Authority FX, Inc.
// www.authorityfx.com

#include "stdafx.h"
#include "afx_defocus.h"

#define pluginGuid ("com.authorityfx:defocusPlugin")
#define pluginName ("AFX Defocus")
#define PLUGIN_LICENSE_NAME ("defocus")

bool ThisClass::pluginIsRegistered;
std::stringstream ThisClass::licenseStatus;

FuRegisterClass(COMPANY_ID ".afx_defocus", CT_Tool)
  //REG_TileID, AFX_Convolve_Tile,
  REGS_Name,  pluginName,
  REGS_Category,  "Authority FX",
  REGS_OpIconString,  "ADef",
  REGS_OpDescription, "Applies a uniform lens blur",
  REGS_VersionString, "Version 1.0",
  REGS_Company, "Authority FX, Inc.",
  REGS_URL, "http://www.authorityfx.com/",
  REG_NoObjMatCtrls,  TRUE,
  REG_NoMotionBlurCtrls,  TRUE,
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
  InShape = AddInput(
      "Shape", "Shape", LINKID_DataType, CLSID_DataType_Number,
      INPID_InputControl, COMBOCONTROL_ID, CCS_AddString, "Triangle (3)",
      CCS_AddString, "Square (4)", CCS_AddString, "Pentagon (5)", CCS_AddString,
      "Hexagon (6)", CCS_AddString, "Heptagon (7)", CCS_AddString,
      "Octagon (8)", CCS_AddString, "Nonagon (9)", CCS_AddString,
      "Decagon (10)", CCS_AddString, "Circle", INP_Default, 3.0, TAG_DONE);

  InSize = AddInput("Size", "Size", LINKID_DataType, CLSID_DataType_Number,
                    INPID_InputControl, SLIDERCONTROL_ID, INP_MinAllowed, 0.0,
                    INP_MaxAllowed, 2.0, INP_MinScale, 0.0, INP_MaxScale, 1.0,
                    INP_Default, 0.1, TAG_DONE);

  InRotation = AddInput("Aperture Blade Rotation", "BladeRotation",
                        LINKID_DataType, CLSID_DataType_Number,
                        INPID_InputControl, SCREWCONTROL_ID, INP_Default, 0.0,
                        INP_MinScale, 0.0, INP_MaxScale, 90.0, TAG_DONE);

  InCurvature =
      AddInput("Aperture Blade Curvature", "BladeCurvature", LINKID_DataType,
               CLSID_DataType_Number, INPID_InputControl, SLIDERCONTROL_ID,
               INP_MinAllowed, -1.0, INP_MaxAllowed, 1.0, INP_MinScale, -1.0,
               INP_MaxScale, 1.0, INP_Default, 0.0, TAG_DONE);

  // Add image
  InImage = AddInput("Input", "Input", LINKID_DataType, CLSID_DataType_Image,
                     LINK_Main, 1, TAG_DONE);

  // Output image
  OutImage = AddOutput("Output", "Output", LINKID_DataType,
                       CLSID_DataType_Image, LINK_Main, 1, TAG_DONE);
}

void ThisClass::Process(Request* req)
{
  // Get input images
  imgInput  = (Image*)InImage->GetValue(req);
  imgOutput = NewImage(IMG_Like, imgInput, TAG_DONE);

  width  = imgInput->Width;
  height = imgInput->Height;

  shape     = (int)(*InShape->GetValue(req) + 0.5);
  curvature = *InCurvature->GetValue(req);
  radius    = *InSize->GetValue(req);
  rotation  = *InRotation->GetValue(req);

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

  double circumradius = std::max<double>(
      0.75, std::min<int>(width, height) * 0.1f * (float)radius);

  ImageContainerA kernel;
  int kernelSize = ((int)(circumradius * 2) + 2) | 1;
  kernel.create_image(kernelSize, kernelSize);
  memset(kernel.getImagePtr(), 0,
         kernel.getRowSizeBytes() * kernel.getSize().height);

  std::list<Edge> edgeList;

  if (Status != STATUS_OK)
    return false;

  /******************************************************************************************
   *    Create Kernel
   *****************************************************************************************/

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
      kernel.getSize().width / 2 + (int)(inradius / sqrtf(2.0f)) - 3;
  innerSSBounds.y1 =
      kernel.getSize().height / 2 - (int)(inradius / sqrtf(2.0f)) + 2;
  innerSSBounds.y2 =
      kernel.getSize().height / 2 + (int)(inradius / sqrtf(2.0f)) - 3;

  // Generate list of edges for the kernel polygon
  for (int i = 0; i < apertureNumBlades; i++) {
    if (Status != STATUS_OK)
      return false;

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
          kernel.getSize().width / 2 + (int)(newInradius / sqrtf(2.0f)) - 3;
      innerSSBounds.y1 =
          kernel.getSize().height / 2 - (int)(newInradius / sqrtf(2.0f)) + 2;
      innerSSBounds.y2 =
          kernel.getSize().height / 2 + (int)(newInradius / sqrtf(2.0f)) - 3;

      if (kernel.getSize().width == 1 && kernel.getSize().height == 1) {
        innerSSBounds.x1 = innerSSBounds.y1 = 0;
        innerSSBounds.x2 = innerSSBounds.y2 = 0;
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

  if (Status != STATUS_OK)
    return false;

  // Ensure outer bounding box is within the bounds of the image
  outerSSBounds.x1 = std::max<int>(outerSSBounds.x1 - 2, 0);
  outerSSBounds.x2 =
      std::min<int>(outerSSBounds.x2 + 2, kernel.getSize().width - 1);
  outerSSBounds.y1 = std::max<int>(outerSSBounds.y1 - 2, 0);
  outerSSBounds.y2 =
      std::min<int>(outerSSBounds.y2 + 2, kernel.getSize().height - 1);

  {
    PolygonFiller polyfiller;
    polyfiller.process(edgeList, kernel, outerSSBounds, innerSSBounds);
  }

  if (Status != STATUS_OK)
    return false;

  // Normalize kernel
  Ipp64f kernelSum;
  ippiSum_32f_C1R(kernel.getImagePtr(), kernel.getRowSizeBytes(),
                  kernel.getSize(), &kernelSum, ippAlgHintFast);
  ippiMulC_32f_C1IR((Ipp32f)(1.0 / kernelSum), kernel.getImagePtr(),
                    kernel.getRowSizeBytes(), kernel.getSize());

  if (Status != STATUS_OK)
    return false;

  int imagePadding = kernelSize / 2;

  if (radius == 0.0 && kernelSize == 3) {
    kernel.pixel_at(0, 0) = kernel.pixel_at(1, 0) = kernel.pixel_at(2, 0) =
        0.0f;
    kernel.pixel_at(0, 1) = kernel.pixel_at(1, 1) = kernel.pixel_at(2, 1) =
        0.0f;
    kernel.pixel_at(0, 2) = kernel.pixel_at(1, 2) = kernel.pixel_at(2, 2) =
        0.0f;

    kernel.pixel_at(1, 1) = 1.0f;
  }

  ImageContainerA source[4];
  for (int i = 0; i < 4; i++)
    source[i].create_image(width + 2 * imagePadding, height + 2 * imagePadding);

  Bounds bounds;
  bounds.x1 = 0;
  bounds.y1 = 0;
  bounds.x2 = width + 2 * imagePadding - 1;
  bounds.y2 = height + 2 * imagePadding - 1;

  {
    Multithreader threader(bounds);
    threader.processInChunks(
        boost::bind(&ThisClass::grabInput, this, _1, source));
  }

  if (Status != STATUS_OK)
    return false;

  ImageContainerA outputRGB[4];
  for (int i = 0; i < 4; i++) outputRGB[i].create_image(width, height);

  for (int i = 0; i < 4; i++) {
    ippiConvValid_32f_C1R(source[i].getImagePtr(), source[i].getRowSizeBytes(),
                          source[i].getSize(), kernel.getImagePtr(),
                          kernel.getRowSizeBytes(), kernel.getSize(),
                          outputRGB[i].getImagePtr(),
                          outputRGB[i].getRowSizeBytes());
  }

  if (Status != STATUS_OK)
    return false;

  {
    Multithreader threader(
        Bounds(0, 0, imgOutput->Width - 1, imgOutput->Height - 1));
    threader.processInChunks(
        boost::bind(&ThisClass::setOutput, this, _1, outputRGB));
  }

  if (Status != STATUS_OK)
    return false;

  for (int i = 0; i < 4; i++) source[i].dispose();

  return true;
}

void ThisClass::grabInput(Bounds region, ImageContainerA* dest)
{
  PixPtr ip(imgInput);
  FltPixel p;

  int widthPadding  = (dest->getSize().width - width) / 2;
  int heightPadding = (dest->getSize().height - height) / 2;

  for (int y = region.y1; y <= region.y2; y++) {
    int clampedY =
        std::max<int>(0, std::min<int>(height - 1, y - heightPadding));
    ip.GotoXY(region.x1, clampedY);

    for (int x = region.x1; x <= region.x2; x++) {
      int clampedX =
          std::max<int>(0, std::min<int>(width - 1, x - widthPadding));

      if (x >= widthPadding && (x - widthPadding) < width - 1)
        p >>= ip;
      else
        p = ip;

      // Store pixel values in memory
      dest[0].pixel_at(x, y) = p.R;
      dest[1].pixel_at(x, y) = p.G;
      dest[2].pixel_at(x, y) = p.B;
      dest[3].pixel_at(x, y) = p.A;
    }
  }
}

void ThisClass::setOutput(Bounds region, ImageContainerA* src)
{
  PixPtr ip(imgInput);
  PixPtr op(imgOutput);
  FltPixel p;

  for (int y = region.y1; y <= region.y2; y++) {
    ip.GotoXY(region.x1, y);
    op.GotoXY(region.x1, y);

    for (int x = region.x1; x <= region.x2; x++) {
      p >>= ip;

      p.R = src[0].pixel_at(x, y);
      p.G = src[1].pixel_at(x, y);
      p.B = src[2].pixel_at(x, y);
      p.A = src[3].pixel_at(x, y);

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
