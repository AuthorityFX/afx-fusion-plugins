// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright (C) 2012
//
// Authority FX, Inc.
// www.authorityfx.com

#ifndef __VECTOR2D_H__
#define __VECTOR2D_H__

class Vector2D {
public:
  double x;
  double y;

  Vector2D() : x(0.0), y(0.0) {}

  Vector2D(double a, double b) : x(a), y(b) {}

  // Returns a vector of length 1.0 in the same direction
  Vector2D normalize() const
  {
    Vector2D out;
    out.x = x / norm();
    out.y = y / norm();
    return out;
  }

  // Returns the length of the vector
  double norm() const { return sqrt(x * x + y * y); }

  // Returns a perpendicular vector
  Vector2D perp() const
  {
    Vector2D out;
    out.x = -y;
    out.y = x;
    return out;
  }

  // Returns the dot product of this vector and the given vector 'a'
  double dot(Vector2D const& a) const { return a.x * x + a.y * y; }

  // Returns the dot product of two vectors
  static double dot(Vector2D const& a, Vector2D const& b)
  {
    return a.x * b.x + a.y * b.y;
  }
};

inline Vector2D operator+(Vector2D const& a, Vector2D const& b)
{
  Vector2D out;
  out.x = a.x + b.x;
  out.y = a.y + b.y;
  return out;
}

inline Vector2D operator-(Vector2D const& a, Vector2D const& b)
{
  Vector2D out;
  out.x = a.x - b.x;
  out.y = a.y - b.y;
  return out;
}

inline Vector2D operator*(Vector2D const& a, double c)
{
  Vector2D out(c * a.x, c * a.y);
  return out;
}

inline Vector2D operator*(double c, Vector2D const& a)
{
  Vector2D out(c * a.x, c * a.y);
  return out;
}

#endif __VECTOR2D_H__
