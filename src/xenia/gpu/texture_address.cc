/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/gpu/texture_address.h"

#include <algorithm>

#include "xenia/base/assert.h"
#include "xenia/base/math.h"

namespace xe {
namespace gpu {
namespace texture_address {

// Invokes the callback for the intersection if it's not empty.
static void EmitBoxIntersection(
    const std::function<void(const Box&)>& box_callback, const Box& a,
    const Box& b) {
  assert_true(box_count_ref < kPageBoundingVolumeMaxBoxes);
  Box intersection;
  for (size_t axis = 0; axis < a.extent.size(); ++axis) {
    assert_true(a.extent[axis] <= UINT32_MAX - a.offset[axis]);
    assert_true(b.extent[axis] <= UINT32_MAX - b.offset[axis]);
    const uint32_t lower_bound = std::max(a.offset[axis], b.offset[axis]);
    const uint32_t upper_bound = std::min(a.offset[axis] + a.extent[axis],
                                          b.offset[axis] + b.extent[axis]);
    if (lower_bound >= upper_bound) {
      return;
    }
    intersection.offset[axis] = lower_bound;
    intersection.extent[axis] = upper_bound - lower_bound;
  }
  box_callback(intersection);
}

// In the implementations of the functions, when working with block-linear
// grids (of linear elements, or of macro tiles), note that in a situation, for
// example, where the page range covers a part of one row on the right, a part
// of the next row on the left, but there are no fully covered rows between
// them, the two rows still must end up in two different boxes, not merged. The
// same applies along the Z axis as well: the box for the bottom one slice can't
// be merged with the box for the top of the next slice.
//
// Often, here lower bounds are rounded up, and upper bound are rounded down.
// This may result in one or both boundaries going out of the range of the pages
// specified by the caller. In such cases, `begin < end` checks (or at least
// `begin <= end`, but there's nothing to do for an empty range) are necessary
// to ensure the bounds are within the page range.

// Bits of the coordinates within a macro tile in the page index:
// - 2D:
//   - 16 bytes per element:
//     - [13:12] = outer_inner_bytes[9:8] = inner_elements[5:4] = y[3:2]
//   - 8 bytes per element:
//     - [12] = outer_inner_bytes[8] = inner_elements[5] = y[3]
// - 3D:
//   - 16 bytes per element:
//     - [14:13] = outer_inner_bytes[10:9] = inner_elements[6:5] = z[1:0]
//     - [12]    = outer_inner_bytes[8]    = inner_elements[4]   = y[2]
//   - 8 bytes per element:
//     - [13:12] = outer_inner_bytes[9:8] = inner_elements[6:5] = z[1:0]
//   - 4 bytes per element:
//     - [12] = outer_inner_bytes[8] = inner_elements[6] = z[1]
//
// Notice that for large numbers of bytes per element, Y[3:2] (2D) or Y[2] is
// included in the page index. However, in a tiled address, Y[4] (2D) or Y[3] is
// specified below the page bits. So, if a page is not aligned to at least a
// full Z slice of a micro tile, 2 rectangles with a width of 32 and a height
// granularity of 4 are needed to cover that page, 16 (2D) or 8 (3D) element
// rows apart from each other.

void PageBoundingVolumeTiled2D(
    const std::function<void(const Box&)> box_callback, uint32_t pages_begin,
    const uint32_t pages_end, const Box& intersect_box,
    const unsigned int bytes_per_element_log2, uint32_t pitch_aligned) {
#ifndef NDEBUG
  // Don't allow ranges that may cause an integer overflow, especially when
  // addressing macro tiles, which may be smaller than pages depending on the
  // number of bytes per element. 4 is the log2 of the maximum number of bytes
  // per element.
  constexpr uint32_t max_subresource_page_count =
      uint32_t(1) << (4 + kStoragePitchBitCount + kMax2DWidthHeightPixelsLog2 -
                      kPageBytesLog2);
  assert_true(pages_begin <= max_subresource_page_count);
  assert_true(pages_end <= max_subresource_page_count);
#endif

  if (!pitch_aligned) {
    // Pitch smaller than width is unsupported by the bounding volume
    // calculation, however it can be specified in the register anyway, prevent
    // division by zero. Specify the maximum width as the pitch, which would
    // also be correct for a small texture consisting of only 1 row of macro
    // tiles, for which the pitch doesn't matter.
    pitch_aligned = kMax2DWidthHeightPixels;
  }
  assert_zero(pitch_aligned & (kStoragePitchHeightAlignmentElements - 1));
  const uint32_t pitch_macro_tiles = pitch_aligned >> kMacroTileWidthLog2;

  constexpr unsigned int macro_tile_elements_log2 =
      kMacroTileWidthLog2 + kMacroTileHeight2DLog2;

  const auto page_macro_tile_index =
      [macro_tile_elements_log2,
       bytes_per_element_log2](const uint32_t page) -> uint32_t {
    return page << (kPageBytesLog2 - macro_tile_elements_log2) >>
           bytes_per_element_log2;
  };

  const unsigned int page_micro_y_bit_count =
      std::max(int(bytes_per_element_log2) -
                   int(kPageBytesLog2 - macro_tile_elements_log2),
               0);
  const unsigned int page_micro_y_step_log2 = 4 - page_micro_y_bit_count;
  const uint32_t page_micro_y_mask =
      (uint32_t(1) << page_micro_y_bit_count) - 1;

  // `pages_begin` must be moved forward by the number of pages contained within
  // the boxes that will be added for the front macro tile, so if the first
  // macro tile is also the last one, the same regions within it won't be added
  // twice.

  const auto emit_micro_y_rows_and_advance_pages_begin =
      [&](const uint32_t micro_y_pages_begin,
          const uint32_t micro_y_pages_end) {
        if (micro_y_pages_begin >= micro_y_pages_end) {
          return;
        }
        const uint32_t macro_tile = page_macro_tile_index(micro_y_pages_begin);
        Box box;
        box.extent[0] = kMacroTileWidth;
        box.extent[1] = (micro_y_pages_end - micro_y_pages_begin)
                        << page_micro_y_step_log2;
        box.offset[0] = (macro_tile % pitch_macro_tiles) << kMacroTileWidthLog2;
        box.offset[1] =
            ((macro_tile / pitch_macro_tiles) << kMacroTileHeight2DLog2) |
            ((micro_y_pages_begin & page_micro_y_mask)
             << page_micro_y_step_log2);
        EmitBoxIntersection(box_callback, intersect_box, box);
        // Include the rectangle in the same page with Y[4], that goes to the
        // bank selection bit below the page bits, flipped.
        box.offset[1] ^= uint32_t(1) << 4;
        EmitBoxIntersection(box_callback, intersect_box, box);
        pages_begin = micro_y_pages_end;
      };

  if (pages_begin & page_micro_y_mask) {
    // Y rows within the first tile.
    emit_micro_y_rows_and_advance_pages_begin(
        pages_begin,
        std::min((pages_begin + page_micro_y_mask) & ~page_micro_y_mask,
                 pages_end));
  }

  // Macro tile grids.
  // TODO(Triang3l): Support the X upper bound greater than the pitch case,
  // possibly by iterating rows, starting from the one that ends beyond the
  // range beginning, while the beginnings of rows are within the end of the
  // range.
  uint32_t macro_tiles_begin = page_macro_tile_index(
      (pages_begin + page_micro_y_mask) & ~page_micro_y_mask);
  const uint32_t macro_tiles_end = page_macro_tile_index(pages_end);
  if (macro_tiles_begin < macro_tiles_end) {
    const uint32_t macro_rows_end = macro_tiles_end / pitch_macro_tiles;
    while (macro_tiles_begin < macro_tiles_end) {
      Box box;
      box.offset[0] = (macro_tiles_begin % pitch_macro_tiles)
                      << kMacroTileWidthLog2;
      box.offset[1] = (macro_tiles_begin / pitch_macro_tiles)
                      << kMacroTileHeight2DLog2;
      const uint32_t macro_rows_begin =
          (macro_tiles_begin + (pitch_macro_tiles - 1)) / pitch_macro_tiles;
      if (macro_rows_begin < macro_rows_end &&
          macro_rows_begin * pitch_macro_tiles == macro_tiles_begin) {
        // Full rows of macro tiles.
        box.extent[0] = pitch_macro_tiles;
        box.extent[1] = macro_rows_end - macro_rows_begin;
      } else {
        // Partial row of macro tiles.
        box.extent[0] =
            std::min(macro_rows_begin * pitch_macro_tiles, macro_tiles_end) -
            macro_tiles_begin;
      }
      macro_tiles_begin += box.extent[1] * pitch_macro_tiles + box.extent[0];
      box.extent[0] <<= kMacroTileWidthLog2;
      box.extent[1] <<= kMacroTileHeight2DLog2;
      EmitBoxIntersection(box_callback, intersect_box, box);
    }
    pages_begin = macro_tiles_end << bytes_per_element_log2 >>
                  (kPageBytesLog2 - macro_tile_elements_log2);
  }

  if (pages_end & page_micro_y_mask) {
    // Y rows within the last tile.
    emit_micro_y_rows_and_advance_pages_begin(
        std::max(pages_end & ~page_micro_y_mask, pages_begin), pages_end);
  }
}

void PageBoundingVolumeTiled3D(
    const std::function<void(const Box&)> box_callback, uint32_t pages_begin,
    const uint32_t pages_end, const Box& intersect_box,
    const unsigned int bytes_per_element_log2, uint32_t pitch_aligned,
    const uint32_t height_aligned) {
#ifndef NDEBUG
  // Don't allow ranges that may cause an integer overflow, especially when
  // addressing macro tiles, which may be smaller than pages depending on the
  // number of bytes per element. 4 is the log2 of the maximum number of bytes
  // per element.
  constexpr uint32_t max_subresource_page_count =
      uint32_t(1) << (4 + kStoragePitchBitCount + kMax3DWidthHeightPixelsLog2 +
                      kMax3DDepthPixelsLog2 - kPageBytesLog2);
  assert_true(pages_begin <= max_subresource_page_count);
  assert_true(pages_end <= max_subresource_page_count);
#endif

  if (!pitch_aligned) {
    // Pitch smaller than width is unsupported by the bounding volume
    // calculation, however it can be specified in the register anyway, prevent
    // division by zero. Specify the maximum width as the pitch, which would
    // also be correct for a small texture consisting of only 1 row of macro
    // tiles, for which the pitch doesn't matter.
    pitch_aligned = kMax3DWidthHeightPixels;
  }
  assert_zero(pitch_aligned & (kStoragePitchHeightAlignmentElements - 1));
  const uint32_t pitch_macro_tiles = pitch_aligned >> kMacroTileWidthLog2;

  assert_not_zero(height_aligned);
  assert_zero(height_aligned & (kStoragePitchHeightAlignmentElements - 1));
  const uint32_t height_macro_tiles = height_aligned >> kMacroTileHeight3DLog2;

  constexpr unsigned int macro_tile_elements_log2 =
      kMacroTileWidthLog2 + kMacroTileHeight3DLog2 + kMacroTileDepthLog2;

  const auto page_macro_tile_index =
      [macro_tile_elements_log2,
       bytes_per_element_log2](const uint32_t page) -> uint32_t {
    return page << (kPageBytesLog2 - macro_tile_elements_log2) >>
           bytes_per_element_log2;
  };

  const auto macro_tile_origin =
      [pitch_macro_tiles, height_macro_tiles](
          const uint32_t macro_tile) -> std::array<uint32_t, 3> {
    const uint32_t x = macro_tile % pitch_macro_tiles;
    const uint32_t row = macro_tile / pitch_macro_tiles;
    const uint32_t y = row % height_macro_tiles;
    const uint32_t slice = row / height_macro_tiles;
    return {x << kMacroTileWidthLog2, y << kMacroTileHeight3DLog2,
            slice << kMacroTileDepthLog2};
  };

  const unsigned int page_micro_bit_count =
      std::max(int(bytes_per_element_log2) -
                   int(kPageBytesLog2 - macro_tile_elements_log2),
               0);

  const unsigned int page_micro_z_bit_count =
      std::min(page_micro_bit_count, 2u);
  const unsigned int page_micro_z_step_log2 = 2 - page_micro_z_bit_count;

  const unsigned int page_micro_y_bit_count =
      page_micro_bit_count - page_micro_z_bit_count;
  const unsigned int page_micro_y_step_log2 = 3 - page_micro_y_bit_count;

  const uint32_t page_micro_z_mask =
      ((uint32_t(1) << page_micro_z_bit_count) - 1) << page_micro_y_bit_count;
  const uint32_t page_micro_y_mask =
      (uint32_t(1) << page_micro_y_bit_count) - 1;
  const uint32_t page_micro_mask = page_micro_z_mask | page_micro_y_mask;

  // `pages_begin` must be moved forward by the number of pages contained within
  // the boxes that will be added for the front macro tile, so if the first
  // macro tile is also the last one, the same regions within it won't be added
  // twice.
  //
  // For instance, with 16 bytes per element (page[2:1] = z[1:0], page[0] =
  // y[2]), for `pages_begin = 1, pages_end = 7`, the current implementation
  // will produce:
  // - Front: Slice 0 rows [4, 8) and [12, 16).
  // - Front: Slices [1, 3).
  // -  Back: Slice 3 rows [0, 4) and [8, 12).
  //
  // The following behavior is also acceptable:
  // - Front: Slice 0 rows [4, 8) and [12, 16).
  // -  Back: Slices [1, 3).
  // -  Back: Slice 3 rows [0, 4) and [8, 12).
  //
  // And so is this, but suboptimal:
  // - Front: Slice 0 rows [4, 8) and [12, 16).
  // - Front: Slices [1, 2).
  // -  Back: Slices [2, 3).
  // -  Back: Slice 3 rows [0, 4) and [8, 12).
  //
  // However, the following situation must be avoided:
  // - Front: Slice 0 rows [4, 8) and [12, 16).
  // - Front: Slices [1, 3).
  // -  Back: Slices [1, 3).
  // -  Back: Slice 3 rows [0, 4) and [8, 12).

  const auto emit_micro_y_rows_and_advance_pages_begin =
      [&](const std::array<uint32_t, 3>& micro_pages_macro_origin,
          const uint32_t micro_y_pages_begin,
          const uint32_t micro_y_pages_end) {
        if (micro_y_pages_begin >= micro_y_pages_end) {
          return;
        }
        Box box({kMacroTileWidth,
                 (micro_y_pages_end - micro_y_pages_begin)
                     << page_micro_y_step_log2,
                 1},
                micro_pages_macro_origin);
        box.offset[1] |= (micro_y_pages_begin & page_micro_y_mask)
                         << page_micro_y_step_log2;
        box.offset[2] |= (micro_y_pages_begin & page_micro_z_mask) >>
                         page_micro_y_bit_count << page_micro_z_step_log2;
        EmitBoxIntersection(box_callback, intersect_box, box);
        // Include the rectangle in the same page with Y[3], that goes to the
        // bank selection bit below the page bits, flipped.
        box.offset[1] ^= uint32_t(1) << 3;
        EmitBoxIntersection(box_callback, intersect_box, box);
        pages_begin = micro_y_pages_end;
      };

  const auto emit_micro_z_slices_and_advance_pages_begin =
      [&](const std::array<uint32_t, 3>& micro_pages_macro_origin,
          const uint32_t micro_z_pages_begin,
          const uint32_t micro_z_pages_end) {
        if (micro_z_pages_begin >= micro_z_pages_end) {
          return;
        }
        assert_zero(micro_z_pages_begin & page_micro_y_mask);
        assert_zero(micro_z_pages_end & page_micro_y_mask);
        Box box({kMacroTileWidth, kMacroTileHeight3D,
                 (micro_z_pages_end - micro_z_pages_begin) >>
                     page_micro_y_bit_count << page_micro_z_step_log2},
                micro_pages_macro_origin);
        box.offset[2] |= (micro_z_pages_begin & page_micro_z_mask) >>
                         page_micro_y_bit_count << page_micro_z_step_log2;
        EmitBoxIntersection(box_callback, intersect_box, box);
        pages_begin = micro_z_pages_end;
      };

  if (pages_begin & page_micro_mask) {
    const std::array<uint32_t, 3> micro_pages_macro_origin =
        macro_tile_origin(page_macro_tile_index(pages_begin));
    // Y rows of the first Z slice within the first tile.
    emit_micro_y_rows_and_advance_pages_begin(
        micro_pages_macro_origin, pages_begin,
        std::min((pages_begin + page_micro_y_mask) & ~page_micro_y_mask,
                 pages_end));
    // Z slices within the first tile.
    emit_micro_z_slices_and_advance_pages_begin(
        micro_pages_macro_origin,
        (pages_begin + page_micro_y_mask) & ~page_micro_y_mask,
        std::min(pages_begin + page_micro_z_mask, pages_end) &
            ~page_micro_mask);
  }

  // Macro tile grids.
  // TODO(Triang3l): Support the X upper bound greater than the pitch case,
  // possibly by iterating rows, starting from the one that ends beyond the
  // range beginning, while the beginnings of rows are within the end of the
  // range.
  uint32_t macro_tiles_begin =
      page_macro_tile_index((pages_begin + page_micro_mask) & ~page_micro_mask);
  const uint32_t macro_tiles_end = page_macro_tile_index(pages_end);
  if (macro_tiles_begin < macro_tiles_end) {
    const uint32_t macro_rows_end = macro_tiles_end / pitch_macro_tiles;
    const uint32_t macro_slices_end = macro_rows_end / height_macro_tiles;
    while (macro_tiles_begin < macro_tiles_end) {
      Box box;
      box.offset = macro_tile_origin(macro_tiles_begin);
      const uint32_t macro_rows_begin =
          (macro_tiles_begin + (pitch_macro_tiles - 1)) / pitch_macro_tiles;
      if (macro_rows_begin < macro_rows_end &&
          macro_rows_begin * pitch_macro_tiles == macro_tiles_begin) {
        box.extent[0] = pitch_macro_tiles;
        const uint32_t macro_slices_begin =
            (macro_rows_begin + (height_macro_tiles - 1)) / height_macro_tiles;
        if (macro_slices_begin < macro_slices_end &&
            macro_slices_begin * height_macro_tiles == macro_rows_begin) {
          // Full slices of macro tiles.
          box.extent[1] = height_macro_tiles;
          box.extent[2] = macro_slices_end - macro_slices_begin;
        } else {
          // Partial slice of full rows of macro tiles.
          box.extent[1] = std::min(macro_slices_begin * height_macro_tiles,
                                   macro_rows_end) -
                          macro_rows_begin;
        }
      } else {
        // Partial row of macro tiles.
        box.extent[0] =
            std::min(macro_rows_begin * pitch_macro_tiles, macro_tiles_end) -
            macro_tiles_begin;
      }
      macro_tiles_begin +=
          ((box.extent[2] * height_macro_tiles) + box.extent[1]) *
              pitch_macro_tiles +
          box.extent[0];
      box.extent[0] <<= kMacroTileWidthLog2;
      box.extent[1] <<= kMacroTileHeight3DLog2;
      box.extent[2] <<= kMacroTileDepthLog2;
      EmitBoxIntersection(box_callback, intersect_box, box);
    }
    pages_begin = macro_tiles_end << bytes_per_element_log2 >>
                  (kPageBytesLog2 - macro_tile_elements_log2);
  }

  if (pages_end & page_micro_mask) {
    const std::array<uint32_t, 3> micro_pages_macro_origin =
        macro_tile_origin(page_macro_tile_index(pages_end));
    // Z slices within the last tile.
    emit_micro_z_slices_and_advance_pages_begin(
        micro_pages_macro_origin,
        (pages_begin + page_micro_z_mask) & ~page_micro_mask,
        pages_end & ~page_micro_y_mask);
    // Y rows of the first Z slice within the last tile.
    emit_micro_y_rows_and_advance_pages_begin(
        micro_pages_macro_origin,
        std::max(pages_end & ~page_micro_y_mask, pages_begin), pages_end);
  }
}

}  // namespace texture_address
}  // namespace gpu
}  // namespace xe
