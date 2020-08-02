// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright (C) 2012
//
// Authority FX, Inc.
// www.authorityfx.com


#ifndef __IMAGE_CALC_H__
#define __IMAGE_CALC_H__

#include "image_container.h"
#include "multithreader.h"
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <list>
#include <ipp.h>

// Calculates the coordinates of the centroid of an image

class CentroidCalculator {
private:
  double* cumulativeIntensity;
  double* weightedMeanX;
  double* weightedMeanY;
  boost::mutex m_mutex[3];

  inline void workerThread(ImageContainerA*, int y1, int y2, int threadNum,
                           int step);

public:
  CentroidCalculator() {}

  CentroidCalculator(ImageContainerA* img, double& centroidX, double& centroidY,
                     int step = 4)
  {
    process(img, centroidX, centroidY, step);
  }

  // step indicates how sparse the sampling of the image should be. default is
  // every fourth pixel
  inline void process(ImageContainerA* img, double& centroidX,
                      double& centroidY, int step = 4);
};

void CentroidCalculator::process(ImageContainerA* img, double& centroidX,
                                 double& centroidY, int step)
{
  // img is assumed to contain three images that are all equally sized

  int nThreads = boost::thread::hardware_concurrency();
  if (nThreads == 0)
    nThreads = 1;

  std::list<boost::thread*> threadPool;

  int y1, y2;

  weightedMeanX       = new double[nThreads];
  weightedMeanY       = new double[nThreads];
  cumulativeIntensity = new double[nThreads];

  double totalWeightedMeanX       = 0.0;
  double totalWeightedMeanY       = 0.0;
  double totalCumulativeIntensity = 0.0;

  for (int i = 0; i < nThreads; i++) {
    weightedMeanX[i]       = 0.0;
    weightedMeanY[i]       = 0.0;
    cumulativeIntensity[i] = 0.0;

    y1 = img[0].getSize().height * i / nThreads;
    y2 = img[0].getSize().height * (i + 1) / nThreads - 1;
    if (i == nThreads - 1)
      y2++;
    threadPool.push_back(new boost::thread(boost::bind(
        &CentroidCalculator::workerThread, this, img, y1, y2, i, step)));
  }

  for (std::list<boost::thread*>::iterator it = threadPool.begin();
       it != threadPool.end(); it++) {
    (*it)->join();
    delete *it;
    (*it) = 0;
  }

  for (int i = 0; i < nThreads; i++) {
    totalWeightedMeanX += weightedMeanX[i];
    totalWeightedMeanY += weightedMeanY[i];
    totalCumulativeIntensity += cumulativeIntensity[i];
  }

  delete[] weightedMeanX;
  delete[] weightedMeanY;
  delete[] cumulativeIntensity;

  centroidX = totalWeightedMeanX / totalCumulativeIntensity;
  centroidY = totalWeightedMeanY / totalCumulativeIntensity;
}

void CentroidCalculator::workerThread(ImageContainerA* img, int y1, int y2,
                                      int threadNum, int step)
{
  double pixelIntensity;

  for (int y = y1; y < y2; y += step) {
    for (int x = 0; x < img->getSize().width; x += step) {
      pixelIntensity =
          img[0].pixel_at(x, y) + img[1].pixel_at(x, y) + img[2].pixel_at(x, y);

      cumulativeIntensity[threadNum] += pixelIntensity;
      weightedMeanX[threadNum] += x * pixelIntensity;
      weightedMeanY[threadNum] += y * pixelIntensity;
    }
  }
}

class BoundaryCalculator {
private:
  int bound[4];

  inline void workerThread(ImageContainerA* input, int side);

public:
  inline Bounds process(ImageContainerA* input);
};

void BoundaryCalculator::workerThread(ImageContainerA* input, int side)
{
  // Assumes 3 inputs of the same size

  switch (side) {
    case 0:
      for (int y = 0; y < input[0].getSize().height; y++) {
        for (int x = 0; x < input[0].getSize().width; x++)
          if (input[0].pixel_at(x, y) > 0.00001 ||
              input[1].pixel_at(x, y) > 0.00001 ||
              input[2].pixel_at(x, y) > 0.00001) {
            bound[0] = y;
            return;
          }
      }
      bound[0] = 0;
      break;

    case 1:
      for (int x = input[0].getSize().width - 1; x >= 0; x--) {
        for (int y = 0; y < input[0].getSize().height; y++)
          if (input[0].pixel_at(x, y) > 0.00001 ||
              input[1].pixel_at(x, y) > 0.00001 ||
              input[2].pixel_at(x, y) > 0.00001) {
            bound[1] = x;
            return;
          }
      }
      bound[1] = 0;
      break;

    case 2:
      for (int y = input[0].getSize().height - 1; y >= 0; y--) {
        for (int x = 0; x < input[0].getSize().width; x++)
          if (input[0].pixel_at(x, y) > 0.00001 ||
              input[1].pixel_at(x, y) > 0.00001 ||
              input[2].pixel_at(x, y) > 0.00001) {
            bound[2] = y;
            return;
          }
      }
      bound[2] = 0;
      break;

    case 3:
      for (int x = 0; x < input[0].getSize().width; x++) {
        for (int y = 0; y < input[0].getSize().height; y++)
          if (input[0].pixel_at(x, y) > 0.00001 ||
              input[1].pixel_at(x, y) > 0.00001 ||
              input[2].pixel_at(x, y) > 0.00001) {
            bound[3] = x;
            return;
          }
      }
      bound[3] = 0;
      break;
  }
}

Bounds BoundaryCalculator::process(ImageContainerA* input)
{
  int nThreads = 4;  // the worker frunction only needs to be run on four
                     // threads

  std::list<boost::thread*> threads;

  // Create threads
  for (int i = 0; i < nThreads; i++)
    threads.push_back(new boost::thread(
        boost::bind(&BoundaryCalculator::workerThread, this, input, i)));

  for (std::list<boost::thread*>::iterator it = threads.begin();
       it != threads.end(); it++) {
    (*it)->join();
    delete (*it);
    (*it) = 0;
  }

  Bounds boundingBox;
  boundingBox.x1 = bound[3];
  boundingBox.y1 = bound[0];
  boundingBox.x2 = bound[1];
  boundingBox.y2 = bound[2];

  return boundingBox;
}

#endif
