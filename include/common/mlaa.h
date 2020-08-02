// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// Copyright (C) 2012
//
// Authority FX, Inc.
// www.authorityfx.com

#ifndef __AFX_MORPH_ANTI_ALIASING_H__
#define __AFX_MORPH_ANTI_ALIASING_H__

#include <afx/image_container.h>
#include <afx/multithreader.h>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <cstdlib>

enum MLAA_Metric {
  MLAA_Value,
  MLAA_Intensity,
  MLAA_Lightness,
  MLAA_Luminance,
  MLAA_Lab
};

class MorphAA {
private:
  struct Diff {
    bool cl;
    bool cr;
    bool tl;
    bool t;
    bool tr;
    bool bl;
    bool b;
    bool br;

    Diff()
    {
      bool cl = false;
      bool cr = false;
      bool tl = false;
      bool t  = false;
      bool tr = false;
      bool bl = false;
      bool b  = false;
      bool br = false;
    };
  };

  struct Pixel_Info {
    bool dis_h;
    bool dis_v;
    bool dis_dir_h;
    bool dis_dir_v;
    int pos_h;
    int pos_v;
    int length_h;
    int length_v;
    bool do_blend_h;
    bool do_blend_v;
    bool orientation_h;
    bool orientation_v;

    Pixel_Info()
    {
      dis_h         = false;
      dis_v         = false;
      dis_dir_h     = false;
      dis_dir_v     = false;
      pos_h         = 0;
      pos_v         = 0;
      length_h      = 0;
      length_v      = 0;
      do_blend_h    = false;
      do_blend_v    = false;
      orientation_h = false;
      orientation_v = false;
    };
  };

  struct Blend_Weight {
    float top;
    float bottom;
    float left;
    float right;
  };

  struct Size {
    unsigned int width;
    unsigned int height;
  };

  class Image_Info {
  private:
    MorphAA::Size size;
    MorphAA::Pixel_Info* image_ptr;
    long long rowBytes;

  public:
    Image_Info(){};

    Image_Info(unsigned int width, unsigned int height)
    {
      create_image(width, height);
    }

    void create_image(unsigned int width, unsigned int height)
    {
      size.width  = width;
      size.height = height;
      image_ptr   = new MorphAA::Pixel_Info[size.width * size.height];
      rowBytes    = sizeof(MorphAA::Pixel_Info) * size.width;
    }

    MorphAA::Pixel_Info& pixel_at(unsigned int x, unsigned int y)
    {
      return *(MorphAA::Pixel_Info*)((char*)image_ptr +
                                     sizeof(MorphAA::Pixel_Info) *
                                         (y * size.width + x));
    }

    MorphAA::Pixel_Info* ptr_at(unsigned int x, unsigned int y)
    {
      return (MorphAA::Pixel_Info*)((char*)image_ptr +
                                    sizeof(MorphAA::Pixel_Info) *
                                        (y * size.width + x));
    }

    MorphAA::Size getSize() { return size; }

    long long getRowBytes() { return rowBytes; }

    ~Image_Info()
    {
      if (image_ptr) {
        delete[] image_ptr;
        image_ptr = 0;
      }
    }
  };

  // Reconstruct single pixels
  inline void recon_RGB(Bounds region, ImageContainerA* input,
                        ImageContainerA* recon, float recon_thresh)
  {
    float* p_r = 0;
    float* p_g = 0;
    float* p_b = 0;

    float* po_r = 0;
    float* po_g = 0;
    float* po_b = 0;

    for (int y = region.y1; y <= region.y2; y++) {
      p_r = input[0].ptr_at(region.x1, y);
      p_g = input[1].ptr_at(region.x1, y);
      p_b = input[2].ptr_at(region.x1, y);

      po_r = recon[0].ptr_at(region.x1, y);
      po_g = recon[1].ptr_at(region.x1, y);
      po_b = recon[2].ptr_at(region.x1, y);

      for (int x = region.x1; x <= region.x2; x++) {
        float RGB_c[3]  = {*p_r, *p_g, *p_b};
        float RGB_cl[3] = {*p_r, *p_g, *p_b};
        float RGB_cr[3] = {*p_r, *p_g, *p_b};

        float RGB_t[3]  = {*p_r, *p_g, *p_b};
        float RGB_tl[3] = {*p_r, *p_g, *p_b};
        float RGB_tr[3] = {*p_r, *p_g, *p_b};

        float RGB_b[3]  = {*p_r, *p_g, *p_b};
        float RGB_bl[3] = {*p_r, *p_g, *p_b};
        float RGB_br[3] = {*p_r, *p_g, *p_b};

        if (x > 0) {
          RGB_cl[0] = *(p_r - 1);
          RGB_cl[1] = *(p_r - 1);
          RGB_cl[2] = *(p_r - 1);
        }

        if (x < input[0].getSize().width - 1) {
          RGB_cr[0] = *(p_r + 1);
          RGB_cr[1] = *(p_g + 1);
          RGB_cr[2] = *(p_b + 1);
        }

        if (y > 0) {
          float* p_b_r = (float*)((char*)p_r - input[0].getRowSizeBytes());
          float* p_b_g = (float*)((char*)p_g - input[1].getRowSizeBytes());
          float* p_b_b = (float*)((char*)p_b - input[2].getRowSizeBytes());

          RGB_b[0] = *p_b_r;
          RGB_b[1] = *p_b_g;
          RGB_b[2] = *p_b_b;

          if (x > 0) {
            RGB_bl[0] = *(p_b_r - 1);
            RGB_bl[1] = *(p_b_g - 1);
            RGB_bl[2] = *(p_b_b - 1);
          }

          if (x < input[0].getSize().width - 1) {
            RGB_br[0] = *(p_b_r + 1);
            RGB_br[1] = *(p_b_g + 1);
            RGB_br[2] = *(p_b_b + 1);
          }
        }

        if (y < input[0].getSize().height - 1) {
          float* p_t_r = (float*)((char*)p_r + input[0].getRowSizeBytes());
          float* p_t_g = (float*)((char*)p_g + input[1].getRowSizeBytes());
          float* p_t_b = (float*)((char*)p_b + input[2].getRowSizeBytes());

          RGB_t[0] = *p_t_r;
          RGB_t[1] = *p_t_g;
          RGB_t[2] = *p_t_b;

          if (x > 0) {
            RGB_tl[0] = *(p_t_r - 1);
            RGB_tl[1] = *(p_t_g - 1);
            RGB_tl[2] = *(p_t_b - 1);
          }

          if (x < input[0].getSize().width - 1) {
            RGB_tr[0] = *(p_t_r + 1);
            RGB_tr[1] = *(p_t_g + 1);
            RGB_tr[2] = *(p_t_b + 1);
          }
        }

        float RGB_sum[3] = {0, 0, 0};
        int count        = 0;

        Diff diff;

        if (fabs(calcDif(RGB_c, RGB_cl, MLAA_Lab)) > recon_thresh) {
          diff.cl = true;
          addRGB(RGB_sum, RGB_cl);
          count += 1;
        }

        if (fabs(calcDif(RGB_c, RGB_cr, MLAA_Lab)) > recon_thresh) {
          diff.cr = true;
          addRGB(RGB_sum, RGB_cr);
          count += 1;
        }

        if (fabs(calcDif(RGB_c, RGB_tl, MLAA_Lab)) > recon_thresh) {
          diff.tl = true;
          addRGB(RGB_sum, RGB_tl);
          count += 1;
        }

        if (fabs(calcDif(RGB_c, RGB_t, MLAA_Lab)) > recon_thresh) {
          diff.t = true;
          addRGB(RGB_sum, RGB_t);
          count += 1;
        }

        if (fabs(calcDif(RGB_c, RGB_tr, MLAA_Lab)) > recon_thresh) {
          diff.tr = true;
          addRGB(RGB_sum, RGB_tr);
          count += 1;
        }

        if (fabs(calcDif(RGB_c, RGB_bl, MLAA_Lab)) > recon_thresh) {
          diff.bl = true;
          addRGB(RGB_sum, RGB_bl);
          count += 1;
        }

        if (fabs(calcDif(RGB_c, RGB_b, MLAA_Lab)) > recon_thresh) {
          diff.b = true;
          addRGB(RGB_sum, RGB_b);
          count += 1;
        }

        if (fabs(calcDif(RGB_c, RGB_br, MLAA_Lab)) > recon_thresh) {
          diff.br = true;
          addRGB(RGB_sum, RGB_br);
          count += 1;
        }

        bool do_blend = false;
        if (count == 2) {
          do_blend = true;

          if (diff.bl == true && (diff.b == true || diff.cl == true))
            do_blend = false;

          if (diff.br == true && (diff.b == true || diff.cr == true))
            do_blend = false;
          /*
                                                  if(diff.tl == true && (diff.t
             == true || diff.cl == true)) do_blend = false;

                                                  if(diff.tr == true && (diff.t
             == true || diff.cr == true)) do_blend = false;
                                                  */
        }

        if (do_blend == true) {
          *po_r = RGB_sum[0] * 0.5f;
          *po_g = RGB_sum[1] * 0.5f;
          *po_b = RGB_sum[2] * 0.5f;
        }
        else {
          *po_r = *p_r;
          *po_g = *p_g;
          *po_b = *p_b;
        }

        p_r++;
        p_g++;
        p_b++;

        po_r++;
        po_g++;
        po_b++;
      }
    }
  }
  // Set pixel discontinuities looking down and right
  inline void markDisc_A(Bounds region, ImageContainerA& input,
                         Image_Info& info, float threshold)
  {
    float* p      = 0;
    float* p_b    = 0;
    Pixel_Info* i = 0;

    for (int y = region.y1; y <= region.y2; y++) {
      p = input.ptr_at(region.x1, y);
      if (y > 0)
        p_b = input.ptr_at(region.x1, y - 1);
      i = info.ptr_at(region.x1, y);

      for (int x = region.x1; x <= region.x2; x++) {
        i->dis_h     = false;
        i->dis_v     = false;
        i->dis_dir_h = false;
        i->dis_dir_v = false;

        // Compare current pixel to bottom neighbour
        if (y > 0) {
          float dif = 2 * (*p - *p_b) / (*p + *p_b);  //(*p - *p_b);

          if (fabs(dif) >= threshold) {
            i->dis_h = true;
            if (dif >= 0)
              i->dis_dir_h = true;
            else
              i->dis_dir_h = false;
          }
        }

        // Compare current pixel to right neighbour
        if (x <= (int)info.getSize().width - 2) {
          float dif = (*p - *(p + 1));
          if (fabs(dif) >= threshold) {
            i->dis_v = true;
            if (dif >= 0)
              i->dis_dir_v = true;
            else
              i->dis_dir_v = false;
          }
        }

        i++;
        p++;
        if (y > 0)
          p_b++;
      }
    }
  }

  inline void markDisc_RGB(Bounds region, ImageContainerA* input,
                           Image_Info& info, float threshold, int metric)
  {
    float* p_r = 0;
    float* p_g = 0;
    float* p_b = 0;

    float* p_b_r = 0;
    float* p_b_g = 0;
    float* p_b_b = 0;

    Pixel_Info* i = 0;

    for (int y = region.y1; y <= region.y2; y++) {
      p_r = input[0].ptr_at(region.x1, y);
      p_g = input[1].ptr_at(region.x1, y);
      p_b = input[2].ptr_at(region.x1, y);
      if (y > 0) {
        p_b_r = input[0].ptr_at(region.x1, y - 1);
        p_b_g = input[1].ptr_at(region.x1, y - 1);
        p_b_b = input[2].ptr_at(region.x1, y - 1);
      }
      i = info.ptr_at(region.x1, y);

      for (int x = region.x1; x <= region.x2; x++) {
        // Compare current pixel to bottom neighbour
        if (y > 0) {
          float RGB1[3];
          float RGB2[3];

          RGB1[0] = *p_r;
          RGB1[1] = *p_g;
          RGB1[2] = *p_b;

          RGB2[0] = *p_b_r;
          RGB2[1] = *p_b_g;
          RGB2[2] = *p_b_b;

          float dif = calcDif(RGB1, RGB2, metric);

          if (fabs(dif) >= threshold) {
            i->dis_h = true;
            if (dif >= 0)
              i->dis_dir_h = true;
          }
        }

        // Compare current pixel to right neighbour
        if (x <= (int)info.getSize().width - 2) {
          float RGB1[3];
          float RGB2[3];

          RGB1[0] = *p_r;
          RGB1[1] = *p_g;
          RGB1[2] = *p_b;

          RGB2[0] = *(p_r + 1);
          RGB2[1] = *(p_g + 1);
          RGB2[2] = *(p_b + 1);

          float dif = calcDif(RGB1, RGB2, metric);

          if (fabs(dif) >= threshold) {
            i->dis_v = true;
            if (dif >= 0)
              i->dis_dir_v = true;
          }
        }

        i++;
        p_r++;
        p_g++;
        p_b++;
        if (y > 0) {
          p_b_r++;
          p_b_g++;
          p_b_b++;
        }
      }
    }
  }

  // Find horizontal lines
  inline void findHorLines(Bounds region, Image_Info& info)
  {
    Pixel_Info* i = 0;

    for (int y = region.y1; y <= region.y2; y++) {
      i = info.ptr_at(region.x1, y);

      int length = 0;  // Length of line, also used to determine if looking for
                       // new line or end of line.
      bool direction;  // Directoin of discontinuity

      for (int x = region.x1; x <= region.x2; x++) {
        // if loooking for new line
        if (length == 0) {
          // if new line found
          if (i->dis_h == true) {
            length++;
            direction = i->dis_dir_h;
          }
        }
        else  // Looking for end of line
        {
          // Line continues
          if (i->dis_h == true && x < (int)info.getSize().width - 1 &&
              direction == i->dis_dir_h) {
            length++;
          }
          else  // Found end of line
          {
            // If not end of row
            if (x < (int)info.getSize().width - 1) {
              bool do_blend_start = false;
              bool do_blend_end   = false;

              bool start_orientation = false;
              bool end_orientation   = false;

              // Determine end line orientation

              if ((i - 1)->dis_v == true) {
                end_orientation = false;
                do_blend_end    = true;
              }
              else if (y > 0) {
                end_orientation = true;
                do_blend_end    = true;
              }

              if (x - length - 1 > 0)  // Start of line is NOT x = 0
              {
                if ((i - length - 1)->dis_v == true) {
                  start_orientation = true;
                  do_blend_start    = true;
                }
                else {
                  start_orientation = false;
                  do_blend_start    = true;
                }
              }

              // Loop back through line and set pixel info
              Pixel_Info* l_i = i;
              for (int pos = length - 1; pos >= length / 2; pos--) {
                l_i--;
                l_i->pos_h         = pos;
                l_i->length_h      = length;
                l_i->orientation_h = end_orientation;
                l_i->do_blend_h    = do_blend_end;
              }

              for (int pos = length / 2 - 1; pos >= 0; pos--) {
                l_i--;
                l_i->pos_h         = pos;
                l_i->length_h      = length;
                l_i->orientation_h = start_orientation;
                l_i->do_blend_h    = do_blend_start;
              }
            }

            // Research new line form this pixel
            x--;
            i--;
            length = 0;
          }
        }
        i++;
      }
    }
  }
  // Find vertical lines
  inline void findVertLines(Bounds region, Image_Info& info)
  {
    Pixel_Info* i = 0;

    for (int x = region.x1; x <= region.x2; x++) {
      i = info.ptr_at(x, region.y1);

      int length = 0;  // Length of line, also used to determine if looking for
                       // new line or end of line.
      bool direction;  // Directoin of discontinuity

      for (int y = region.y1; y <= region.y2; y++) {
        // Initialize paramters
        i->length_v      = 0;
        i->do_blend_v    = false;
        i->orientation_v = false;
        i->pos_v         = 0;

        // if loooking for new line
        if (length == 0) {
          // if new line found
          if (i->dis_v == true) {
            length++;
            direction = i->dis_dir_v;
          }
        }
        else  // Looking for end of line
        {
          // Line continues
          if (i->dis_v == true && y < (int)info.getSize().height - 1 &&
              direction == i->dis_dir_v) {
            length++;
          }
          else  // Found end of line
          {
            // If not end of row
            if (y < (int)info.getSize().height) {
              bool do_blend_start = false;
              bool do_blend_end   = false;

              bool start_orientation = false;
              bool end_orientation   = false;

              // Determine end line orientation

              if (i->dis_h == true) {
                end_orientation = false;
                do_blend_end    = true;
              }
              else if (x < (int)info.getSize().width - 1) {
                end_orientation = true;
                do_blend_end    = true;
              }

              if (y - length - 1 > 0)  // Start of line is NOT y = 0
              {
                if (((Pixel_Info*)((char*)i - length * info.getRowBytes()))
                        ->dis_h == true) {
                  start_orientation = true;
                  do_blend_start    = true;
                }
                else {
                  start_orientation = false;
                  do_blend_start    = true;
                }
              }

              // Loop back through line and set pixel info
              Pixel_Info* l_i = i;
              for (int pos = length - 1; pos >= length / 2; pos--) {
                l_i           = (Pixel_Info*)((char*)l_i - info.getRowBytes());
                l_i->pos_v    = pos;
                l_i->length_v = length;
                l_i->orientation_v = end_orientation;
                l_i->do_blend_v    = do_blend_end;
              }

              for (int pos = length / 2 - 1; pos >= 0; pos--) {
                l_i           = (Pixel_Info*)((char*)l_i - info.getRowBytes());
                l_i->pos_v    = pos;
                l_i->length_v = length;
                l_i->orientation_v = start_orientation;
                l_i->do_blend_v    = do_blend_start;
              }
            }

            // Research new line form this pixel
            y--;
            i      = (Pixel_Info*)((char*)i - info.getRowBytes());
            length = 0;
          }
        }
        i = (Pixel_Info*)((char*)i + info.getRowBytes());
      }
    }
  }

  // Blend pixels
  inline void blendPixels_A(Bounds region, ImageContainerA& input,
                            ImageContainerA& output, Image_Info& info)
  {
    float* i_p    = 0;
    float* o_p    = 0;
    Pixel_Info* i = 0;

    for (int y = region.y1; y <= region.y2; y++) {
      i_p = input.ptr_at(region.x1, y);
      o_p = output.ptr_at(region.x1, y);
      i   = info.ptr_at(region.x1, y);

      for (int x = region.x1; x <= region.x2; x++) {
        Blend_Weight blend = {0, 0, 0, 0};

        calcBlendingWeights(blend, info, i, x, y);

        float old_val = *i_p;
        float new_val = 0.0f;

        float h_sum = blend.top + blend.bottom;
        float v_sum = blend.right + blend.left;

        // Blend pixel with neighbours, need if to prevent null pointer.
        if (blend.top > 0 && v_sum <= h_sum)
          new_val +=
              *(float*)((char*)i_p + input.getRowSizeBytes()) * blend.top;
        else
          blend.top = 0;

        if (blend.bottom > 0 && v_sum <= h_sum)
          new_val +=
              *(float*)((char*)i_p - input.getRowSizeBytes()) * blend.bottom;
        else
          blend.bottom = 0;

        if (blend.left > 0 && v_sum >= h_sum)
          new_val += *(i_p - 1) * blend.left;
        else
          blend.left = 0;

        if (blend.right > 0 && v_sum >= h_sum)
          new_val += *(i_p + 1) * blend.right;
        else
          blend.right = 0;

        new_val += old_val *
                   (1.0f - blend.top - blend.bottom - blend.right - blend.left);

        // Set output pixel to new value
        *o_p = new_val;

        i_p++;
        o_p++;
        i++;
      }
    }
  }

  inline void blendPixels_RGB(Bounds region, ImageContainerA* input,
                              ImageContainerA* output, Image_Info& info)
  {
    float* ip_r = 0;
    float* ip_g = 0;
    float* ip_b = 0;

    float* op_r = 0;
    float* op_g = 0;
    float* op_b = 0;

    Pixel_Info* i = 0;

    for (int y = region.y1; y <= region.y2; y++) {
      ip_r = input[0].ptr_at(region.x1, y);
      ip_g = input[1].ptr_at(region.x1, y);
      ip_b = input[2].ptr_at(region.x1, y);

      op_r = output[0].ptr_at(region.x1, y);
      op_g = output[1].ptr_at(region.x1, y);
      op_b = output[2].ptr_at(region.x1, y);

      i = info.ptr_at(region.x1, y);

      for (int x = region.x1; x <= region.x2; x++) {
        Blend_Weight blend = {0, 0, 0, 0};

        // Calculate blending weights
        calcBlendingWeights(blend, info, i, x, y);

        float old_val_r = *ip_r;
        float old_val_g = *ip_g;
        float old_val_b = *ip_b;

        float new_val_r = 0.0f;
        float new_val_g = 0.0f;
        float new_val_b = 0.0f;

        float h_sum = blend.top + blend.bottom;
        float v_sum = blend.right + blend.left;

        if (blend.top > 0 && v_sum <= h_sum) {
          new_val_r +=
              *(float*)((char*)ip_r + input[0].getRowSizeBytes()) * blend.top;
          new_val_g +=
              *(float*)((char*)ip_g + input[1].getRowSizeBytes()) * blend.top;
          new_val_b +=
              *(float*)((char*)ip_b + input[2].getRowSizeBytes()) * blend.top;
        }
        else
          blend.top = 0;

        if (blend.bottom > 0 && v_sum <= h_sum) {
          new_val_r += *(float*)((char*)ip_r - input[0].getRowSizeBytes()) *
                       blend.bottom;
          new_val_g += *(float*)((char*)ip_g - input[1].getRowSizeBytes()) *
                       blend.bottom;
          new_val_b += *(float*)((char*)ip_b - input[2].getRowSizeBytes()) *
                       blend.bottom;
        }
        else
          blend.bottom = 0;

        if (blend.left > 0 && v_sum >= h_sum) {
          new_val_r += *(ip_r - 1) * blend.left;
          new_val_g += *(ip_g - 1) * blend.left;
          new_val_b += *(ip_b - 1) * blend.left;
        }
        else
          blend.left = 0;

        if (blend.right > 0 && v_sum >= h_sum) {
          new_val_r += *(ip_r + 1) * blend.right;
          new_val_g += *(ip_g + 1) * blend.right;
          new_val_b += *(ip_b + 1) * blend.right;
        }
        else
          blend.right = 0;

        new_val_r += old_val_r * (1.0f - blend.top - blend.bottom -
                                  blend.right - blend.left);
        new_val_g += old_val_g * (1.0f - blend.top - blend.bottom -
                                  blend.right - blend.left);
        new_val_b += old_val_b * (1.0f - blend.top - blend.bottom -
                                  blend.right - blend.left);

        // Set output pixel to new value
        *op_r = new_val_r;
        *op_g = new_val_g;
        *op_b = new_val_b;
        /*
         *op_r = *ip_r;
         *op_g = *ip_g;
         *op_b = *ip_b;
         */

        ip_r++;
        ip_g++;
        ip_b++;

        op_r++;
        op_g++;
        op_b++;

        i++;
      }
    }
  }

  // Calculate trapazoid area under blendign line
  float calcTrapArea(int pos, int length)
  {
    return abs(1.0f / (2 * (float)length) -
               ((float)length / 2.0f - (float)pos) / (float)length);
  };

  // Calculate difference bewteen two pixel based on different mertrics.
  float calcDif(float* RGB1, float* RGB2, int metric)
  {
    float dif;

    switch (metric) {
      case MLAA_Value: {
        float val1 =
            std::max<float>(std::max<float>(RGB1[0], RGB1[1]), RGB1[2]);
        float val2 =
            std::max<float>(std::max<float>(RGB2[0], RGB2[1]), RGB2[2]);
        dif = val1 - val2;

        break;
      }

      case MLAA_Intensity: {
        float val1 = (RGB1[0] + RGB1[1] + RGB1[2]) / 3.0f;
        float val2 = (RGB2[0] + RGB2[1] + RGB2[2]) / 3.0f;
        dif        = val1 - val2;

        break;
      }

      case MLAA_Lightness: {
        float val1 =
            (std::max<float>(std::max<float>(RGB1[0], RGB1[1]), RGB1[2]) +
             std::min<float>(std::min<float>(RGB1[0], RGB1[1]), RGB1[2])) *
            0.5f;
        float val2 =
            (std::max<float>(std::max<float>(RGB2[0], RGB2[1]), RGB2[2]) +
             std::min<float>(std::min<float>(RGB2[0], RGB2[1]), RGB2[2])) *
            0.5f;
        dif = val1 - val2;

        break;
      }

      case MLAA_Luminance: {
        float val1 = 0.3f * RGB1[0] + 0.59f * RGB1[1] + 0.11f * RGB1[2];
        float val2 = 0.3f * RGB2[0] + 0.59f * RGB2[1] + 0.11f * RGB2[2];
        dif        = val1 - val2;

        break;
      }

      case MLAA_Lab: {
        float Lab1[3];
        float Lab2[3];
        RGBtoLab(RGB1, Lab1);
        RGBtoLab(RGB2, Lab2);

        dif = 0.01f * calcDistLab(Lab1, Lab2);

        break;
      }
    }

    return dif;
  }

  // Calculate blending weights per pixel
  void calcBlendingWeights(Blend_Weight& blend, Image_Info& info, Pixel_Info* i,
                           unsigned int x, unsigned int y)
  {
    Pixel_Info zero_pixel;

    Pixel_Info* i_up     = 0;
    Pixel_Info* i_left   = 0;
    Pixel_Info* i_bottom = 0;
    Pixel_Info* i_right  = 0;

    if (y < info.getSize().height - 1)
      i_up = (Pixel_Info*)((char*)i + info.getRowBytes());
    else
      i_up = &zero_pixel;

    if (y > 0)
      i_bottom = (Pixel_Info*)((char*)i - info.getRowBytes());
    else
      i_bottom = &zero_pixel;

    if (x < info.getSize().width - 1)
      i_right = (i + 1);
    else
      i_right = &zero_pixel;

    if (x > 0)
      i_left = (i - 1);
    else
      i_left = &zero_pixel;

    if (y > 0) {
      if (i->do_blend_h == true) {
        if (i->length_h == 1 && i_bottom->length_v <= 1)
          blend.bottom = 0.125f;
        else if (i->pos_h < i->length_h / 2 && i->orientation_h == true)
          blend.bottom = calcTrapArea(i->pos_h, i->length_h);
        else if (i->pos_h >= i->length_h / 2 && i->orientation_h == false)
          blend.bottom = calcTrapArea(i->pos_h, i->length_h);
      }
    }

    if (y < info.getSize().height - 1) {
      if (i_up->do_blend_h == true) {
        if (i_up->length_h == 1 && i_up->length_v <= 1)
          blend.top = 0.125f;
        else if (i_up->pos_h < i_up->length_h / 2 &&
                 i_up->orientation_h == false)
          blend.top = calcTrapArea(i_up->pos_h, i_up->length_h);
        else if (i_up->pos_h >= i_up->length_h / 2 &&
                 i_up->orientation_h == true)
          blend.top = calcTrapArea(i_up->pos_h, i_up->length_h);
      }
    }

    if (x < info.getSize().width - 1) {
      if (i->do_blend_v == true) {
        if (i->length_v == 1 && i_right->length_h <= 1)
          blend.right = 0.125f;
        else if (i->pos_v < i->length_v / 2 && i->orientation_v == true)
          blend.right = calcTrapArea(i->pos_v, i->length_v);
        else if (i->pos_v >= i->length_v / 2 && i->orientation_v == false)
          blend.right = calcTrapArea(i->pos_v, i->length_v);
      }
    }

    if (x > 0) {
      if (i_left->do_blend_v == true) {
        if (i_left->length_v == 1 && i_left->length_h <= 1)
          blend.left = 0.125f;
        else if (i_left->pos_v < i_left->length_v / 2 &&
                 i_left->orientation_v == false)
          blend.left = calcTrapArea(i_left->pos_v, i_left->length_v);
        else if (i_left->pos_v >= i_left->length_v / 2 &&
                 i_left->orientation_v == true)
          blend.left = calcTrapArea(i_left->pos_v, i_left->length_v);
      }
    }
  }
  // Convert RGB to LAB
  void RGBtoLab(float* RGB, float* Lab)
  {
    float X = 0.412453f * RGB[0] + 0.357580f * RGB[1] + 0.180423f * RGB[2];
    float Y = 0.212671f * RGB[0] + 0.715160f * RGB[1] + 0.072169f * RGB[2];
    float Z = 0.019334f * RGB[0] + 0.119193f * RGB[1] + 0.950227f * RGB[2];

    float Xn = 0.950456f;
    float Yn = 1.0f;
    float Zn = 1.088754f;

    float YYn = Y / Yn;
    float XXn = X / Xn;
    float ZZn = Z / Zn;

    if (YYn > 0.008856f) {
      Lab[0] = 116.0f * pow(YYn, 1.0f / 3.0f) - 16.0f;
      Lab[1] = 500.0f * (pow(XXn, 3.0f) - pow(YYn, 3.0f));
      Lab[2] = 200.0f * (pow(YYn, 3.0f) - pow(ZZn, 3.0f));
    }
    else {
      float A = 16.0f / 116.0f;

      Lab[0] = 903.3f * YYn;
      Lab[1] = 500.0f * ((7.787f * XXn + A) - (7.787f * YYn + A));
      Lab[2] = 200.0f * ((7.787f * YYn + A) - (7.787f * ZZn + A));
    }
  }

  // Calculate Euclidean distance between two LAB values
  float calcDistLab(float* Lab1, float* Lab2)
  {
    float dist =
        sqrt(pow(Lab2[0] - Lab1[0], 2.0f) + pow(Lab2[1] - Lab1[1], 2.0f) +
             pow(Lab2[2] - Lab1[2], 2.0f));
    return Lab1[0] > Lab2[0] ? dist : -dist;
  }

  // add two RGB pointers
  void addRGB(float* RGB1, float* RGB2)
  {
    RGB1[0] += RGB2[0];
    RGB1[1] += RGB2[1];
    RGB1[2] += RGB2[2];
  }

public:
  MorphAA(){};
  void doAA_A(ImageContainerA& input, ImageContainerA& output, float threshold)
  {
    float aa_thresh = 1.0f - std::max(std::min(threshold, 1.0f), 0.0f);

    unsigned int width  = input.getSize().width;
    unsigned int height = input.getSize().height;

    Image_Info info(width, height);

    // Mark vartical and horizontal edge flags
    {
      Multithreader threader(Bounds(0, 0, width - 1, height - 1));
      threader.processInChunks(boost::bind(&MorphAA::markDisc_A, this, _1,
                                           boost::ref(input), boost::ref(info),
                                           aa_thresh));
    }

    // Find horizontal primary lines
    {
      Multithreader threader(Bounds(0, 0, width - 1, height - 1));
      threader.processInChunks(
          boost::bind(&MorphAA::findHorLines, this, _1, boost::ref(info)));
    }

    // Find vertical primary lines
    {
      Multithreader threader(Bounds(0, 0, width - 1, height - 1));
      threader.processInChunksX(
          boost::bind(&MorphAA::findVertLines, this, _1, boost::ref(info)));
    }

    // Blend pixels
    {
      Multithreader threader(Bounds(0, 0, width - 1, height - 1));
      threader.processInChunks(
          boost::bind(&MorphAA::blendPixels_A, this, _1, boost::ref(input),
                      boost::ref(output), boost::ref(info)));
    }
  }

  void doAA_RGB(ImageContainerA* input, ImageContainerA* output,
                float threshold, int metric, float recon_thresh)
  {
    float aa_thresh = 1.0f - std::max(std::min(threshold, 1.0f), 0.0f);

    unsigned int width  = input[0].getSize().width;
    unsigned int height = input[0].getSize().height;

    Image_Info info(width, height);

    ImageContainerA recon[3];
    for (int i = 0; i < 3; i++) {
      recon[i].create_image(width, height);
    }

    /*
    //Reconstruct single pixels
    {
            Multithreader threader(Bounds(0, 0, width - 1, height - 1));
            threader.processInChunks(boost::bind(&MorphAA::recon_RGB, this, _1,
    input, recon, recon_thresh));
    }
    */

    // Mark vartical and horizontal edge flags
    {
      Multithreader threader(Bounds(0, 0, width - 1, height - 1));
      threader.processInChunks(boost::bind(&MorphAA::markDisc_RGB, this, _1,
                                           input, boost::ref(info), aa_thresh,
                                           metric));
    }

    // Find horizontal primary lines
    {
      Multithreader threader(Bounds(0, 0, width - 1, height - 1));
      threader.processInChunks(
          boost::bind(&MorphAA::findHorLines, this, _1, boost::ref(info)));
    }

    // Find vertical primary lines
    {
      Multithreader threader(Bounds(0, 0, width - 1, height - 1));
      threader.processInChunksX(
          boost::bind(&MorphAA::findVertLines, this, _1, boost::ref(info)));
    }

    // Blend pixels
    {
      Multithreader threader(Bounds(0, 0, width - 1, height - 1));
      threader.processInChunks(boost::bind(&MorphAA::blendPixels_RGB, this, _1,
                                           input, output, boost::ref(info)));
    }
  }
};

#endif
