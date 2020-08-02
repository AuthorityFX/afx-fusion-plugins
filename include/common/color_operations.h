// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright (C) 2012
//
// Authority FX, Inc.
// www.authorityfx.com

#ifndef __COLOR_OPERATIONS_H__
#define __COLOR_OPERATIONS_H__
#include <cmath>

enum suppresion_method {
  light       = 0,
  medium      = 1,
  hard        = 2,
  harder      = 3,
  even_harder = 4,
  extreme     = 5
};

template <typename T>
inline T clamp(T value, T min, T max);
inline float afx_abs(float value);
inline void RGBtoHSV(float* RGB, float* HSV);
inline void HSVtoRGB(float* HSV, float* RGB);
inline float spillSupression(float* RGB, int algorithm);

class RotateColor {
private:
  float _m[3][3];

  float* multiplyMatrix(float* color);

public:
  RotateColor();
  RotateColor(float deg);
  RotateColor(float* axis, float deg);
  void buildMatrix(float* axis, float deg);
  void buildMatrix(float deg);
  inline void applyTransform(float* color);
  void invertTransform();
  float getMatrix_at(int r, int c);
};

RotateColor::RotateColor()
{
  // Initialize identity matrix.
  for (int r = 0; r < 3; r++) {
    for (int c = 0; c < 3; c++) {
      if (c == r) {
        _m[r][c] = 1.0f;
      }
      else {
        _m[r][c] = 0.0f;
      }
    }
  }
}

RotateColor::RotateColor(float deg) { buildMatrix(deg); }

RotateColor::RotateColor(float* axis, float deg) { buildMatrix(axis, deg); }

void RotateColor::buildMatrix(float* axis, float deg)
{
  const float PI = 3.14159265359f;

  float angle = deg * PI / 180.0f;

  float cosA = cos(angle);
  float sinA = sin(angle);

  float x  = axis[0];
  float y  = axis[1];
  float z  = axis[2];
  float xx = x * x;
  float yy = y * y;
  float zz = z * z;
  float xy = x * y;
  float xz = x * z;
  float yz = y * z;

  // Rodrigues' rotation formula
  // http://en.wikipedia.org/wiki/Rodrigues'_rotation_formula

  _m[0][0] = cosA + (xx) * (1 - cosA);
  _m[0][1] = (xy) * (1 - cosA) - (z)*sinA;
  _m[0][2] = (xz) * (1 - cosA) + (y)*sinA;

  _m[1][0] = (xy) * (1 - cosA) + (z)*sinA;
  _m[1][1] = cosA + (yy) * (1 - cosA);
  _m[1][2] = -(x)*sinA + (yz) * (1 - cosA);

  _m[2][0] = (xz) * (1 - cosA) - (y)*sinA;
  _m[2][1] = (yz) * (1 - cosA) + (x)*sinA;
  _m[2][2] = cosA + (zz) * (1 - cosA);
}

void RotateColor::buildMatrix(float deg)
{
  const float PI = 3.14159265359f;

  float angle = deg * PI / 180.0f;

  float cosA = cos(angle);
  float sinA = sin(angle);

  float x  = 1.0f / sqrt(3.0f);
  float xx = 1.0f / 3.0f;

  _m[0][0] = cosA + (xx) * (1 - cosA);
  _m[0][1] = (xx) * (1 - cosA) - (x)*sinA;
  _m[0][2] = (xx) * (1 - cosA) + (x)*sinA;

  _m[1][0] = (xx) * (1 - cosA) + (x)*sinA;
  _m[1][1] = cosA + (xx) * (1 - cosA);
  _m[1][2] = -(x)*sinA + (xx) * (1 - cosA);

  _m[2][0] = (xx) * (1 - cosA) - (x)*sinA;
  _m[2][1] = (xx) * (1 - cosA) + (x)*sinA;
  _m[2][2] = cosA + (xx) * (1 - cosA);
}

inline void RotateColor::applyTransform(float* color)
{
  float temp[3] = {0.0f, 0.0f, 0.0f};
  for (int r = 0; r < 3; r++) {
    for (int c = 0; c < 3; c++) {
      temp[r] += _m[r][c] * color[c];
    }
  }

  for (int i = 0; i < 3; i++) {
    color[i] = temp[i];
  }
}

void RotateColor::invertTransform()
{
  for (int r = 0; r < 3; r++) {
    for (int c = r; c < 3; c++) {
      float temp;
      temp     = _m[r][c];
      _m[r][c] = _m[c][r];
      _m[c][r] = temp;
    }
  }
}

float RotateColor::getMatrix_at(int r, int c) { return _m[r][c]; }

template <typename T>
inline T clamp(T value, T min, T max)
{
  return std::max(std::min(value, max), min);
}
inline float afx_abs(float value) { return value < 0 ? (-value) : value; }
inline void RGBtoHSV(float* RGB, float* HSV)
{
  float* r;
  float* g;
  float* b;

  r = &RGB[0];
  g = &RGB[1];
  b = &RGB[2];

  float H;

  float m = std::min(*r, std::min(*g, *b));
  float M =
      std::max(std::max(*r, std::max(*g, *b)), 0.001f);  // M must never be zero
  float C = std::max(M - m, 0.001f);                     // C must ever be zero

  if (M == *r) {
    H = (*g - *b) / C;

    if (H < 0.0)
      H += 6.0f;
  }
  else if (M == *g)
    H = ((*b - *r) / C) + 2.0f;
  else if (M == *b)
    H = ((*r - *g) / C) + 4.0f;
  else
    H = 0;

  HSV[0] = std::max(std::min(H / 6.0f, 1.0f), 0.0f);
  HSV[1] = C / M;
  HSV[2] = M;

  if (M == 0) {
    HSV[0] = 0;
    HSV[1] = 0;
    HSV[2] = 0;
  }
}
inline void HSVtoRGB(float* HSV, float* RGB)
{
  float H = HSV[0];
  float S = HSV[1];
  float V = HSV[2];

  float R = 0;
  float G = 0;
  float B = 0;

  float C = V * S;

  double Ho = 6.0f * H;

  float X = C * (1.0f - afx_abs((float)fmod(Ho, 2.0) - 1.0f));

  if (Ho >= 0 && Ho < 1) {
    R = C;
    G = X;
    B = 0;
  }
  else if (Ho >= 1 && Ho < 2) {
    R = X;
    G = C;
    B = 0;
  }
  else if (Ho >= 2 && Ho < 3) {
    R = 0;
    G = C;
    B = X;
  }
  else if (Ho >= 3 && Ho < 4) {
    R = 0;
    G = X;
    B = C;
  }
  else if (Ho >= 4 && Ho < 5) {
    R = X;
    G = 0;
    B = C;
  }
  else if (Ho >= 5) {
    R = C;
    G = 0;
    B = X;
  }

  float m = V - C;

  RGB[0] = R + m;
  RGB[1] = G + m;
  RGB[2] = B + m;
}
inline float spillSupression(float* RGB, int algorithm)
{
  float temp        = 0.0f;
  float suppression = 0.0f;

  switch (algorithm) {
    case light:

      temp = RGB[1] - std::max(RGB[0], RGB[2]);
      if (temp > 0)
        suppression = temp;

      break;

    case medium:

      temp = 2 * RGB[1] - std::max(RGB[0], RGB[2]) - (RGB[0] + RGB[2]) / 2.0f;
      if (temp > 0)
        suppression = temp / 2.0f;

      break;

    case hard:

      temp = RGB[1] - (RGB[0] + RGB[2]) / 2.0f;
      if (temp > 0)
        suppression = temp;

      break;

    case harder:

      temp = 3 * RGB[1] - (RGB[0] + RGB[2]) - std::min(RGB[0], RGB[2]);
      if (temp > 0)
        suppression = temp / 3.0f;

      break;

    case even_harder:

      temp = 2 * RGB[1] - (RGB[0] + RGB[2]) / 2.0f - std::min(RGB[0], RGB[2]);
      if (temp > 0)
        suppression = temp / 2.0f;

      break;

    case extreme:

      temp =
          3 * RGB[1] - (RGB[0] + RGB[2]) / 2.0f - 2 * std::min(RGB[0], RGB[2]);
      if (temp > 0)
        suppression = temp / 3.0f;

      break;
  }

  return suppression;
}

#endif
