#ifndef __POLYFILL_MULTITHREADER_H__
#define __POLYFILL_MULTITHREADER_H__

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <list>

class PolygonFiller {
private:
  boost::mutex m_mutex;
  int nextLine;

  inline void lineByLineThreadTask(
      boost::function<void(Bounds, std::list<Edge>&, ImageContainerA&)> mpc,
      std::list<Edge> edgeList, ImageContainerA& image);
  inline void fillPolygonThreadTask(Bounds region, std::list<Edge>& edgeList,
                                    ImageContainerA& image,
                                    Bounds outerSSBounds, Bounds innerSSBounds);
  inline void processLineByLine(
      boost::function<void(Bounds, std::list<Edge>&, ImageContainerA&)> mpc,
      std::list<Edge> edgeList, ImageContainerA& image);

public:
  void process(const std::list<Edge>& edgeList, ImageContainerA& image,
               const Bounds& outerSSBounds, const Bounds& innerSSBounds)
  {
    processLineByLine(boost::bind(&PolygonFiller::fillPolygonThreadTask, this,
                                  _1, _2, _3, outerSSBounds, innerSSBounds),
                      edgeList, image);
  }
};

void PolygonFiller::processLineByLine(
    boost::function<void(Bounds, std::list<Edge>&, ImageContainerA&)> mpc,
    std::list<Edge> edgeList, ImageContainerA& image)
{
  int nThreads = boost::thread::hardware_concurrency();
  if (nThreads == 0)
    nThreads = 1;

  nThreads = 1;

  std::list<boost::thread*> threadPool;

  nextLine = 0;

  for (int i = 0; i < nThreads; i++)
    threadPool.push_back(
        new boost::thread(boost::bind(&PolygonFiller::lineByLineThreadTask,
                                      this, mpc, edgeList, boost::ref(image))));

  for (std::list<boost::thread*>::iterator it = threadPool.begin();
       it != threadPool.end(); it++) {
    (*it)->join();
    delete *it;
    (*it) = 0;
  }
}

void PolygonFiller::lineByLineThreadTask(
    boost::function<void(Bounds, std::list<Edge>&, ImageContainerA&)> mpc,
    std::list<Edge> edgeList, ImageContainerA& image)
{
  int thisLine;
  Bounds region;

  while (true) {
    {
      boost::mutex::scoped_lock lock(m_mutex);
      if (nextLine >= image.getSize().height)
        break;
      thisLine = nextLine++;
    }

    region.x1 = 0;
    region.x2 = image.getSize().width - 1;
    region.y1 = thisLine;
    region.y2 = thisLine;
    mpc(region, boost::ref(edgeList), boost::ref(image));
  }
}

void PolygonFiller::fillPolygonThreadTask(Bounds region,
                                          std::list<Edge>& edgeList,
                                          ImageContainerA& image,
                                          Bounds outerSSBounds,
                                          Bounds innerSSBounds)
{
  float subpixelIntensity = 1.0 / (SUPER_SAMPLE_AMOUNT * SUPER_SAMPLE_AMOUNT);
  double y                = region.y1;

  if (y >= outerSSBounds.y1 && y <= outerSSBounds.y2 + 1) {
    for (int ss = 0; ss < SUPER_SAMPLE_AMOUNT; ss++) {
      double ssy =
          y +
          (double)(ss - SUPER_SAMPLE_AMOUNT / 2) / (double)SUPER_SAMPLE_AMOUNT +
          ((SUPER_SAMPLE_AMOUNT % 2) ? 0 : 0.5 / SUPER_SAMPLE_AMOUNT);

      std::list<double> points;
      std::list<Edge>::iterator eit = edgeList.begin();

      // Find points of intersection between polygon edges and
      // this scanline
      while (eit != edgeList.end()) {
        // The line segment we reached is too far down the y-axis
        // since the list is sorted, we can skip the remaining edges
        if (eit->y1 - 0.5 > ssy)
          break;

        // We've come to the end of the line segment. remove it from the list
        // as it's complete
        if (eit->y2 - 0.5 < ssy) {
          edgeList.erase(eit++);
          continue;
        }

        // Find the point of intersection of the line segment
        // with this scanline
        double m1 = (eit->x2 - eit->x1) / (eit->y2 - eit->y1);
        double b1 = (eit->x1 - 0.5) - m1 * (eit->y1 - 0.5);
        double p  = m1 * ssy + b1;

        bool inserted = false;

        // Add point to list with insertion sort
        for (std::list<double>::iterator pit = points.begin();
             pit != points.end(); pit++) {
          if (p < *pit) {
            points.insert(pit, p);
            inserted = true;
            break;
          }
        }

        if (!inserted)
          points.insert(points.end(), p);

        eit++;
      }

      std::list<double>::iterator pit = points.begin();

      // Fill in each scanline
      while (pit != points.end()) {
        double p1 = *(pit++);
        double p2;

        if (pit == points.end())
          p2 = image.getSize().width;
        else
          p2 = *(pit++);

        for (int x = outerSSBounds.x1; x <= outerSSBounds.x2; x++) {
          // Skip everything within the inner boundary. We don't need to do
          // supersampling in this region
          if (y > innerSSBounds.y1 && y < innerSSBounds.y2 &&
              x == innerSSBounds.x1)
            x = innerSSBounds.x2;

          for (int ssx = 0; ssx < SUPER_SAMPLE_AMOUNT; ssx++) {
            double xs =
                (double)x +
                (double)(ssx - SUPER_SAMPLE_AMOUNT / 2) /
                    (double)SUPER_SAMPLE_AMOUNT +
                ((SUPER_SAMPLE_AMOUNT % 2) ? 0 : 0.5 / SUPER_SAMPLE_AMOUNT);

            if (xs > p1 && xs < p2)
              image.pixel_at(x, y) += subpixelIntensity;
          }
        }
      }

      if (points.begin() == points.end()) {
        for (int x = 0; x < image.getSize().width; x++)
          image.pixel_at(x, y) = 0.0f;
      }
    }

    // Fill in the inner bounding box with white.
    if (y > innerSSBounds.y1 && y < innerSSBounds.y2)
      for (int x = innerSSBounds.x1; x < innerSSBounds.x2; x++)
        image.pixel_at(x, y) = 1.0;
  }
}

#endif