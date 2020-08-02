// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright (C) 2012
//
// Authority FX, Inc.
// www.authorityfx.com

#ifndef __IMAGE_PROC_H__
#define __IMAGE_PROC_H__

#include "image_container.h"
#include "multithreader.h"
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <vector>
#include <cmath>
#include <algorithm>

class ImageCropper {
private:
  inline void workerThread(Bounds region, ImageContainerA* input,
                           ImageContainerA* output, int numChannels,
                           Bounds bounds);

public:
  inline void process(ImageContainerA* input, ImageContainerA* output,
                      int numChannels, Bounds bounds);
};

void ImageCropper::workerThread(Bounds region, ImageContainerA* input,
                                ImageContainerA* output, int numChannels,
                                Bounds bounds)
{
  for (int y = region.y1; y <= region.y2; y++) {
    for (int x = region.x1; x <= region.x2; x++) {
      for (int i = 0; i < numChannels; i++)
        output[i].pixel_at(x - bounds.x1, y - bounds.y1) =
            input[i].pixel_at(x, y);
    }
  }
}

void ImageCropper::process(ImageContainerA* input, ImageContainerA* output,
                           int numChannels, Bounds bounds)
{
  int nThreads = boost::thread::hardware_concurrency();

  std::list<boost::thread*> threads;

  Bounds region;
  region.x1 = bounds.x1;
  region.x2 = bounds.x2;

  // Create threads
  for (int i = 0; i < nThreads; i++) {
    region.y1 = bounds.y1 + ((bounds.y2 - bounds.y1) * i) / nThreads;
    region.y2 = bounds.y1 + ((bounds.y2 - bounds.y1) * (i + 1)) / nThreads;
    if (i - i < nThreads)
      region.y2--;
    threads.push_back(
        new boost::thread(boost::bind(&ImageCropper::workerThread, this, region,
                                      input, output, numChannels, bounds)));
  }

  for (std::list<boost::thread*>::iterator it = threads.begin();
       it != threads.end(); it++) {
    (*it)->join();
    delete (*it);
    (*it) = 0;
  }
}

class GaussianBlur : private Multithreader {
private:
  inline void horizontalGaussianBlur(Bounds region, ImageContainerA* in,
                                     ImageContainerA* out, float* kernel,
                                     int kernelSize);
  inline void verticalGaussianBlur(Bounds region, ImageContainerA* in,
                                   ImageContainerA* out, float* kernel,
                                   int kernelSize);

public:
  GaussianBlur(Bounds region) : Multithreader(region) {}

  void process(ImageContainerA* in, float blurRadius)
  {
    process(in, in, blurRadius);
  }

  inline void process(ImageContainerA* in, ImageContainerA* out,
                      float blurRadius);
};

void GaussianBlur::process(ImageContainerA* in, ImageContainerA* out,
                           float blurRadius)
{
  ImageContainerA intermediate(in->getSize().width, in->getSize().height);

  float sigma      = blurRadius;
  int kernelSize   = (int)ceil(7 * sigma) | 1;
  int kernelCenter = kernelSize / 2;

  float normalizingFactor = 1.0f / (sqrt(6.28318530717958647f) * sigma);
  float sigmaInvSqr       = 1.0f / (sigma * sigma);

  float sum = 0.0f;

  std::vector<float> kernel(kernelSize);

  for (int i = 0; i < kernelSize; i++) {
    kernel[i] = normalizingFactor * exp(-0.5f * (i - kernelCenter) *
                                        (i - kernelCenter) * sigmaInvSqr);
    sum += kernel[i];
  }

  float invSum = 1.0f / sum;

  for (int i = 0; i < kernelSize; i++) kernel[i] *= invSum;

  processInChunks(boost::bind(&GaussianBlur::horizontalGaussianBlur, this, _1,
                              in, &intermediate, &kernel[0], kernelSize));
  processInChunks(boost::bind(&GaussianBlur::verticalGaussianBlur, this, _1,
                              &intermediate, out, &kernel[0], kernelSize));
}

void GaussianBlur::horizontalGaussianBlur(Bounds region, ImageContainerA* in,
                                          ImageContainerA* out, float* kernel,
                                          int kernelSize)
{
  float* po;

  int kernelCenter = kernelSize / 2;
  int xx;

  int width = in->getSize().width;

  for (int y = region.y1; y <= region.y2; y++) {
    po = &out->pixel_at(region.x1, y);

    for (int x = region.x1; x <= region.x2; x++) {
      *po = 0.0f;

      for (int i = 0; i < kernelSize; i++) {
        xx = x - kernelCenter + i;
        if (xx < 0)
          xx = 0;
        if (xx > width - 1)
          xx = width - 1;

        *po += in->pixel_at(xx, y) * kernel[i];
      }

      po++;
    }
  }
}

void GaussianBlur::verticalGaussianBlur(Bounds region, ImageContainerA* in,
                                        ImageContainerA* out, float* kernel,
                                        int kernelSize)
{
  float* po;

  int kernelCenter = kernelSize / 2;
  int yy;

  int height = in->getSize().height;

  for (int y = region.y1; y <= region.y2; y++) {
    po = &out->pixel_at(region.x1, y);

    for (int x = region.x1; x <= region.x2; x++) {
      *po = 0.0f;

      for (int i = 0; i < kernelSize; i++) {
        yy = y - kernelCenter + i;
        if (yy < 0)
          yy = 0;
        if (yy > height - 1)
          yy = height - 1;

        *po += in->pixel_at(x, yy) * kernel[i];
      }

      po++;
    }
  }
}

#endif
