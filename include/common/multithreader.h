// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright (C) 2012
//
// Authority FX, Inc.
// www.authorityfx.com

#ifndef __MULTITHREADER_H__
#define __MULTITHREADER_H__

#include <boost/thread.hpp>
#include <boost/bind.hpp>

struct Bounds {
public:
  Bounds() : x1(0), y1(0), x2(0), y2(0) {}

  Bounds(long int x1, long int y1, long int x2, long int y2)
      : x1(x1), y1(y1), x2(x2), y2(y2)
  {
  }

  long int x1;
  long int y1;
  long int x2;
  long int y2;
};

class Multithreader {
private:
  Bounds region;
  boost::mutex m_mutex;
  int nextLine;

  inline virtual void lineByLineThreadTask(boost::function<void(Bounds)>);

public:
  Multithreader(const Bounds& region) : region(region) {}

  Multithreader(boost::function<void(Bounds)> mpc, const Bounds& region,
                bool lineByLine = false)
      : region(region)
  {
    if (lineByLine)
      processLineByLine(mpc);
    else
      processInChunks(mpc);
  }

  Bounds Region() const { return region; }
  void SetRegion(const Bounds& b) { region = b; }

  inline virtual void processInChunks(boost::function<void(Bounds)> mpc);
  inline virtual void processLineByLine(boost::function<void(Bounds)> mpc);
  inline virtual void processInChunksX(boost::function<void(Bounds)> mpc);
};

void Multithreader::processInChunks(boost::function<void(Bounds)> mpc)
{
  int nThreads = boost::thread::hardware_concurrency();
  if (nThreads == 0)
    nThreads = 1;

  std::list<boost::thread*> threadPool;

  Bounds threadRegion;
  threadRegion.x1 = region.x1;
  threadRegion.x2 = region.x2;

  for (int i = 0; i < nThreads; i++) {
    threadRegion.y1 = (region.y2 - region.y1) * i / nThreads + region.y1;
    threadRegion.y2 =
        (region.y2 - region.y1) * (i + 1) / nThreads - 1 + region.y1;
    if (i == nThreads - 1)
      threadRegion.y2 += 1;
    threadPool.push_back(new boost::thread(boost::bind(mpc, threadRegion)));
  }

  // Join and clean up threads
  for (std::list<boost::thread*>::iterator it = threadPool.begin();
       it != threadPool.end(); it++) {
    (*it)->join();
    delete *it;
    (*it) = 0;
  }
}

void Multithreader::processLineByLine(boost::function<void(Bounds)> mpc)
{
  int nThreads = boost::thread::hardware_concurrency();
  if (nThreads == 0)
    nThreads = 1;

  std::list<boost::thread*> threadPool;

  nextLine = region.y1;

  for (int i = 0; i < nThreads; i++)
    threadPool.push_back(new boost::thread(
        boost::bind(&Multithreader::lineByLineThreadTask, this, mpc)));

  // Join and clean up threads
  for (std::list<boost::thread*>::iterator it = threadPool.begin();
       it != threadPool.end(); it++) {
    (*it)->join();
    delete *it;
    (*it) = 0;
  }
}

void Multithreader::processInChunksX(boost::function<void(Bounds)> mpc)
{
  int nThreads = boost::thread::hardware_concurrency();
  if (nThreads == 0)
    nThreads = 1;

  std::list<boost::thread*> threadPool;

  Bounds threadRegion;
  threadRegion.y1 = region.y1;
  threadRegion.y2 = region.y2;

  for (int i = 0; i < nThreads; i++) {
    threadRegion.x1 = (region.x2 - region.x1) * i / nThreads + region.x1;
    threadRegion.x2 =
        (region.x2 - region.x1) * (i + 1) / nThreads - 1 + region.x1;
    if (i == nThreads - 1)
      threadRegion.x2 += 1;
    threadPool.push_back(new boost::thread(boost::bind(mpc, threadRegion)));
  }

  // Join and clean up threads
  for (std::list<boost::thread*>::iterator it = threadPool.begin();
       it != threadPool.end(); it++) {
    (*it)->join();
    delete *it;
    (*it) = 0;
  }
}

void Multithreader::lineByLineThreadTask(boost::function<void(Bounds)> mpc)
{
  int thisLine;
  Bounds threadRegion;

  while (true) {
    {
      boost::mutex::scoped_lock lock(m_mutex);
      if (nextLine > region.y2)
        break;
      thisLine = nextLine++;
    }

    threadRegion.x1 = region.x1;
    threadRegion.x2 = region.x2;
    threadRegion.y1 = thisLine;
    threadRegion.y2 = thisLine;
    mpc(threadRegion);
  }
}

#endif
