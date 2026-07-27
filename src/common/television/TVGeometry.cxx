//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include <cmath>

#include "TVGeometry.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TVGeometry::setCurvature(uInt32 amount, uInt32 vertical)
{
  const uInt32 newAmount = BSPF::clamp<uInt32>(amount, 0, 100),
               newVertical = BSPF::clamp<uInt32>(vertical, 0, 100);

  if(newAmount == myAmount && newVertical == myVertical)
    return;

  myAmount = newAmount;
  myVertical = newVertical;
  myDirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IntSpan TVGeometry::indices() const
{
  static constexpr IndexArray INDICES = makeIndices();

  return myVertices.empty() ? IntSpan{} : IntSpan{INDICES};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TVGeometry::warp(float u, float v, float& x, float& y) const
{
  // Arc position on the glass, as an angle
  const float ax = u * myAlphaX, ay = v * myAlphaY;

  // The whole projection is this one factor, sin(theta)/theta
  const float s = sinc(std::sqrt(ax * ax + ay * ay));

  // Zero normalisation means a flat axis, which passes through unchanged
  x = myNormX > 0.F ? ax * s / myNormX : u;
  y = myNormY > 0.F ? ay * s / myNormY : v;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TVGeometry::unwarp(float& u, float& v) const
{
  if(!enabled())
    return;

  // Screen position, origin at the image centre
  const float x = u * 2.F - 1.F, y = v * 2.F - 1.F;

  // Undoing the normalisation leaves the arc vector scaled by
  // sin(theta)/theta, so its length is exactly sin(theta)
  const float px = x * myNormX, py = y * myNormY;
  const float r = std::sqrt(px * px + py * py);
  const float scale = r > 1.0E-6F
    ? std::asin(std::min(r, 1.F)) / r
    : 1.F;

  // An uncurved axis was never displaced, so leave it alone
  if(myAlphaX > 0.F)
    u = (px * scale / myAlphaX) * 0.5F + 0.5F;
  if(myAlphaY > 0.F)
    v = (py * scale / myAlphaY) * 0.5F + 0.5F;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TVGeometry::build(const Common::Rect& dst)
{
  if(!myDirty && dst == myRect)
    return;

  myRect = dst;
  myDirty = false;
  ++myGeneration;

  myVertices.clear();

  if(!enabled() || dst.w() == 0 || dst.h() == 0)
    return;

  const float width = static_cast<float>(dst.w()),
              height = static_cast<float>(dst.h());

  myAlphaX = MAX_ANGLE * static_cast<float>(myAmount) / 100.F;

  // A spherical face subtends angles in proportion to the extent of the
  // image, so scaling by the aspect ratio is what keeps the curvature
  // isotropic rather than squashed along one axis
  myAlphaY = myAlphaX * static_cast<float>(myVertical) / 100.F * height / width;

  myNormX = std::sin(myAlphaX);
  myNormY = std::sin(myAlphaY);

  const float halfW = width / 2.F, halfH = height / 2.F;
  const float centerX = static_cast<float>(dst.x()) + halfW,
              centerY = static_cast<float>(dst.y()) + halfH;

  myVertices.reserve(static_cast<size_t>(GRID_X + 1) * (GRID_Y + 1));
  for(uInt32 row = 0; row <= GRID_Y; ++row)
  {
    const float v = static_cast<float>(row) / GRID_Y * 2.F - 1.F;

    for(uInt32 col = 0; col <= GRID_X; ++col)
    {
      const float u = static_cast<float>(col) / GRID_X * 2.F - 1.F;
      float x{0.F}, y{0.F};

      warp(u, v, x, y);

      myVertices.push_back(Vertex{
        .x = centerX + x * halfW,
        .y = centerY + y * halfH,
        .u = u * 0.5F + 0.5F,
        .v = v * 0.5F + 0.5F
      });
    }
  }
}
