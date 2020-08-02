// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright (C) 2012
//
// Authority FX, Inc.
// www.authorityfx.com

#ifndef __IMAGE_CONTAINER_H__
#define __IMAGE_CONTAINER_H__

#include <ipp.h>

struct PixelRGB {
  Ipp32f& r;
  Ipp32f& g;
  Ipp32f& b;
};

class ImageContainerRGB {
private:
  Ipp32f* imageData;
  int rowSizeBytes;
  IppiSize imageSize;

public:
  ImageContainerRGB() : imageData(0) {}

  ImageContainerRGB(unsigned int width, unsigned int height) : imageData(0)
  {
    create_image(width, height);
  }

  virtual ~ImageContainerRGB() { ippiFree(imageData); }

  PixelRGB pixel_at(unsigned int x, unsigned int y)
  {
    Ipp32f* redPtr   = (Ipp32f*)((char*)(imageData) + y * rowSizeBytes +
                               (x * 3) * sizeof(Ipp32f));
    Ipp32f* greenPtr = (Ipp32f*)((char*)(imageData) + y * rowSizeBytes +
                                 (x * 3 + 1) * sizeof(Ipp32f));
    Ipp32f* bluePtr  = (Ipp32f*)((char*)(imageData) + y * rowSizeBytes +
                                (x * 3 + 2) * sizeof(Ipp32f));

    PixelRGB p = {*redPtr, *greenPtr, *bluePtr};

    return p;
  }

  Ipp32f* getImagePtr() { return imageData; }
  int getRowSizeBytes() { return rowSizeBytes; }
  IppiSize getSize() { return imageSize; }

  Ipp32f* create_image(unsigned int width, unsigned int height)
  {
    if (imageData)
      return imageData;

    imageSize.width  = width;
    imageSize.height = height;
    imageData        = ippiMalloc_32f_C3(width, height, &rowSizeBytes);
    return imageData;
  }

  void dispose()
  {
    if (imageData) {
      ippiFree(imageData);
      imageData = 0;
    }
  }
};

class ImageContainerA {
private:
  Ipp32f* imageData;
  int rowSizeBytes;
  IppiSize imageSize;

public:
  ImageContainerA() : imageData(0) {}

  ImageContainerA(unsigned int width, unsigned int height) : imageData(0)
  {
    create_image(width, height);
  }

  virtual ~ImageContainerA()
  {
    if (imageData)
      ippiFree(imageData);
  }

  Ipp32f& pixel_at(unsigned int x, unsigned int y)
  {
    Ipp32f* alphaPtr =
        (Ipp32f*)((char*)(imageData) + y * rowSizeBytes + x * sizeof(Ipp32f));

    return *alphaPtr;
  }

  Ipp32f* ptr_at(unsigned int x, unsigned int y)
  {
    Ipp32f* alphaPtr =
        (Ipp32f*)((char*)(imageData) + y * rowSizeBytes + x * sizeof(Ipp32f));

    return alphaPtr;
  }

  Ipp32f* getImagePtr() { return imageData; }
  int getRowSizeBytes() { return rowSizeBytes; }
  IppiSize getSize() { return imageSize; }

  Ipp32f* create_image(unsigned int width, unsigned int height)
  {
    if (imageData)
      return imageData;

    imageSize.width  = width;
    imageSize.height = height;
    imageData        = ippiMalloc_32f_C1(width, height, &rowSizeBytes);
    return imageData;
  }

  void dispose()
  {
    if (imageData) {
      ippiFree(imageData);
      imageData = 0;
    }
  }
};

#endif
