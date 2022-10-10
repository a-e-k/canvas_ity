<img src="icon.png" width="128" height="128" align="left">

# canvas_ity

[![Tests](../../../actions/workflows/tests.yml/badge.svg)](../../../actions/workflows/tests.yml)

This is a tiny, [single-header C++ library](../src/canvas_ity.hpp)
for rasterizing immediate-mode 2D vector graphics, closely
modeled on the basic [W3C (not WHATWG) HTML5 2D canvas
specification](https://www.w3.org/TR/2015/REC-2dcontext-20151119/).

The priorities for this library are high-quality rendering, ease of use, and
compact size.  Speed is important too, but secondary to the other priorities.
Notably, this library takes an opinionated approach and does not provide
options for trading off quality for speed.

Despite its small size, it supports nearly everything listed in the W3C
HTML5 2D canvas specification, except for hit regions and getting certain
properties.  The main differences lie in the surface-level API to make this
easier for C++ use, while the underlying implementation is carefully based
on the specification.  In particular, stroke, fill, gradient, pattern,
image, and font styles are specified slightly differently (avoiding strings
and auxiliary classes).  Nonetheless, the goal is that this library could
produce a conforming HTML5 2D canvas implementation if wrapped in a thin
layer of JavaScript bindings.  See the accompanying [C++ automated test
suite](../test/test.cpp) and its [HTML5 port](../test/test.html) for a mapping
between the APIs and a comparison of this library's rendering output against
browser canvas implementations.

## :memo: Example

The following complete example program writes out a TGA image file and
demonstrates path building, fills, strokes, line dash patterns, line joins,
line caps, linear gradients, drop shadows, and compositing operations.  See
the HTML5 equivalent of the example on the right (scroll the code horizontally
if needed) and compare them line-by-line.  Note that the minor differences
in shading are due to the library's use of gamma-correct blending whereas
browsers typically ignore this.

<table>
<tr>
<th>canvas_ity</th>
<th>HTML5</th>
</tr>
<tr>
<td><img src="example_canvas_ity.png" width="256" height="256"></td>
<td><img src="example_html5.png" width="256" height="256"></td>
</tr>
<tr>
<td>
<sub>

```c++
#include <algorithm>
#include <fstream>
// Include the library header and implementation.
#define CANVAS_ITY_IMPLEMENTATION
#include "canvas_ity.hpp"
int main()
{
    // Construct the canvas.
    static int const width = 256, height = 256;
    canvas_ity::canvas context( width, height );

    // Build a star path.
    context.move_to( 128.0f,  28.0f ); context.line_to( 157.0f,  87.0f );
    context.line_to( 223.0f,  97.0f ); context.line_to( 175.0f, 143.0f );
    context.line_to( 186.0f, 208.0f ); context.line_to( 128.0f, 178.0f );
    context.line_to(  69.0f, 208.0f ); context.line_to(  80.0f, 143.0f );
    context.line_to(  32.0f,  97.0f ); context.line_to(  98.0f,  87.0f );
    context.close_path();

    // Set up the drop shadow.
    context.set_shadow_blur( 8.0f );
    context.shadow_offset_y = 4.0f;
    context.set_shadow_color( 0.0f, 0.0f, 0.0f, 0.5f );

    // Fill the star with yellow.
    context.set_color( canvas_ity::fill_style, 1.0f, 0.9f, 0.2f, 1.0f );
    context.fill();

    // Draw the star with a thick red stroke and rounded points.
    context.line_join = canvas_ity::rounded;
    context.set_line_width( 12.0f );
    context.set_color( canvas_ity::stroke_style, 0.9f, 0.0f, 0.5f, 1.0f );
    context.stroke();

    // Draw the star again with a dashed thinner orange stroke.
    float segments[] = { 21.0f, 9.0f, 1.0f, 9.0f, 7.0f, 9.0f, 1.0f, 9.0f };
    context.set_line_dash( segments, 8 );
    context.line_dash_offset = 10.0f;
    context.line_cap = canvas_ity::circle;
    context.set_line_width( 6.0f );
    context.set_color( canvas_ity::stroke_style, 0.95f, 0.65f, 0.15f, 1.0f );
    context.stroke();

    // Turn off the drop shadow.
    context.set_shadow_color( 0.0f, 0.0f, 0.0f, 0.0f );

    // Add a shine layer over the star.
    context.set_linear_gradient( canvas_ity::fill_style, 64.0f, 0.0f, 192.0f, 256.0f );
    context.add_color_stop( canvas_ity::fill_style, 0.30f, 1.0f, 1.0f, 1.0f, 0.0f );
    context.add_color_stop( canvas_ity::fill_style, 0.35f, 1.0f, 1.0f, 1.0f, 0.8f );
    context.add_color_stop( canvas_ity::fill_style, 0.45f, 1.0f, 1.0f, 1.0f, 0.8f );
    context.add_color_stop( canvas_ity::fill_style, 0.50f, 1.0f, 1.0f, 1.0f, 0.0f );

    context.global_composite_operation = canvas_ity::source_atop;
    context.fill_rectangle( 0.0f, 0.0f, 256.0f, 256.0f );

    // Fetch the rendered RGBA pixels from the entire canvas.
    unsigned char *image = new unsigned char[ height * width * 4 ];
    context.get_image_data( image, width, height, width * 4, 0, 0 );
    // Write them out to a TGA image file (TGA uses BGRA order).
    unsigned char header[] = { 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        width & 255, width >> 8, height & 255, height >> 8, 32, 40 };
    for ( int pixel = 0; pixel < height * width; ++pixel )
        std::swap( image[ pixel * 4 + 0 ], image[ pixel * 4 + 2 ] );
    std::ofstream stream( "example.tga", std::ios::binary );
    stream.write( reinterpret_cast< char * >( header ), sizeof( header ) );
    stream.write( reinterpret_cast< char * >( image ), height * width * 4 );
    delete[] image;
}
```

</sub>
</td>
<td>
<sub>

```html
<!DOCTYPE html>
<html>
<head>
  <title>Example</title>
</head>
<body>
  <canvas id="example" width="256" height="256"></canvas>
  <script type="text/javascript">
    const context = document.getElementById( "example" ).getContext( "2d" );


    // Build a star path.
    context.moveTo( 128.0,  28.0 ); context.lineTo( 157.0,  87.0 );
    context.lineTo( 223.0,  97.0 ); context.lineTo( 175.0, 143.0 );
    context.lineTo( 186.0, 208.0 ); context.lineTo( 128.0, 178.0 );
    context.lineTo(  69.0, 208.0 ); context.lineTo(  80.0, 143.0 );
    context.lineTo(  32.0,  97.0 ); context.lineTo(  98.0,  87.0 );
    context.closePath();

    // Set up the drop shadow.
    context.shadowBlur = 8.0;
    context.shadowOffsetY = 4.0;
    context.shadowColor = "rgba(0,0,0,0.5)";

    // Fill the star with yellow.
    context.fillStyle = "#ffe633";
    context.fill();

    // Draw the star with a thick red stroke and rounded points.
    context.lineJoin = "round";
    context.lineWidth = 12.0;
    context.strokeStyle = "#e60080";
    context.stroke();

    // Draw the star again with a dashed thinner orange stroke.
    const segments = [ 21.0, 9.0, 1.0, 9.0, 7.0, 9.0, 1.0, 9.0 ];
    context.setLineDash( segments );
    context.lineDashOffset = 10.0;
    context.lineCap = "round";
    context.lineWidth = 6.0;
    context.strokeStyle = "#f2a626";
    context.stroke();

    // Turn off the drop shadow.
    context.shadowColor = "rgba(0,0,0,0.0)";

    // Add a shine layer over the star.
    let gradient = context.createLinearGradient( 64.0, 0.0, 192.0, 256.0 );
    gradient.addColorStop( 0.30, "rgba(255,255,255,0.0)" );
    gradient.addColorStop( 0.35, "rgba(255,255,255,0.8)" );
    gradient.addColorStop( 0.45, "rgba(255,255,255,0.8)" );
    gradient.addColorStop( 0.50, "rgba(255,255,255,0.0)" );
    context.fillStyle = gradient;
    context.globalCompositeOperation = "source-atop";
    context.fillRect( 0.0, 0.0, 256.0, 256.0 );

  </script>
</body>
</html>












```

</sub>
</td>
</tr>
</table>

## :sparkles: Features

### High-quality rendering

- Trapezoidal area antialiasing provides very smooth antialiasing, even when
    lines are nearly horizontal or vertical.
- Gamma-correct blending, interpolation, and resampling are used throughout.
    It linearizes all colors and premultiplies alpha on input and converts
    back to unpremultiplied sRGB on output.  This reduces muddiness on many
    gradients (e.g., red to green), makes line thicknesses more perceptually
    uniform, and avoids dark fringes when interpolating opacity.
- Bicubic convolution resampling is used whenever it needs to resample a
    pattern or image.  This smoothly interpolates with less blockiness when
    magnifying, and antialiases well when minifying.  It can simultaneously
    magnify and minify along different axes.
- Ordered dithering is used on output.  This reduces banding on subtle
    gradients while still being compression-friendly.
- High curvature is handled carefully in line joins.  Thick lines are drawn
    correctly as though tracing with a wide pen nib, even where the lines
    curve sharply.  (Simpler curve offsetting approaches tend to show
    bite-like artifacts in these cases.)

### Ease of use

- Provided as a single-header library with no dependencies beside the standard
    C++ library.  There is nothing to link, and it even includes built-in
    binary parsing for TrueType font (TTF) files.  It is pure CPU code and
    does not require a GPU or GPU context.
- Has extensive Doxygen-style documentation comments for the public API.
- Compiles cleanly at moderately high warning levels on most compilers.
- Shares no internal pointers, nor holds any external pointers.  Newcomers to
    C++ can have fun drawing with this library without worrying so much about
    resource lifetimes or mutability.
- Uses no static or global variables.  Threads may safely work with different
    canvas instances concurrently without locking.
- Allocates no dynamic memory after reaching the high-water mark.  Except for
    the pixel buffer, flat `std::vector` instances embedded in the canvas
    instance handle all dynamic memory.  This reduces fragmentation and
    makes it easy to change the code to reserve memory up front or even to
    use statically allocated vectors.
- Works with exceptions and RTTI disabled.
- Intentionally uses a plain C++03 style to make it as widely portable as
    possible, easier to understand, and (with indexing preferred over pointer
    arithmetic) easier to port natively to other languages.  The accompanying
    test suite may also help with porting.

### Compact size

- The source code for the entire library consists of a bit over 2300 lines
    (not counting comments or blanks), each no longer than 78 columns.
    Alternately measured, it has fewer than 1300 semicolons.
- The object code for the library can be less than 36 KiB on x86-64 with
    appropriate compiler settings for size.
- Due to the library's small size, the accompanying automated test suite
    achieves 100% line coverage of the library in gcov and llvm-cov.

## :warning: Limitations

- Trapezoidal antialiasing overestimates coverage where paths self-intersect
    within a single pixel.  Where inner joins are visible, this can lead to
    a "grittier" appearance due to the extra windings used.
- Clipping uses an antialiased sparse pixel mask rather than geometrically
    intersecting paths.  Therefore, it is not subpixel-accurate.
- Text rendering is extremely basic and mainly for convenience.  It only
    supports left-to-right text, and does not do any hinting, kerning,
    ligatures, text shaping, or text layout.  If you require any of those,
    consider using another library to provide those and feed the results to
    this library as either placed glyphs or raw paths.
- _TRUETYPE FONT PARSING IS NOT SECURE!_ It does some basic validity checking,
    but should only be used with known-good or sanitized fonts.
- Parameter checking does not test for non-finite floating-point values.
- Rendering is single-threaded, not explicitly vectorized, and not GPU-
    accelerated.  It also copies data to avoid ownership issues.  If you
    need the speed, you are better off using a more fully-featured library.
- The library does no input or output on its own.  Instead, you must provide
    it with buffers to copy into or out of.

## :computer: Usage

This is a [single-header library](../src/canvas_ity.hpp).  You may freely
include it in any of your source files to declare the `canvas_ity` namespace
and its members.  However, to get the implementation, you must
```c++
#define CANVAS_ITY_IMPLEMENTATION
```
in exactly one C++ file before including this header.

Then, construct an instance of the `canvas_ity::canvas` class with the pixel
dimensions that you want and draw into it using any of the various drawing
functions.  You can then use the `get_image_data()` function to retrieve the
currently drawn image at any time.

See each of the public member function and data member (i.e., method and
field) declarations for the full API documentation.  Also see the accompanying
[C++ automated test suite](../test/test.cpp) for examples of the usage of each
public member, and the [test suite's HTML5 port](../test/test.html) for how
these map to the HTML5 canvas API.

To build the test program, either just compile the one source file directly
to an executable with a C++ compiler, e.g.:
```
g++ -O3 -o test test.cpp
```
or else use the accompanying [CMake file](../test/CMakeLists.txt).  The CMake
file enables extensive warnings and also offers targets for static analysis,
dynamic analysis, measuring code size, and measuring test coverage.

By default, the test harness simply runs each test once and reports the
results.  However, with command line arguments, it can write PNG images of
the test results, run tests repeatedly to benchmark them, run just a subset
of the test, or write out a new table of expected image hashes.  Run the
program with `--help` to see the usage guide for more on these.

## :copyright: License

This software is distributed as open source under the terms of the permissive
[ISC license](https://choosealicense.com/licenses/isc/).

## :mailbox: Contributing

_Please do not send pull requests!_ They will be politely declined at
this time.  This library is open source, but not currently open to outside
code contributions.  It is also considered largely feature-complete.
(Moreover, this GitHub repository is only a mirror for publishing releases
from the author's local Mercurial repository.)

Bug reports, discussions, kudos, and notices of nifty projects built using
this library are most welcome, however.
