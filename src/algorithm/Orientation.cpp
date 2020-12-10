/**********************************************************************
 *
 * GEOS - Geometry Engine Open Source
 * http://geos.osgeo.org
 *
 * Copyright (C) 2018 Paul Ramsey <pramsey@cleverlephant.ca>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU Lesser General Public Licence as published
 * by the Free Software Foundation.
 * See the COPYING file for more information.
 *
 **********************************************************************
 *
 * Last port: algorithm/Orientation.java @ 2017-09-04
 *
 **********************************************************************/

#include <cassert>
#include <cmath>
#include <vector>

#include <geos/algorithm/Orientation.h>
#include <geos/algorithm/CGAlgorithmsDD.h>
#include <geos/geom/CoordinateSequence.h>
#include <geos/geom/CoordinateArraySequence.h>
#include <geos/geom/Coordinate.h>
#include <geos/geom/Location.h>
#include <geos/util/IllegalArgumentException.h>

namespace geos {
namespace algorithm { // geos.algorithm

/* public static */
int
Orientation::index(const geom::Coordinate& p1, const geom::Coordinate& p2,
                   const geom::Coordinate& q)
{
    return CGAlgorithmsDD::orientationIndex(p1, p2, q);
}


/* public static */
bool
Orientation::isCCW(const geom::CoordinateSequence* ring)
{
    // # of points without closing endpoint
    int inPts = static_cast<int>(ring->size()) - 1;
    // sanity check
    if (inPts < 3)
        throw util::IllegalArgumentException(
            "Ring has fewer than 4 points, so orientation cannot be determined");

    uint32_t nPts = static_cast<uint32_t>(inPts);
    /**
     * Find first highest point after a lower point, if one exists
     * (e.g. a rising segment)
     * If one does not exist, hiIndex will remain 0
     * and the ring must be flat.
     * Note this relies on the convention that
     * rings have the same start and end point.
     */
    geom::Coordinate upHiPt;
    ring->getAt(0, upHiPt);
    double prevY = upHiPt.y;
    geom::Coordinate upLowPt;
    upLowPt.setNull();
    // const geom::Coordinate& upLowPt = nullptr;
    uint32_t iUpHi = 0;
    for (uint32_t i = 1; i <= nPts; i++) {
        double py = ring->getY(i);
        /**
        * If segment is upwards and endpoint is higher, record it
        */
        if (py > prevY && py >= upHiPt.y) {
            ring->getAt(i, upHiPt);
            iUpHi = i;
            ring->getAt(i-1, upLowPt);
        }
        prevY = py;
    }
    /**
     * Check if ring is flat and return default value if so
     */
    if (iUpHi == 0) return false;

    /**
     * Find the next lower point after the high point
     * (e.g. a falling segment).
     * This must exist since ring is not flat.
     */
    uint32_t iDownLow = iUpHi;
    do {
        iDownLow = (iDownLow + 1) % nPts;
    } while (iDownLow != iUpHi && ring->getY(iDownLow) == upHiPt.y );

    const geom::Coordinate& downLowPt = ring->getAt(iDownLow);
    uint32_t iDownHi = iDownLow > 0 ? iDownLow - 1 : nPts - 1;
    const geom::Coordinate& downHiPt = ring->getAt(iDownHi);

    /**
     * Two cases can occur:
     * 1) the hiPt and the downPrevPt are the same.
     *    This is the general position case of a "pointed cap".
     *    The ring orientation is determined by the orientation of the cap
     * 2) The hiPt and the downPrevPt are different.
     *    In this case the top of the cap is flat.
     *    The ring orientation is given by the direction of the flat segment
     */
    if (upHiPt.equals2D(downHiPt)) {
      /**
       * Check for the case where the cap has configuration A-B-A.
       * This can happen if the ring does not contain 3 distinct points
       * (including the case where the input array has fewer than 4 elements), or
       * it contains coincident line segments.
       */
      if (upLowPt.equals2D(upHiPt) || downLowPt.equals2D(upHiPt) || upLowPt.equals2D(downLowPt))
        return false;

      /**
       * It can happen that the top segments are coincident.
       * This is an invalid ring, which cannot be computed correctly.
       * In this case the orientation is 0, and the result is false.
       */
      int orientationIndex = index(upLowPt, upHiPt, downLowPt);
      return orientationIndex == COUNTERCLOCKWISE;
    }
    else {
      /**
       * Flat cap - direction of flat top determines orientation
       */
      double delX = downHiPt.x - upHiPt.x;
      return delX < 0;
    }
}

#if 0
/* public static */
bool
Orientation::isCCW(const geom::CoordinateSequence* ring)
{
    // sanity check
    if(ring->getSize() < 4) {
        throw util::IllegalArgumentException("Ring has fewer than 4 points, so orientation cannot be determined");
    }

    // # of points without closing endpoint
    const std::size_t nPts = ring->getSize() - 1;
    assert(nPts >= 3); // This is here for scan-build

    // find highest point
    const geom::Coordinate* hiPt = &ring->getAt(0);
    std::size_t hiIndex = 0;
    for(std::size_t i = 1; i <= nPts; ++i) {
        const geom::Coordinate* p = &ring->getAt(i);
        if(p->y > hiPt->y) {
            hiPt = p;
            hiIndex = i;
        }
    }

    // find distinct point before highest point
    auto iPrev = hiIndex;
    do {
        if(iPrev == 0) {
            iPrev = nPts;
        }
        iPrev = iPrev - 1;
    }
    while(ring->getAt(iPrev) == *hiPt && iPrev != hiIndex);

    // find distinct point after highest point
    auto iNext = hiIndex;
    do {
        iNext = (iNext + 1) % nPts;
    }
    while(ring->getAt(iNext) == *hiPt && iNext != hiIndex);

    const geom::Coordinate* prev = &ring->getAt(iPrev);
    const geom::Coordinate* next = &ring->getAt(iNext);

    /*
     * This check catches cases where the ring contains an A-B-A
     * configuration of points.
     * This can happen if the ring does not contain 3 distinct points
     * (including the case where the input array has fewer than 4 elements),
     * or it contains coincident line segments.
     */
    if(prev->equals2D(*hiPt) || next->equals2D(*hiPt) ||
            prev->equals2D(*next)) {
        return false;
        // MD - don't bother throwing exception,
        // since this isn't a complete check for ring validity
        //throw  IllegalArgumentException("degenerate ring (does not contain 3 distinct points)");
    }

    int disc = Orientation::index(*prev, *hiPt, *next);

    /*
     *  If disc is exactly 0, lines are collinear.
     * There are two possible cases:
     *  (1) the lines lie along the x axis in opposite directions
     *  (2) the lines lie on top of one another
     *
     *  (1) is handled by checking if next is left of prev ==> CCW
     *  (2) should never happen, so we're going to ignore it!
     *  (Might want to assert this)
     */
    bool isCCW = false;

    if(disc == 0) {
        // poly is CCW if prev x is right of next x
        isCCW = (prev->x > next->x);
    }
    else {
        // if area is positive, points are ordered CCW
        isCCW = (disc > 0);
    }

    return isCCW;
}
#endif


} // namespace geos.algorithm
} //namespace geos

