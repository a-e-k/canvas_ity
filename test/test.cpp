// canvas_ity test suite and harness v1.00 -- ISC license
// Copyright (c) 2022 Andrew Kensler
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

// ======== ABOUT ========
//
// This program contains both the test suite and a standalone test harness
// for automated testing of the canvas_ity library.  The harness calls each
// test with a fresh instance of a canvas.  The test then exercises the public
// interface of the canvas.  After it returns, the harness fetches the image
// of the canvas and hashes the contents to compare against an expected hash
// to determine if the test passed or fails.
//
// To build the test program, either just compile the one source file
// directly to an executable with a C++ compiler, e.g.:
//     g++ -O3 -o test test.cpp
// or else use the accompanying CMake file.  The CMake file enables extensive
// warnings and also offers targets for static analysis, dynamic analysis,
// measuring code size, and measuring test coverage.
//
// By default, the test harness simply runs each test once and reports the
// results.  However, with command line arguments, it can write PNG images
// of the test results, run tests repeatedly to benchmark them, run just a
// subset of the test, or write out a new table of expected image hashes.
// Run the program with --help to see the usage guide for more on these.
//
// Beware that while the hash checks are tuned to allow tests to pass even
// with minor numerical differences due to aggressive compiler optimizations
// (e.g., -Ofast on certain architectures), some tests may still report as
// failing.  This does not necessarily mean that there is a problem, but it
// does warrant manual verification of the failing test's image against a
// passing baseline test image produced with optimizations disabled.
//
// Also see test.html, the HTML5 2D canvas port of these tests.  Compare
// the code for the C++ and JavaScript tests line-by-line to see how this
// library's API maps to the HTML5 API and vice-versa.  Compare the images
// produced by each (run with --pngs to get the images) to see how the
// library's rendering relates to browser canvas implementations.

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS
#endif

#define CANVAS_ITY_IMPLEMENTATION
#include "../src/canvas_ity.hpp"

#if defined( __linux__ )
#include <time.h>
#include <unistd.h>
#elif defined( _WIN32 )
#include <windows.h>
#elif defined( __MACH__ )
#include <mach/mach_time.h>
#include <unistd.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace canvas_ity;
using namespace std;

// ======== RESOURCES ========
//
// The resources embedded here are mainly font files in TTF form.  While
// the data could be stored directly as an array of integer literals, the
// Base64 encoding is much more compact in terms of source.  It also means
// that the HTML5 port of these tests can use these resources almost as-is.
//
// The valid fonts all have the following properties in common:
//
// - The asterisk is a composite of the acute mark with a mix of simple and
//     complex (i.e., 2x2) scaling transforms used to rotate them to the
//     eight principal directions at 45-degree angles.
// - A glyph assigned to the high end of the private use area (at 10FFFD)
//     tests all combinations of four on-curve or off-curve points.
// - Characters 'D' through 'H' are copies of a simple dot and assigned to
//     the last glyph indices; being copies and at the end means that they can
//     test the hmtx table having fewer horizontal metrics than glyphs, with
//     the last advance width being replicated to all the glyphs beyond it.
// - Having assignments to 'C' through 'I', but in non-consecutive order also
//     allows for testing the range table in the format-4 cmap subtable.
// - The 'a' glyph has a sequence of off-curve points going in the same
//     direction so that the points have a consecutive sequence of identical
//     flags that are compacted with a repeat flag.
// - The 's' glyph is translated 1024 font units to the right.  However, it
//     has a normal advance width and left side bearing so it should render
//     like a normal character despite this.
// - The .notdef glyph has a couple of hinting instructions that just push a
//     few values on the stack but do nothing else.  These instructions must
//     be skipped over to get to the point data in the glyph.

// Valid TTF file, cmap table has types 12 and 4 subtables.
vector< unsigned char > font_a;
char const font_a_base64[] =
    "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"
    "FxY3FhUUBwYnJgcCFxYXFjcGJyYTEjU0aCIGBBxcQhUKIAMIVD+VMjKMTk8BCAgJoVVJOc3z"
    "ERQDLgUXEBZKQhUECyBQAgEHCi41AwcBAVH+u4mnAQEnlAQFAQEBNKpSAAH/+f+6A7QDjAAe"
    "AAAlJgEmJwUyFRQHBhUUEzYTNicmJzQ3NjcGBwAHBgciAbYX/tMRaAFkHh494U93Bz4sASik"
    "hV8Y/uEJDR4kDoACfSRdAhYSCxZAJv4/LwHaGRIMGhABAgU9Rv1/VHkBAAH/+f5tA7QDjAAm"
    "AAAlNAEmJwUyFRQHBhUUEzYTNicmJzQ3NjcGBwIHAgcGIyY1Njc2NzYBqv7IEWgBZB4ePeFF"
    "gQc+LAEopIVfGOJGngEYOFgBWSAGWixJApYkXQIWEgsWQCb+PygB4RoRDBoQAQIFPUb927D+"
    "cQM1AVAZGgkNy///ABT/+gLXB/MQZwAMABEC1T/4QAASBgAEAAAAAQEFAyMCxgUeAA0AAAE2"
    "EzY3NhcWBwYHBicmARAwqBoOWkoSHsKSFBwfA0prASUtAxQYBSf+ohcHBwAAEAAA/nAHkAYA"
    "AAMABwALAA8AEwAXABsAHwAjACcAKwAvADMANwA7AD8AABAQIBAAECARABAhEAAQIRESESAQ"
    "ABEgEQARIRAAESERExAgEAEQIBEBECEQARAhERMRIBABESARAREhEAERIREBkP5wAZD+cAGQ"
    "/nABkHABkP5wAZD+cAGQ/nABkHABkP5wAZD+cAGQ/nABkHABkP5wAZD+cAGQ/nABkP5wAZD+"
    "cAIAAZD+cAIAAZD+cAIAAZD+cPoAAZD+cAIAAZD+cAIAAZD+cAIAAZD+cPoAAZD+cAIAAZD+"
    "cAIAAZD+cAIAAZD+cPoAAZD+cAIAAZD+cAIAAZD+cAIAAZD+cAD//wBkADIDIAWqECcAEgAA"
    "/qIABAASAAP//wBkAZADIARMEAYAEgAA//8AZAGQAyAETBAGABIAAP//AGQBkAMgBEwQBgAS"
    "AAAAAQBkAZADIARMAAMAABIgECBkArz9RARM/UQAAAAAAAAMAJYAAQAAAAAAAQAFAAAAAQAA"
    "AAAAAgAHAAUAAQAAAAAAAwAFAAAAAQAAAAAABAAFAAAAAQAAAAAABQALAAwAAQAAAAAABgAF"
    "AAAAAwABBAkAAQAKABcAAwABBAkAAgAOACEAAwABBAkAAwAKABcAAwABBAkABAAKABcAAwAB"
    "BAkABQAWAC8AAwABBAkABgAKABdGb250QVJlZ3VsYXJWZXJzaW9uIDEuMABGAG8AbgB0AEEA"
    "UgBlAGcAdQBsAGEAcgBWAGUAcgBzAGkAbwBuACAAMQAuADAAAAMAAAAAAAD/ZgBmAAAAAAAA"
    "AAAAAAAAAAAAAAAAAAA=";

// Valid TTF file, cmap table has type 4 subtable only and loca table is long.
vector< unsigned char > font_b;
char const font_b_base64[] =
    "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"
    "PeFPdwc+LAEopIVfGP7hCQ0eJA6AAn0kXQIWEgsWQCb+Py8B2hkSDBoQAQIFPUb9f1R5AQAB"
    "//n+bQO0A4wAJgAAJTQBJicFMhUUBwYVFBM2EzYnJic0NzY3BgcCBwIHBiMmNTY3Njc2Aar+"
    "yBFoAWQeHj3hRYEHPiwBKKSFXxjiRp4BGDhYAVkgBlosSQKWJF0CFhILFkAm/j8oAeEaEQwa"
    "EAECBT1G/duw/nEDNQFQGRoJDcv//wAU//oC1wfzEGcADAARAtU/+EAAEgYABAAAAAEBBQMj"
    "AsYFHgANAAABNhM2NzYXFgcGBwYnJgEQMKgaDlpKEh7CkhQcHwNKawElLQMUGAUn/qIXBwcA"
    "ABAAAP5wB5AGAAADAAcACwAPABMAFwAbAB8AIwAnACsALwAzADcAOwA/AAAQECAQABAgEQAQ"
    "IRAAECEREhEgEAARIBEAESEQABEhERMQIBABECARARAhEAEQIRETESAQAREgEQERIRABESER"
    "AZD+cAGQ/nABkP5wAZBwAZD+cAGQ/nABkP5wAZBwAZD+cAGQ/nABkP5wAZBwAZD+cAGQ/nAB"
    "kP5wAZD+cAGQ/nACAAGQ/nACAAGQ/nACAAGQ/nD6AAGQ/nACAAGQ/nACAAGQ/nACAAGQ/nD6"
    "AAGQ/nACAAGQ/nACAAGQ/nACAAGQ/nD6AAGQ/nACAAGQ/nACAAGQ/nACAAGQ/nAA//8AZAAy"
    "AyAFqhAnABIAAP6iAAQAEgAD//8AZAGQAyAETBAGABIAAP//AGQBkAMgBEwQBgASAAD//wBk"
    "AZADIARMEAYAEgAAAAEAZAGQAyAETAADAAASIBAgZAK8/UQETP1EAAAAAAAADACWAAEAAAAA"
    "AAEABQAAAAEAAAAAAAIABwAFAAEAAAAAAAMABQAAAAEAAAAAAAQABQAAAAEAAAAAAAUACwAM"
    "AAEAAAAAAAYABQAAAAMAAQQJAAEACgAXAAMAAQQJAAIADgAhAAMAAQQJAAMACgAXAAMAAQQJ"
    "AAQACgAXAAMAAQQJAAUAFgAvAAMAAQQJAAYACgAXRm9udEJSZWd1bGFyVmVyc2lvbiAxLjAA"
    "RgBvAG4AdABCAFIAZQBnAHUAbABhAHIAVgBlAHIAcwBpAG8AbgAgADEALgAwAAADAAAAAAAA"
    "/2YAZgAAAAAAAAAAAAAAAAAAAAAAAAAA";

// Valid TTF file, cmap table has type 0 subtable only.
vector< unsigned char > font_c;
char const font_c_base64[] =
    "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"
    "AAAAAAAAAAAAAAAAAAAAAAAAAAA=";

// Invalid TTF file, valid magic number but only one byte after that.
vector< unsigned char > font_d;
char const font_d_base64[] =
    "AAEAAAA=";

// Invalid TTF file, offset table cut short.
vector< unsigned char > font_e;
char const font_e_base64[] =
    "AAEAAAALAIAAAwAwT1MvMmisck8AAAE4AAAAYGNtYXAXewGCAAAB3AAAAUJjdnQgAEQFEQAA"
    "AyAAAAAEZ2x5ZjCUlAIAAANMAAAGhmhlYWQe1bIjAAAAvAAAADZoaGVhDf8FBAAAAPQAAAAk"
    "aG10eDmaBAMAAAGYAAAARGxvY2ERbxMOAAADJAAAAChtYXhwAHUAtwAAARgAAAAgbmFtZVZp"
    "NvsAAAnUAAAA23Bvc3T/aQBmAAAKsAAAAA==";

// Invalid TTF file, offset table is complete but points to missing tables.
vector< unsigned char > font_f;
char const font_f_base64[] =
    "AAEAAAALAIAAAwAwT1MvMmisck8AAAE4AAAAYGNtYXAXewGCAAAB3AAAAUJjdnQgAEQFEQAA"
    "AyAAAAAEZ2x5ZjCUlAIAAANMAAAGhmhlYWQe1bIjAAAAvAAAADZoaGVhDf8FBAAAAPQAAAAk"
    "aG10eDmaBAMAAAGYAAAARGxvY2ERbxMOAAADJAAAAChtYXhwAHUAtwAAARgAAAAgbmFtZVZp"
    "NvsAAAnUAAAA23Bvc3T/aQBmAAAKsAAAACAAAQAAAAEAAEPW4v5fDzz1AB0IAAAAAADcB1gv"
    "AAAAANwUDpf/+f5tB5AH8wAAAAgAAgAAAAAAAA==";

// Invalid TTF file, lacking most required TTF tables except for head.
vector< unsigned char > font_g;
char const font_g_base64[] =
    "AAEAAAABABAAAAAAaGVhZB7h+0cAAAAcAAAANgABAAAAAQAAC4VXZl8PPPUAHQgAAAAAANwH"
    "WC8AAAAA3CBXu//5/m0HkAfzAAAACAACAAAAAAAA";

// ======== TEST SUITE ========
//
// Note that while individual tests only use the same public interface
// as clients normally would, most tests are not good examples of the
// ordinary use of the library.  They tend to make superfluous calls, make
// calls in unusual orders, and assume documented implicit behavior.  They
// typically test strict conformance to the W3C (not WHATWG) HTML5 2D canvas
// specification (https://www.w3.org/TR/2015/REC-2dcontext-20151119/).
//
// For better examples of normal use of the library, see the tests prefixed
// with "example_".  These are written in more orthodox ways and intended to
// demonstrate interesting things that the library can draw.
//
// To add a new test to the suite, write a function for it here with the same
// function signature as the other tests, and then register it below in the
// harness's table of tests to run.  (Remember to also port it to test.html!)

namespace
{

void scale_uniform( canvas &that, float width, float height )
{
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    float segments[] = { 1.0f };
    that.set_line_dash( segments, 1 );
    that.line_cap = circle;
    for ( float size = 8.0f; size < min( width, height ); size *= 2.0f )
    {
        that.scale( 2.0f, 2.0f );
        that.stroke_rectangle( 0.0f, 0.0f, 8.0f, 8.0f );
    }
}

void scale_non_uniform( canvas &that, float width, float height )
{
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    float segments[] = { 4.0f };
    that.set_line_dash( segments, 1 );
    that.scale( 4.0f, 0.5f );
    that.stroke_rectangle( width * 0.125f / 4.0f, height * 0.125f / 0.5f,
                           width * 0.75f / 4.0f, height * 0.75f / 0.5f );
}

void rotate( canvas &that, float width, float height )
{
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    for ( int step = 0; step < 64; ++step )
    {
        that.rotate( 3.14159265f / 2.0f / 64.0f );
        that.stroke_rectangle( 0.0f, 0.0f, width, height );
    }
}

void translate( canvas &that, float width, float height )
{
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    for ( float step = 0.0f; step < 32.0f; step += 1.0f )
    {
        that.translate( ( 0.5f - step / 32.0f ) * width * 0.2f,
                        height / 32.0f );
        that.begin_path();
        that.arc( 0.0f, 0.0f, width * 0.125f, 0.0f, 6.28318531f );
        that.close_path();
        that.stroke();
    }
}

void transform( canvas &that, float width, float height )
{
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    for ( int step = 0; step < 8; ++step )
    {
        that.transform( 1.0f, 0.0f, 0.1f, 1.0f, width * -0.05f, 0.0f );
        that.stroke_rectangle( width * 0.25f, height * 0.25f,
                               width * 0.5f, height * 0.5f );
    }
}

void transform_fill( canvas &that, float width, float height )
{
    unsigned char checker[ 1024 ];
    for ( int index = 0; index < 1024; ++index )
        checker[ index ] = static_cast< unsigned char >(
            ( ( index >> 5 & 1 ) ^ ( index >> 9 & 1 ) ^
              ( ( index & 3 ) == 3 ) ) * 255 );
    that.set_pattern( fill_style, checker, 16, 16, 64, repeat );
    that.begin_path();
    that.rectangle( width * 0.2f, height * 0.2f,
                    width * 0.6f, height * 0.6f );
    that.transform( 1.0f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f );
    that.fill();
}

void transform_stroke( canvas &that, float width, float height )
{
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    that.set_line_width( 8.0f );
    float segments[] = { 22.0f, 8.0f, 10.0f, 8.0f };
    that.set_line_dash( segments, 4 );
    that.begin_path();
    that.arc( width * 0.5f, height * 0.5f, min( width, height ) * 0.4f,
              0.0f, 6.28318531f );
    that.close_path();
    that.transform( 1.0f, 1.0f, 0.0f, 2.0f, 0.0f, 0.0f );
    that.stroke();
}

void set_transform( canvas &that, float width, float height )
{
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    for ( int step = 0; step < 8; ++step )
        that.set_transform( 1.0f, 0.0f, 0.1f, 1.0f, width * -0.05f, 0.0f );
    that.stroke_rectangle( width * 0.25f, height * 0.25f,
                           width * 0.5f, height * 0.5f );
}

void global_alpha( canvas &that, float width, float height )
{
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    that.set_line_width( 3.0f );
    for ( float y = 0.0f; y < 6.0f; y += 1.0f )
        for ( float x = 0.0f; x < 6.0f; x += 1.0f )
        {
            that.set_color( fill_style, x / 5.0f, 1.0f, y / 5.0f, x / 5.0f );
            that.set_global_alpha( y / 4.0f - 0.25f );
            that.begin_path();
            that.rectangle(
                ( x + 0.1f ) / 6.0f * width, ( y + 0.1f ) / 6.0f * height,
                0.8f / 6.0f * width, 0.8f / 6.0f * height );
            that.fill();
            that.stroke();
        }
}

void global_composite_operation( canvas &that, float width, float height )
{
    composite_operation const operations[] = {
        source_in, source_copy, source_out, destination_in,
        destination_atop, lighter, destination_over, destination_out,
        source_atop, source_over, exclusive_or };
    float box_width = 0.25f * width;
    float box_height = 0.25f * height;
    for ( int index = 0; index < 11; ++index )
    {
        float column = static_cast< float >( operations[ index ] % 4 );
        float row = static_cast< float >( operations[ index ] / 4 );
        that.save();
        that.begin_path();
        that.rectangle( column * box_width, row * box_height,
                        box_width, box_height );
        that.clip();
        that.set_color( fill_style, 0.0f, 0.0f, 1.0f, 1.0f );
        that.fill_rectangle( ( column + 0.4f ) * box_width,
                             ( row + 0.4f ) * box_height,
                             0.4f * box_width, 0.4f * box_height );
        that.global_composite_operation = operations[ index ];
        that.set_color( fill_style, 1.0f, 0.0f, 0.0f, 1.0f );
        that.fill_rectangle( ( column + 0.2f ) * box_width,
                             ( row + 0.2f ) * box_height,
                             0.4f * box_width, 0.4f * box_height );
        that.restore();
    }
}

void shadow_color( canvas &that, float width, float height )
{
    that.shadow_offset_x = 5.0f;
    that.shadow_offset_y = 5.0f;
    that.set_shadow_blur( 1.0f );
    for ( float row = 0.0f; row < 5.0f; row += 1.0f )
    {
        float y = ( row + 0.25f ) * 0.2f * height;
        that.set_color( fill_style, 0.0f, 0.0f, 0.0f, 0.25f * row );
        that.set_shadow_color( 1.0f, -1.0f, 0.0f, 0.25f );
        that.fill_rectangle( 0.05f * width, y,
                             0.15f * width, 0.1f * height );
        that.set_shadow_color( 0.0f, 1.0f, 0.0f, 0.5f );
        that.fill_rectangle( 0.30f * width, y,
                             0.15f * width, 0.1f * height );
        that.set_shadow_color( 0.0f, 0.0f, 2.0f, 0.75f );
        that.fill_rectangle( 0.55f * width, y,
                             0.15f * width, 0.1f * height );
        that.set_shadow_color( 1.0f, 1.0f, 1.0f, 100.0f );
        that.fill_rectangle( 0.80f * width, y,
                             0.15f * width, 0.1f * height );
    }
}

void shadow_offset( canvas &that, float width, float height )
{
    that.set_shadow_blur( 2.0f );
    that.set_color( fill_style, 1.0f, 1.0f, 1.0f, 1.0f );
    that.set_shadow_color( 0.0f, 0.0f, 0.0f, 1.0f );
    for ( float y = 0.0f; y < 5.0f; y += 1.0f )
        for ( float x = 0.0f; x < 5.0f; x += 1.0f )
        {
            that.shadow_offset_x = ( x - 2.0f ) * 4.0f;
            that.shadow_offset_y = ( y - 2.0f ) * 4.0f;
            that.fill_rectangle( ( x + 0.25f ) * 0.2f * width,
                                 ( y + 0.25f ) * 0.2f * height,
                                 0.1f * width, 0.1f * height );
        }
}

void shadow_offset_offscreen( canvas &that, float width, float height )
{
    that.shadow_offset_x = width;
    that.set_color( fill_style, 1.0f, 0.0f, 0.0f, 1.0f );
    that.set_shadow_color( 0.0f, 0.0f, 0.0f, 0.5f );
    that.fill_rectangle( width * -0.6875f, height * 0.0625f,
                         width * 0.375f, height * 0.375f );
    that.begin_path();
    that.arc( width * 0.5f, height * 0.75f, min( width, height ) * 0.2f,
              0.0f, 6.28318531f );
    that.close_path();
    that.fill();
}

void shadow_blur( canvas &that, float width, float height )
{
    that.set_color( fill_style, 1.0f, 1.0f, 1.0f, 1.0f );
    that.set_shadow_color( 0.0f, 0.0f, 0.0f, 1.0f );
    that.shadow_offset_x = 5.0f;
    that.shadow_offset_y = 5.0f;
    for ( float x = 0.0f; x < 5.0f; x += 1.0f )
        for ( float y = 4.0f; y >= 0.0f; y -= 1.0f )
        {
            that.set_shadow_blur( ( y * 5.0f + x ) * 0.5f - 0.5f );
            that.fill_rectangle( ( x + 0.25f ) * 0.2f * width,
                                 ( y + 0.25f ) * 0.2f * height,
                                 0.1f * width, 0.1f * height );
        }
}

void shadow_blur_offscreen( canvas &that, float width, float height )
{
    that.set_color( fill_style, 0.0f, 0.0f, 0.0f, 1.0f );
    that.set_shadow_color( 0.0f, 0.0f, 0.0f, 1.0f );
    that.set_shadow_blur( 5.0f );
    that.fill_rectangle( 0.0f, height * 2.0f, width, height );
    that.fill_rectangle( 0.0f, height * -2.0f, width, height );
    that.fill_rectangle( width + 1.0f, 0.0f, width, height );
    that.fill_rectangle( -width - 1.0f, 0.0f, width, height );
}

void shadow_blur_composite( canvas &that, float width, float height )
{
    float radius = min( width, height ) * 0.5f;
    that.arc( 0.5f * width, 0.5f * height, radius, 0.0f, 6.28318531f );
    that.clip();
    that.set_color( fill_style, 0.0f, 0.0f, 1.0f, 1.0f );
    that.set_shadow_color( 0.0f, 0.0f, 0.0f, 1.0f );
    that.fill_rectangle( 0.4f * width, 0.0f, 0.2f * width, height );
    that.global_composite_operation = destination_atop;
    that.set_color( stroke_style, 1.0f, 0.0f, 0.0f, 1.0f );
    float dashing[] = { 16.0f, 4.0f };
    that.set_line_dash( dashing, 2 );
    that.set_line_width( 15.0f );
    that.shadow_offset_x = 5.0f;
    that.shadow_offset_y = 5.0f;
    that.set_shadow_blur( 6.0f );
    that.set_shadow_color( 0.0f, 0.0f, 0.0f, 1.0f );
    that.begin_path();
    that.arc( 0.45f * width, 0.85f * height, radius * 0.5f, 0.0f, 6.28318531f );
    that.close_path();
    that.stroke();
    that.global_composite_operation = source_over;
    that.begin_path();
    that.arc( 0.75f * width, 0.25f * height, radius, 0.0f, 6.28318531f );
    that.close_path();
    that.stroke();
}

void line_width( canvas &that, float width, float height )
{
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    that.set_line_width( 4.0f );
    for ( float step = 0.0f; step < 16.0f; step += 1.0f )
    {
        float left = ( step + 0.25f ) / 16.0f * width;
        float right = ( step + 0.75f ) / 16.0f * width;
        that.begin_path();
        that.move_to( left, 0.0f );
        that.bezier_curve_to( left, 0.5f * height,
                              right, 0.5f * height,
                              right, height );
        that.set_line_width( 0.5f * ( step - 1 ) );
        that.stroke();
    }
    that.set_color( fill_style, 1.0f, 1.0f, 1.0f, 1.0f );
    that.global_composite_operation = source_atop;
    that.fill_rectangle( 0.0f, 0.5f * height, width, 0.5f * height );
    that.global_composite_operation = destination_over;
    that.fill_rectangle( 0.0f, 0.25f * height, width, 0.25f * height );
    that.set_color( fill_style, 0.0f, 0.0f, 0.0f, 1.0f );
    that.fill_rectangle( 0.0f, 0.5f * height, width, 0.25f * height );
}

void line_width_angular( canvas &that, float width, float height )
{
    for ( float step = 0.0f; step < 5.0f; step += 1.0f )
    {
        float grey = ( step + 1.0f ) / 5.0f;
        that.set_color( stroke_style, grey, grey, grey, 1.0f );
        that.begin_path();
        that.move_to( 0.1f * width, 0.1f * height );
        that.bezier_curve_to( 1.2f * width, 1.0f * height,
                              1.2f * width, -0.0f * height,
                              0.1f * width, 0.9f * height );
        that.set_line_width( 30.0f - 7.0f * step );
        that.stroke();
    }
}

void line_cap( canvas &that, float width, float height )
{
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    that.set_line_width( 24.0f );
    cap_style const caps[] = { butt, square, circle };
    for ( int index = 0; index < 3; ++index )
    {
        float right = static_cast< float >( index + 1 ) / 3.0f * width - 20.0f;
        that.begin_path();
        that.move_to( right, 0.125f * height );
        that.bezier_curve_to( right, 0.125f * height + 100.0f,
                              right - 100.0f, 0.875f * height,
                              right, 0.875f * height );
        that.line_cap = caps[ index ];
        that.stroke();
    }
}

void line_cap_offscreen( canvas &that, float width, float height )
{
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    that.set_line_width( 36.0f );
    cap_style const caps[] = { butt, square, circle };
    for ( int index = 0; index < 3; ++index )
    {
        float x = ( static_cast< float >( index ) + 0.5f ) / 3.0f * width;
        float y = ( static_cast< float >( index ) + 0.5f ) / 3.0f * height;
        that.begin_path();
        that.move_to( x, -19.0f );
        that.line_to( x, -9.0f );
        that.move_to( x, height + 17.0f );
        that.line_to( x, height + 27.0f );
        that.move_to( -27.0f, y );
        that.line_to( -17.0f, y );
        that.move_to( width + 9.0f, y );
        that.line_to( width + 19.0f, y );
        that.line_cap = caps[ index ];
        that.stroke();
    }
}

void line_join( canvas &that, float width, float height )
{
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    that.set_line_width( 16.0f );
    join_style const joins[] = { miter, bevel, rounded };
    for ( int index = 0; index < 3; ++index )
    {
        float left = ( static_cast< float >( index ) + 0.25f ) / 3.0f * width;
        float right = ( static_cast< float >( index ) + 0.75f ) / 3.0f * width;
        that.begin_path();
        that.move_to( left, 0.2f * height );
        that.line_to( left, 0.1f * height );
        that.line_to( left, 0.2f * height );
        that.line_to( right, 0.2f * height );
        that.line_to( left, 0.2f * height );
        that.line_to( left, 0.3f * height );
        that.line_to( right, 0.3f * height );
        that.line_to( right, 0.4f * height );
        that.line_to( right, 0.5f * height );
        that.line_to( left, 0.4f * height );
        that.line_to( left, 0.5f * height );
        that.line_to( right, 0.6f * height );
        that.bezier_curve_to( right, height,
                              left, 0.4f * height,
                              left, 0.7f * height );
        that.bezier_curve_to( left, 0.8f * height,
                              right, 0.8f * height,
                              right, 0.9f * height );
        that.line_join = joins[ index ];
        that.stroke();
    }
}

void line_join_offscreen( canvas &that, float width, float height )
{
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    that.set_line_width( 36.0f );
    join_style const joins[] = { miter, bevel, rounded };
    for ( int index = 0; index < 3; ++index )
    {
        float x = ( static_cast< float >( index ) + 0.5f ) / 3.0f * width;
        float y = ( static_cast< float >( index ) + 0.5f ) / 3.0f * height;
        that.begin_path();
        that.move_to( x - 10.0f, -55.0f );
        that.line_to( x - 10.0f, -5.0f );
        that.line_to( x + 10.0f, -55.0f );
        that.move_to( x - 10.0f, height + 130.0f );
        that.line_to( x + 10.0f, height + 80.0f );
        that.line_to( x + 10.0f, height + 130.0f );
        that.move_to( -130.0f, y - 10.0f );
        that.line_to( -80.0f, y - 10.0f );
        that.line_to( -130.0f, y + 10.0f );
        that.move_to( height + 55.0f, y - 10.0f );
        that.line_to( height + 5.0f, y + 10.0f );
        that.line_to( height + 55.0f, y + 10.0f );
        that.line_join = joins[ index ];
        that.stroke();
    }
}

void miter_limit( canvas &that, float width, float height )
{
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    for ( float line = 0.0f; line < 4.0f; line += 1.0f )
    {
        that.set_line_width( 1.5f * line + 1.0f );
        that.set_miter_limit( 20.0f );
        for ( float limit = 0.0f; limit < 8.0f; limit += 1.0f )
        {
            float left = ( limit + 0.2f ) / 8.0f * width;
            float middle = ( limit + 0.5f ) / 8.0f * width;
            float right = ( limit + 0.7f ) / 8.0f * width;
            float top = ( line + 0.3f ) / 4.0f * height;
            float bottom = ( line + 0.7f ) / 4.0f * height;
            that.begin_path();
            that.move_to( left, bottom );
            that.line_to( left, top );
            that.line_to( right, bottom );
            that.line_to( middle, top );
            that.set_miter_limit( 1.5f * limit );
            that.stroke();
        }
    }
}

void line_dash_offset( canvas &that, float width, float height )
{
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    that.set_line_width( 6.0f );
    float segments[] = { 20.0f, 8.0f, 8.0f, 8.0f };
    that.set_line_dash( segments, 4 );
    for ( float step = 0.0f; step < 16.0f; step += 1.0f )
    {
        float left = ( step + 0.125f ) / 16.0f * width;
        float right = ( step + 0.875f ) / 16.0f * width;
        that.begin_path();
        that.move_to( left, 0.0f );
        that.line_to( right, 0.125f * height );
        that.line_to( left, 0.375f * height );
        that.line_to( right, 0.625f * height );
        that.line_to( left, 0.875f * height );
        that.line_to( right, height );
        that.line_dash_offset = ( step / 16.0f - 0.5f ) * 44.0f;
        that.stroke();
    }
}

void line_dash( canvas &that, float width, float height )
{
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    that.set_line_width( 6.0f );
    float segments_1[] = { 10.0f };
    that.set_line_dash( segments_1, 1 );
    that.stroke();
    that.move_to( 0.0f, 0.0f );
    that.stroke();
    that.begin_path();
    that.move_to( width * 0.25f, 0.0f );
    that.line_to( width * 0.25f, height );
    that.stroke();
    float segments_2[] = { 20.0f, -8.0f };
    that.set_line_dash( segments_2, 2 );
    that.begin_path();
    that.move_to( width * 0.375f, 0.0f );
    that.line_to( width * 0.375f, height );
    that.stroke();
    float segments_3[] = { 20.0f, 8.0f, 8.0f, 8.0f };
    that.set_line_dash( segments_3, 4 );
    that.begin_path();
    that.move_to( width * 0.5f, 0.0f );
    that.line_to( width * 0.5f, height );
    that.stroke();
    float segments_4[] = { 0.0f, 8.0f, 2.0f, 8.0f };
    that.set_line_dash( segments_4, 4 );
    that.begin_path();
    that.move_to( width * 0.625f, 0.0f );
    that.line_to( width * 0.625f, height );
    that.stroke();
    that.set_line_dash( 0, 0 );
    that.begin_path();
    that.move_to( width * 0.75f, 0.0f );
    that.line_to( width * 0.75f, height );
    that.stroke();
}

void line_dash_closed( canvas &that, float width, float height )
{
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    that.set_line_width( 32.0f );
    float segments_1[] = { 96.0f, 32.0f };
    that.set_line_dash( segments_1, 2 );
    that.line_dash_offset = -80.0f;
    that.stroke_rectangle( 0.25f * width, 0.25f * height,
                           0.5f * width, 0.5f * height );
    float segments_2[] = { 96.0f, 32.0f, 1024.0f, 16.0f };
    that.set_line_dash( segments_2, 4 );
    that.line_dash_offset = 128.0f;
    that.stroke_rectangle( 0.09375f * width, 0.09375f * height,
                           0.8125f * width, 0.8125f * height );
}

void line_dash_overlap( canvas &that, float width, float height )
{
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    that.set_color( fill_style, 1.0f, 0.0f, 0.0f, 1.0f );
    that.line_cap = circle;
    that.set_line_width( 16.0f );
    float segments[] = { 14.0f, 12.0f };
    that.set_line_dash( segments, 2 );
    for ( int index = 0; index < 4; ++index )
    {
        float flip = ( index == 3 ? -1.0f : 1.0f );
        float top_y = ( index & 1 ? 0.25f : 0.1f ) * height;
        float bottom_y = ( index & 1 ? 0.9f : 0.75f ) * height;
        float mid_x = ( index & 2 ? 0.75f : 0.25f ) * width;
        float top_width = ( index & 1 ? 0.25f : 0.55f ) * flip * width;
        float bottom_width = ( index & 1 ? 0.55f : 0.25f ) * flip * width;
        that.move_to( mid_x, top_y );
        that.bezier_curve_to( mid_x - top_width, top_y,
                              mid_x + bottom_width, bottom_y,
                              mid_x, bottom_y );
        that.bezier_curve_to( mid_x - bottom_width, bottom_y,
                              mid_x + top_width, top_y,
                              mid_x, top_y );
        that.close_path();
    }
    that.fill();
    that.stroke();
}

void line_dash_offscreen( canvas &that, float width, float height )
{
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    that.set_line_width( 6.0f );
    float segments[] = {
        0.0f, width * 20.5f * 3.14159265f - height * 0.5f + 1.0f,
        height - 2.0f, 0.0f };
    that.set_line_dash( segments, 4 );
    for ( float step = -2.0f; step <= 2.0f; step += 1.0f )
    {
        that.begin_path();
        that.arc( width * -20.0f, height * 0.5f,
                  width * ( 20.5f - step * 0.1f ),
                  3.14159265f, 1.5707963268f );
        that.line_dash_offset = width * step * 0.1f * 3.14159265f;
        that.stroke();
    }
}

void color( canvas &that, float width, float height )
{
    float radius = min( width, height ) * 0.4f;
    that.set_color( fill_style, 2.0f, -1.0f, 0.0f, 0.5f );
    that.set_color( stroke_style, 0.0f, 0.0f, 1.0f, 1.5f );
    that.set_line_width( 16.0f );
    that.arc( 0.5f * width, 0.5f * height, radius, 0.0f, 6.28318531f );
    that.close_path();
    that.fill();
    that.stroke();
}

void linear_gradient( canvas &that, float width, float height )
{
    float radius = min( width, height ) * 0.4f;
    that.set_linear_gradient( fill_style,
                              0.3f * width, 0.3f * height,
                              0.7f * width, 0.7f * height );
    that.add_color_stop( fill_style, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f );
    that.add_color_stop( fill_style, 1.0f, 1.0f, 0.0f, 1.0f, 100.0f );
    that.set_linear_gradient( stroke_style,
                              0.3f * width, 0.7f * height,
                              0.7f * width, 0.3f * height );
    that.add_color_stop( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f );
    that.add_color_stop( stroke_style, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f );
    that.set_line_width( 16.0f );
    that.arc( 0.5f * width, 0.5f * height, radius, 0.0f, 6.28318531f );
    that.close_path();
    that.fill();
    that.stroke();
    that.set_linear_gradient( stroke_style,
                              0.5f * width, 0.5f * height,
                              0.5f * width, 0.5f * height );
    that.add_color_stop( stroke_style, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f );
    that.add_color_stop( stroke_style, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f );
    that.stroke();
}

void radial_gradient( canvas &that, float width, float height )
{
    float radius = min( width, height ) * 0.4f;
    that.set_radial_gradient( fill_style,
                              0.0f, 0.0f, radius,
                              width, height, 0.5f * radius );
    that.add_color_stop( fill_style, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f );
    that.add_color_stop( fill_style, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f );
    that.set_radial_gradient( stroke_style,
                              0.0f, height, radius,
                              width, 0.0f, 0.5f * radius );
    that.add_color_stop( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f );
    that.add_color_stop( stroke_style, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f );
    that.set_line_width( 16.0f );
    that.arc( 0.5f * width, 0.5f * height, radius, 0.0f, 6.28318531f );
    that.close_path();
    that.fill();
    that.stroke();
    that.set_radial_gradient( stroke_style,
                              0.5f * width, 0.4f * height, 10.0f,
                              0.5f * width, 0.6f * height, 0.0f );
    that.set_radial_gradient( stroke_style,
                              0.0f, 0.5f * height, -10.0f,
                              width, 0.5f * height, 10.0f );
    that.add_color_stop( stroke_style, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f );
    that.add_color_stop( stroke_style, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f );
    that.stroke();
    that.set_radial_gradient( fill_style,
                              0.5f * width, 0.5f * height, 0.0f,
                              0.5f * width, 0.5f * height, radius );
    that.add_color_stop( fill_style, 0.15f, 0.0f, 0.0f, 0.0f, 1.0f );
    that.add_color_stop( fill_style, 0.20f, 0.0f, 0.0f, 0.0f, 0.0f );
    that.fill();
}

void color_stop( canvas &that, float width, float height )
{
    that.add_color_stop( fill_style, 0.5f, 1.0f, 0.0f, 1.0f, 1.0f );
    that.set_linear_gradient( fill_style,
                              0.1f * width, 0.0f,
                              0.9f * width, 0.0f );
    that.fill_rectangle( 0.0f, 0.0f, width, 0.1f * height );
    that.add_color_stop( fill_style, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f );
    that.add_color_stop( fill_style, 2.0f, 1.0f, 0.0f, 0.0f, 1.0f );
    that.add_color_stop( fill_style, 0.3f, -1.0f, 0.0f, 2.0f, 2.0f );
    that.add_color_stop( fill_style, 0.3f, 1.0f, 1.0f, 1.0f, 1.0f );
    that.add_color_stop( fill_style, 0.3f, 0.0f, 0.0f, 0.0f, 1.0f );
    that.add_color_stop( fill_style, 0.0f, 0.0f, 0.0f, 0.8f, 1.0f );
    that.add_color_stop( fill_style, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f );
    that.add_color_stop( fill_style, 0.7f, 0.9f, 0.9f, 0.9f, 1.0f );
    that.add_color_stop( fill_style, 0.6f, 0.1f, 0.1f, 0.1f, 1.0f );
    that.fill_rectangle( 0.0f, 0.1f * height, width, 0.4f * height );
    that.fill_rectangle( 0.0f, 0.5f * height, width, 0.4f * height );
}

void pattern( canvas &that, float width, float height )
{
    unsigned char checker[ 256 ];
    for ( int index = 0; index < 256; ++index )
        checker[ index ] = static_cast< unsigned char >(
            ( ( ( index >> 2 & 1 ) ^ ( index >> 5 & 1 ) ) |
              ( ( index & 3 ) == 3 ) ) * 255 );
    that.arc( 0.5f * width, 0.5f * height, 32.0f, 0.0f, 6.28318531f );
    that.close_path();
    that.set_line_width( 20.0f );
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    that.set_pattern( stroke_style, 0, 8, 8, 32, repeat );
    that.stroke();
    that.set_line_width( 16.0f );
    that.set_pattern( stroke_style, checker, 8, 8, 32, repeat );
    that.stroke();
    for ( float scale = 8.0f; scale >= 1.0f; scale /= 2.0f )
    {
        that.set_transform( 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f );
        that.scale( scale, scale );
        float size_x = 0.5f * width / scale;
        float size_y = 0.5f * height / scale;
        that.set_pattern( fill_style, checker, 8, 8, 32, no_repeat );
        that.fill_rectangle( 0.0f, 0.0f, size_x, size_y );
        that.set_pattern( fill_style, checker, 8, 8, 32, repeat_x );
        that.fill_rectangle( size_x, 0.0f, size_x, size_y );
        that.set_pattern( fill_style, checker, 8, 8, 32, repeat_y );
        that.fill_rectangle( 0.0f, size_y, size_x, size_y );
        that.set_pattern( fill_style, checker, 8, 8, 32, repeat );
        that.fill_rectangle( size_x, size_y, size_x, size_y );
    }
}

void begin_path( canvas &that, float width, float height )
{
    that.move_to( 0.0f, 0.0f );
    that.line_to( width, height );
    that.stroke();
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    that.begin_path();
    that.begin_path();
    that.move_to( width, 0.0f );
    that.line_to( 0.0f, height );
    that.stroke();
    that.begin_path();
    that.line_to( 0.5f * width, height );
    that.stroke();
}

void move_to( canvas &that, float width, float height )
{
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    that.set_color( fill_style, 1.0f, 0.0f, 0.0f, 1.0f );
    that.set_line_width( 8.0f );
    that.move_to( 0.6f * width, height );
    that.move_to( 0.4f * width, 0.1f * height );
    that.line_to( 0.2f * width, 0.5f * height );
    that.line_to( 0.4f * width, 0.9f * height );
    that.move_to( 0.6f * width, 0.2f * height );
    that.line_to( 0.8f * width, 0.4f * height );
    that.move_to( 0.8f * width, 0.6f * height );
    that.line_to( 0.6f * width, 0.8f * height );
    that.move_to( 0.7f * width, 0.5f * height );
    that.line_to( 0.7f * width, 0.5f * height );
    that.fill();
    that.stroke();
}

void close_path( canvas &that, float width, float height )
{
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    that.set_color( fill_style, 1.0f, 0.0f, 0.0f, 1.0f );
    that.set_line_width( 8.0f );
    that.close_path();
    that.line_to( 0.5f * width, 0.5f * height );
    that.line_to( 0.2f * width, 0.8f * height );
    that.line_to( 0.2f * width, 0.2f * height );
    that.close_path();
    that.line_to( 0.5f * width, 0.2f * height );
    that.line_to( 0.8f * width, 0.2f * height );
    that.close_path();
    that.close_path();
    that.move_to( 0.5f * width, 0.8f * height );
    that.line_to( 0.8f * width, 0.8f * height );
    that.line_to( 0.8f * width, 0.5f * height );
    that.close_path();
    that.fill();
    that.stroke();
}

void line_to( canvas &that, float width, float height )
{
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    that.set_color( fill_style, 1.0f, 0.0f, 0.0f, 1.0f );
    that.set_line_width( 16.0f );
    that.line_to( 0.1f * width, 0.2f * height );
    that.line_to( 0.1f * width, 0.2f * height );
    that.line_to( 0.2f * width, 0.5f * height );
    that.line_to( 0.2f * width, 0.5f * height );
    that.line_to( 0.3f * width, 0.8f * height );
    that.line_to( 0.4f * width, 0.2f * height );
    that.line_to( 0.4f * width, 0.2f * height );
    that.line_to( 0.6f * width, 0.8f * height );
    that.line_to( 0.6f * width, 0.8f * height );
    that.move_to( 0.7f * width, 0.4f * height );
    that.line_to( 0.9f * width, 0.4f * height );
    that.line_to( 0.9f * width, 0.6f * height );
    that.line_to( 0.7f * width, 0.6f * height );
    that.line_to( 0.7f * width, 0.4f * height );
    that.fill();
    that.stroke();
}

void quadratic_curve_to( canvas &that, float width, float height )
{
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    that.set_color( fill_style, 1.0f, 0.0f, 0.0f, 1.0f );
    that.set_line_width( 8.0f );
    that.quadratic_curve_to( 0.1f * width, 0.2f * height,
                             0.1f * width, 0.2f * height );
    that.quadratic_curve_to( 0.2f * width, 0.5f * height,
                             0.2f * width, 0.5f * height );
    that.quadratic_curve_to( 0.3f * width, 0.8f * height,
                             0.4f * width, 0.2f * height );
    that.quadratic_curve_to( 0.6f * width, 0.8f * height,
                             0.7f * width, 0.2f * height );
    that.move_to( 0.7f * width, 0.6f * height );
    that.quadratic_curve_to( 0.9f * width, 0.6f * height,
                             0.9f * width, 0.8f * height );
    that.quadratic_curve_to( 0.9f * width, 0.9f * height,
                             0.7f * width, 0.9f * height );
    that.close_path();
    that.move_to( 0.1f * width, 0.9f * height );
    that.quadratic_curve_to( 0.5f * width, 0.5f * height,
                             0.1f * width, 0.9f * height );
    that.fill();
    that.stroke();
}

void bezier_curve_to( canvas &that, float width, float height )
{
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    that.set_color( fill_style, 1.0f, 0.0f, 0.0f, 1.0f );
    that.set_line_width( 8.0f );
    that.bezier_curve_to( 0.9f * width, 0.9f * height,
                          0.6f * width, 0.6f * height,
                          0.6f * width, 0.9f * height );
    that.move_to( 0.1f * width, 0.1f * height );
    that.bezier_curve_to( 0.9f * width, 0.9f * height,
                          0.9f * width, 0.1f * height,
                          0.1f * width, 0.9f * height );
    that.move_to( 0.4f * width, 0.1f * height );
    that.bezier_curve_to( 0.1f * width, 0.3f * height,
                          0.7f * width, 0.3f * height,
                          0.4f * width, 0.1f * height );
    that.move_to( 0.9f * width, 0.1f * height );
    that.bezier_curve_to( 0.6f * width, 0.2f * height,
                          0.9f * width, 0.1f * height,
                          0.6f * width, 0.2f * height );
    that.move_to( 0.7f * width, 0.3f * height );
    that.bezier_curve_to( 0.9f * width, 0.3f * height,
                          0.9f * width, 0.4f * height,
                          0.8f * width, 0.5f * height );
    that.bezier_curve_to( 0.7f * width, 0.6f * height,
                          0.7f * width, 0.7f * height,
                          0.9f * width, 0.7f * height );
    that.fill();
    that.stroke();
}

void arc_to( canvas &that, float width, float height )
{
    float radius = min( width, height ) * 0.5f;
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    that.set_color( fill_style, 1.0f, 0.0f, 0.0f, 1.0f );
    that.set_line_width( 8.0f );
    that.arc_to( 0.3f * width, 0.3f * height,
                 0.5f * width, 0.5f * height, 16.0f );
    that.move_to( 0.4f * width, 0.4f * height );
    that.arc_to( 0.7f * width, 0.1f * height,
                 0.7f * width, 0.4f * height, 0.0f );
    that.arc_to( 0.9f * width, 0.5f * height,
                 0.7f * width, 0.7f * height, 0.125f * radius );
    that.arc_to( 0.5f * width, 0.9f * height,
                 0.3f * width, 0.8f * height, 0.25f * radius );
    that.arc_to( 0.1f * width, 0.7f * height,
                 0.4f * width, 0.4f * height, 0.375f * radius );
    that.close_path();
    that.move_to( 0.1f * width, 0.6f * height );
    that.transform( 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.2f * height );
    that.arc_to( 0.1f * width, 0.9f * height,
                 0.5f * width, 0.9f * height, 0.3f * radius );
    that.set_transform( 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f );
    that.close_path();
    that.move_to( 0.2f * width, 0.1f * height );
    that.arc_to( 0.1f * width, 0.1f * height,
                 0.1f * width, 0.7f * height, 0.6f * radius );
    that.arc_to( 0.2f * width, 0.4f * height,
                 0.2f * width, 0.4f * height, 0.5f * radius );
    that.arc_to( 0.4f * width, 0.2f * height,
                 0.2f * width, 0.4f * height, 0.5f * radius );
    that.arc_to( 0.5f * width, 0.5f * height,
                 0.9f * width, 0.1f * height, -1.0f );
    that.move_to( 0.6f * width, 0.9f * height );
    that.set_transform( 0.0f, 0.0f, 0.0f, 1.0f, 0.9f * width, 0.0f );
    that.arc_to( 0.9f * width, 0.9f * height,
                 0.9f * width, 0.6f * height, 0.3f * radius );
    that.set_transform( 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f );
    that.arc_to( 0.9f * width, 0.6f * height,
                 0.9f * width, 0.6f * height, 0.0f );
    that.fill();
    that.stroke();
}

void arc( canvas &that, float width, float height )
{
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    that.set_color( fill_style, 1.0f, 0.0f, 0.0f, 1.0f );
    that.set_line_width( 8.0f );
    for ( int i = 0; i < 4; ++i )
        for ( int j = 0; j < 3; ++j )
        {
            float x = ( static_cast< float >( j ) + 0.5f ) * width / 3.0f;
            float y = ( static_cast< float >( i ) + 0.5f ) * height / 4.0f;
            float radius = min( width, height ) * 0.1f;
            float start = ( 3.14159265f + 1.0e-6f ) *
                static_cast< float >( i % 2 );
            float end = ( 3.14159265f + 1.0e-6f ) *
                ( 1.0f + 0.5f * static_cast< float >( j ) );
            bool counter = i / 2;
            that.begin_path();
            that.arc( x, y, -radius, start, end, counter );
            that.arc( x, y, radius, start, end, counter );
            that.close_path();
            that.fill();
            that.stroke();
        }
}

void rectangle( canvas &that, float width, float height )
{
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    that.set_color( fill_style, 1.0f, 0.0f, 0.0f, 1.0f );
    that.set_line_width( 8.0f );
    that.move_to( 0.3f * width, 0.3f * height );
    that.line_to( 0.7f * width, 0.3f * height );
    that.line_to( 0.7f * width, 0.7f * height );
    that.line_to( 0.3f * width, 0.7f * height );
    that.close_path();
    that.move_to( 0.0f, 0.0f );
    for ( float y = -1.0f; y <= 1.0f; y += 1.0f )
        for ( float x = -1.0f; x <= 1.0f; x += 1.0f )
            that.rectangle( ( 0.5f + 0.1f * x ) * width,
                            ( 0.5f + 0.1f * y ) * height,
                            x * 0.3f * width, y * 0.3f * height );
    that.line_to( width, height );
    that.fill();
    that.stroke();
}

void fill( canvas &that, float width, float height )
{
    float radius = min( width, height ) * 0.45f;
    that.set_color( fill_style, 0.0f, 0.0f, 0.0f, 1.0f );
    that.fill();
    that.begin_path();
    for ( float step = 0.0f; step < 128.0f; step += 1.0f )
    {
        float angle = step * ( 59.0f / 128.0f * 6.28318531f );
        float x = cosf( angle ) * radius + width / 2.0f;
        float y = sinf( angle ) * radius + height / 2.0f;
        that.line_to( x, y );
    }
    that.close_path();
    that.fill();
    that.set_color( fill_style, 1.0f, 0.0f, 0.0f, 1.0f );
    that.scale( 0.0f, 1.0f );
    that.fill();
}

void fill_rounding( canvas &that, float width, float height )
{
    static_cast< void >( width );
    static_cast< void >( height );
    that.set_color( fill_style, 0.0f, 0.0f, 0.0f, 1.0f );
    that.begin_path();
    that.move_to( 4.00000191f, 4.00000763f );
    that.line_to( 3.99999809f, 192.0f );
    that.line_to( 28.0000019f, 192.0f );
    that.close_path();
    that.move_to( -10390.0664f, 52.3311195f );
    that.line_to( -10389.9941f, 47.6248589f );
    that.line_to( -10395.9941f, 47.5328255f );
    that.line_to( -10396.0664f, 52.2478294f );
    that.close_path();
    that.move_to( 110.0f, 256.0f );
    that.line_to( 124.086205f, 255.998276f );
    that.line_to( 123.203453f, 0.0f );
    that.close_path();
    that.fill();
}

void fill_converging( canvas &that, float width, float height )
{
    float radius = min( width, height ) * 0.48f;
    that.set_color( fill_style, 0.0f, 0.0f, 0.0f, 1.0f );
    for ( float step = 0.0f; step < 256.0f; step += 1.0f )
    {
        float angle_1 = ( step + 0.0f ) / 256.0f * 6.28318531f;
        float angle_2 = ( step + 0.5f ) / 256.0f * 6.28318531f;
        that.move_to( width / 2.0f + 0.5f, height / 2.0f + 0.5f );
        that.line_to( cosf( angle_1 ) * radius + width  / 2.0f + 0.5f,
                      sinf( angle_1 ) * radius + height / 2.0f + 0.5f );
        that.line_to( cosf( angle_2 ) * radius + width  / 2.0f + 0.5f,
                      sinf( angle_2 ) * radius + height / 2.0f + 0.5f );
        that.close_path();
    }
    that.fill();
}

void fill_zone_plate( canvas &that, float width, float height )
{
    float radius = floorf( min( width, height ) * 0.48f / 4.0f ) * 4.0f;
    that.set_color( fill_style, 0.0f, 0.0f, 0.0f, 1.0f );
    for ( float step = 0.0f; step < radius; step += 2.0f )
    {
        float inner = sqrtf( ( step + 0.0f ) / radius ) * radius;
        float outer = sqrtf( ( step + 1.0f ) / radius ) * radius;
        that.move_to( width / 2.0f + inner, height / 2.0f );
        that.arc( width / 2.0f, height / 2.0f, inner,
                  0.0f, 6.28318531f );
        that.close_path();
        that.move_to( width / 2.0f + outer, height / 2.0f );
        that.arc( width / 2.0f, height / 2.0f, outer,
                  6.28318531f, 0.0f, true );
        that.close_path();
    }
    that.fill();
}

void stroke( canvas &that, float width, float height )
{
    float radius = min( width, height ) * 0.45f;
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    that.stroke();
    that.begin_path();
    for ( float step = 0.0f; step < 128.0f; step += 1.0f )
    {
        float angle = step * ( 59.0f / 128.0f * 6.28318531f );
        float x = cosf( angle ) * radius + width / 2.0f;
        float y = sinf( angle ) * radius + height / 2.0f;
        that.line_to( x, y );
    }
    that.close_path();
    that.stroke();
    that.set_color( stroke_style, 1.0f, 0.0f, 0.0f, 1.0f );
    that.scale( 0.0f, 1.0f );
    that.stroke();
}

void stroke_wide( canvas &that, float width, float height )
{
    that.scale( width / 256.0f, height / 256.0f );
    that.line_join = rounded;
    that.move_to( 24.0f, 104.0f );
    that.bezier_curve_to( 112.0f, 24.0f, 16.0f, 24.0f, 104.0f, 104.0f );
    that.move_to( 152.0f, 104.0f );
    that.bezier_curve_to( 232.8f, 24.0f, 151.2f, 24.0f, 232.0f, 104.0f );
    that.move_to( 24.0f, 232.0f );
    that.bezier_curve_to( 104.0f, 152.0f, 24.0f, 152.0f, 104.0f, 232.0f );
    that.move_to( 188.0f, 232.0f );
    that.bezier_curve_to( 196.0f, 184.0f, 188.0f, 184.0f, 196.0f, 192.0f );
    that.set_line_width( 40.0f );
    that.stroke();
    that.set_color( stroke_style, 1.0f, 0.0f, 0.0f, 1.0f );
    that.set_line_width( 1.0f );
    that.stroke();
}

void stroke_inner_join( canvas &that, float width, float height )
{
    join_style const joins[] = { miter, bevel, rounded };
    for ( int index = 0; index < 3; ++index )
    {
        float center = ( static_cast< float >( index ) + 0.5f ) / 3.0f * width;
        that.begin_path();
        that.move_to( center - 0.05f * width, 0.275f * height );
        that.line_to( center, 0.225f * height );
        that.line_to( center + 0.025f * width, 0.25f * height );
        that.move_to( center - 0.05f * width, 0.775f * height );
        that.bezier_curve_to( center, 0.725f * height,
                              center, 0.725f * height,
                              center + 0.025f * width, 0.75f * height );
        that.line_join = joins[ index ];
        that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
        that.set_line_width( 0.3f * width );
        that.stroke();
        that.set_color( stroke_style, 1.0f, 0.0f, 0.0f, 1.0f );
        that.set_line_width( 1.0f );
        that.stroke();
    }
}

void stroke_spiral( canvas &that, float width, float height )
{
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    that.set_line_width( 2.0f );
    that.begin_path();
    float outside = min( width, height ) * 0.48f;
    for ( float step = 0.0f; step <= 2048.0f; step += 1.0f )
    {
        float parameter = ( step - 1024.0f ) / 1024.0f;
        float angle = fabsf( parameter ) * 12.0f * 6.28318531f;
        float radius = parameter * outside;
        that.line_to( cosf( angle ) * radius + width * 0.5f,
                      sinf( angle ) * radius + height * 0.5f );
    }
    that.stroke();
}

void stroke_long( canvas &that, float width, float height )
{
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    for ( float step = 0.0f; step <= 29.0f; step += 1.0f )
    {
        that.move_to( 0.4f * width, -23.0f * height );
        that.line_to( width * step / 29.0f, height );
        that.move_to( -23.0f * width, 0.4f * height );
        that.line_to( width, height * step / 29.0f );
    }
    that.stroke();
}

void clip( canvas &that, float width, float height )
{
    float radius = min( width, height ) * 0.5f;
    that.set_line_width( 8.0f );
    for ( int step = 0; step < 8; ++step )
    {
        float fraction = static_cast< float >( step ) / 8.0f;
        float angle = fraction * 6.28318531f;
        that.set_color( stroke_style,
                        0.0f, static_cast< float >( step & 1 ), 0.0f, 1.0f );
        that.begin_path();
        that.arc( 0.5f * width + 0.8f * radius * cosf( angle ),
                  0.5f * height + 0.8f * radius * sinf( angle ),
                  radius, 0.0f, 6.28318531f );
        that.close_path();
        that.stroke();
        that.clip();
    }
    that.begin_path();
    that.clip();
    that.set_color( fill_style, 1.0, 0.0f, 1.0f, 1.0f );
    that.fill_rectangle( 0.0f, 0.0f, width, height );
}

void clip_winding( canvas &that, float width, float height )
{
    that.move_to( 0.125f * width, 0.125f * height );
    that.line_to( 0.625f * width, 0.125f * height );
    that.line_to( 0.625f * width, 0.625f * height );
    that.line_to( 0.125f * width, 0.625f * height );
    that.move_to( 0.250f * width, 0.250f * height );
    that.line_to( 0.750f * width, 0.250f * height );
    that.line_to( 0.750f * width, 0.750f * height );
    that.line_to( 0.250f * width, 0.750f * height );
    that.move_to( 0.375f * width, 0.375f * height );
    that.line_to( 0.375f * width, 0.875f * height );
    that.line_to( 0.875f * width, 0.875f * height );
    that.line_to( 0.875f * width, 0.375f * height );
    that.set_color( fill_style, 1.0f, 0.0f, 0.0f, 1.0f );
    that.fill();
    that.clip();
    that.set_line_width( 4.0f );
    that.stroke();
    that.set_line_width( 6.0f );
    that.begin_path();
    for ( float step = 0.0f; step < 32.0f; step += 1.0f )
    {
        that.move_to( step / 16.0f * width, 0.0f );
        that.line_to( step / 16.0f * width - width, height );
    }
    that.stroke();
}

void is_point_in_path( canvas &that, float width, float height )
{
    that.set_color( fill_style, 0.0f, 0.0f, 1.0f, 1.0f );
    that.set_color( stroke_style, 1.0f, 1.0f, 1.0f, 1.0f );
    if ( that.is_point_in_path( 0.0f, 0.0f ) )
        that.fill_rectangle( 0.0f, 0.0f, 16.0f, 16.0f );
    that.scale( width / 256.0f, height / 256.0f );
    that.begin_path();
    that.move_to( 65.0f, 16.0f );
    that.line_to( 113.0f, 24.0f );
    that.bezier_curve_to( 113.0f, 24.0f, 93.0f, 126.0f, 119.0f, 160.0f );
    that.bezier_curve_to( 133.0f, 180.0f, 170.0f, 196.0f, 186.0f, 177.0f );
    that.bezier_curve_to( 198.0f, 162.0f, 182.0f, 130.0f, 166.0f, 118.0f );
    that.bezier_curve_to( 123.0f, 80.0f, 84.0f, 124.0f, 84.0f, 124.0f );
    that.line_to( 35.0f, 124.0f );
    that.line_to( 18.0f, 56.0f );
    that.line_to( 202.0f, 56.0f );
    that.line_to( 202.0f, 90.0f );
    that.bezier_curve_to( 202.0f, 90.0f, 240.0f, 168.0f, 209.0f, 202.0f );
    that.bezier_curve_to( 175.0f, 240.0f, 65.0f, 187.0f, 65.0f, 187.0f );
    that.close_path();
    that.translate( 40.0f, 160.0f );
    that.move_to( 110.0f, 0.0f );
    that.line_to( 0.0f, 0.0f );
    that.line_to( 0.0f, 0.0f );
    that.bezier_curve_to( 0.0f, 90.0f, 110.0f, 90.0f, 110.0f, 40.0f );
    that.close_path();
    that.fill();
    that.stroke();
    for ( int index = 0; index < 256; ++index )
    {
        int bits = index;
        bits = ( bits << 1 & 0xaa ) | ( bits >> 1 & 0x55 );
        bits = ( bits << 2 & 0xcc ) | ( bits >> 2 & 0x33 );
        bits = ( bits << 4 & 0xf0 ) | ( bits >> 4 & 0x0f );
        float x = static_cast< float >( bits ) / 256.0f * width;
        float y = static_cast< float >( index ) / 256.0f * height;
        that.rotate( 0.5f );
        bool inside = that.is_point_in_path( x, y );
        that.set_transform( 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f );
        that.set_color( stroke_style, 1.0f - inside, inside, 0.0f, 1.0f );
        that.stroke_rectangle( x - 1.5f, y - 1.5f, 3.0f, 3.0f );
    }
}

void is_point_in_path_offscreen( canvas &that, float width, float height )
{
    that.set_color( fill_style, 0.0f, 0.0f, 1.0f, 1.0f );
    that.set_color( stroke_style, 1.0f, 1.0f, 1.0f, 1.0f );
    that.scale( width / 256.0f, height / 256.0f );
    that.begin_path();
    that.move_to( 321.0f, -240.0f );
    that.line_to( 369.0f, -232.0f );
    that.bezier_curve_to( 369.0f, -232.0f, 349.0f, -130.0f, 375.0f, -96.0f );
    that.bezier_curve_to( 389.0f, -76.0f, 426.0f, -60.0f, 442.0f, -79.0f );
    that.bezier_curve_to( 454.0f, -94.0f, 438.0f, -126.0f, 422.0f, -138.0f );
    that.bezier_curve_to( 379.0f, -176.0f, 340.0f, -132.0f, 340.0f, -132.0f );
    that.line_to( 291.0f, -132.0f );
    that.line_to( 274.0f, -200.0f );
    that.line_to( 458.0f, -200.0f );
    that.line_to( 458.0f, -166.0f );
    that.bezier_curve_to( 458.0f, -166.0f, 496.0f, -88.0f, 465.0f, -54.0f );
    that.bezier_curve_to( 431.0f, -16.0f, 321.0f, -69.0f, 321.0f, -69.0f );
    that.close_path();
    that.translate( 40.0f, 160.0f );
    that.move_to( 366.0f, -256.0f );
    that.line_to( 256.0f, -256.0f );
    that.line_to( 256.0f, -256.0f );
    that.bezier_curve_to( 256.0f, -166.0f, 366.0f, -166.0f, 366.0f, -216.0f );
    that.close_path();
    that.fill();
    that.stroke();
    for ( int index = 0; index < 256; ++index )
    {
        int bits = index;
        bits = ( bits << 1 & 0xaa ) | ( bits >> 1 & 0x55 );
        bits = ( bits << 2 & 0xcc ) | ( bits >> 2 & 0x33 );
        bits = ( bits << 4 & 0xf0 ) | ( bits >> 4 & 0x0f );
        float x = static_cast< float >( bits ) / 256.0f * width;
        float y = static_cast< float >( index ) / 256.0f * height;
        that.rotate( 0.5f );
        bool inside = that.is_point_in_path( x + width, y - height );
        that.set_transform( 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f );
        that.set_color( stroke_style, 1.0f - inside, inside, 0.0f, 1.0f );
        that.stroke_rectangle( x - 1.5f, y - 1.5f, 3.0f, 3.0f );
    }
}

void clear_rectangle( canvas &that, float width, float height )
{
    that.set_color( stroke_style, 1.0f, 1.0f, 1.0f, 1.0f );
    that.set_color( fill_style, 0.4f, 0.05f, 0.2f, 1.0f );
    that.move_to( 0.0f, 0.0f );
    that.line_to( width, 0.0f );
    that.line_to( width, height );
    that.line_to( 0.0f, height );
    that.fill();
    that.rotate( 0.2f );
    that.begin_path();
    that.move_to( 0.2f * width, 0.2f * height );
    that.line_to( 0.8f * width, 0.2f * height );
    that.line_to( 0.8f * width, 0.8f * height );
    that.shadow_offset_x = 5.0f;
    that.set_shadow_color( 0.0f, 0.0f, 0.0f, 1.0f );
    that.global_composite_operation = destination_atop;
    that.set_global_alpha( 0.5f );
    for ( float y = -1.0f; y <= 1.0f; y += 1.0f )
        for ( float x = -1.0f; x <= 1.0f; x += 1.0f )
            that.clear_rectangle( ( 0.5f + 0.05f * x ) * width,
                                  ( 0.5f + 0.05f * y ) * height,
                                  x * 0.2f * width, y * 0.2f * height );
    that.set_global_alpha( 1.0f );
    that.global_composite_operation = source_over;
    that.set_shadow_color( 0.0f, 0.0f, 0.0f, 0.0f );
    that.line_to( 0.2f * width, 0.8f * height );
    that.close_path();
    that.stroke();
}

void fill_rectangle( canvas &that, float width, float height )
{
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    that.set_color( fill_style, 0.4f, 0.05f, 0.2f, 1.0f );
    that.rotate( 0.2f );
    that.move_to( 0.2f * width, 0.2f * height );
    that.line_to( 0.8f * width, 0.2f * height );
    that.line_to( 0.8f * width, 0.8f * height );
    for ( float y = -1.0f; y <= 1.0f; y += 1.0f )
        for ( float x = -1.0f; x <= 1.0f; x += 1.0f )
            that.fill_rectangle( ( 0.5f + 0.05f * x ) * width,
                                 ( 0.5f + 0.05f * y ) * height,
                                 x * 0.2f * width, y * 0.2f * height );
    that.line_to( 0.2f * width, 0.8f * height );
    that.close_path();
    that.stroke();
}

void stroke_rectangle( canvas &that, float width, float height )
{
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    that.rotate( 0.2f );
    that.begin_path();
    that.move_to( 0.2f * width, 0.2f * height );
    that.line_to( 0.8f * width, 0.2f * height );
    that.line_to( 0.8f * width, 0.8f * height );
    for ( float y = -1.0f; y <= 1.0f; y += 1.0f )
        for ( float x = -1.0f; x <= 1.0f; x += 1.0f )
            that.stroke_rectangle( ( 0.5f + 0.05f * x ) * width,
                                   ( 0.5f + 0.05f * y ) * height,
                                   x * 0.2f * width, y * 0.2f * height );
    that.line_to( 0.2f * width, 0.8f * height );
    that.close_path();
    that.stroke();
}

void text_align( canvas &that, float width, float height )
{
    that.set_font( &font_a[ 0 ], static_cast< int >( font_a.size() ), 0.2f * height );
    that.rotate( 0.2f );
    that.set_color( fill_style, 0.0f, 0.0f, 0.0f, 1.0f );
    align_style const alignments[] = { leftward, center, rightward, start, ending };
    for ( int index = 0; index < 5; ++index )
    {
        float base = ( 0.1f + 0.2f * static_cast< float >( index ) ) * height;
        that.text_align = alignments[ index ];
        that.fill_text( "HIty", 0.5f * width, base );
    }
    that.set_color( stroke_style, 1.0f, 0.0f, 0.0f, 0.5f );
    that.set_line_width( 1.0f );
    that.move_to( 0.0f, 0.5f * height );
    that.line_to( width, 0.5f * height );
    that.move_to( 0.5f * width, 0.0f );
    that.line_to( 0.5f * width, height );
    that.stroke();
}

void text_baseline( canvas &that, float width, float height )
{
    that.set_font( &font_a[ 0 ], static_cast< int >( font_a.size() ), 0.2f * height );
    that.rotate( 0.2f );
    that.set_color( fill_style, 0.0f, 0.0f, 0.0f, 1.0f );
    baseline_style const baselines[] = {
        alphabetic, top, middle, bottom, hanging, ideographic };
    for ( int index = 0; index < 6; ++index )
    {
        float left = ( 0.1f + 0.15f * static_cast< float >( index ) ) * width;
        that.text_baseline = baselines[ index ];
        that.fill_text( "Iy", left, 0.5f * height );
    }
    that.set_color( stroke_style, 1.0f, 0.0f, 0.0f, 0.5f );
    that.set_line_width( 1.0f );
    that.move_to( 0.0f, 0.5f * height );
    that.line_to( width, 0.5f * height );
    that.move_to( 0.5f * width, 0.0f );
    that.line_to( 0.5f * width, height );
    that.stroke();
}

void font( canvas &that, float width, float height )
{
    that.set_color( stroke_style, 1.0f, 0.0f, 0.0f, 1.0f );
    that.stroke_text( "D", 0.8f * width, 0.95f * height );
    that.set_color( fill_style, 1.0f, 0.0f, 0.0f, 1.0f );
    that.fill_text( "D", 0.9f * width, 0.95f * height );
    that.set_font( 0, 0, 0.1f * height );
    that.set_color( fill_style, 0.0f, 0.0f, 0.0f, 1.0f );
    that.set_font( &font_a[ 0 ], static_cast< int >( font_a.size() ), 0.2f * height );
    that.fill_text( "CE\xc3\x8d\xf4\x8f\xbf\xbd\xf0I", 0.0f, 0.20f * height );
    that.set_font( 0, 0, 0.1f * height );
    that.fill_text( "CE\xc3\x8d\xf4\x8f\xbf\xbd\xf0I", 0.65f * width, 0.20f * height );
    that.set_font( &font_b[ 0 ], static_cast< int >( font_b.size() ), 0.2f * height );
    that.fill_text( "CE\xc3\x8d\xf4\x8f\xbf\xbd\xf0I", 0.0f, 0.45f * height );
    that.set_font( &font_c[ 0 ], static_cast< int >( font_c.size() ), 0.2f * height );
    that.fill_text( "CE\xc3\x8d\xf4\x8f\xbf\xbd\xf0I", 0.0, 0.70f * height );
    that.set_color( fill_style, 1.0f, 0.0f, 0.0f, 1.0f );
    that.set_font( &font_d[ 0 ], static_cast< int >( font_d.size() ), 0.2f * height );
    that.fill_text( "D", 0.1f * width, 0.95f * height );
    that.set_font( &font_e[ 0 ], static_cast< int >( font_e.size() ), 0.2f * height );
    that.fill_text( "D", 0.2f * width, 0.95f * height );
    that.set_font( &font_f[ 0 ], static_cast< int >( font_f.size() ), 0.2f * height );
    that.fill_text( "D", 0.3f * width, 0.95f * height );
    that.set_font( &font_g[ 0 ], static_cast< int >( font_g.size() ), 0.2f * height );
    that.fill_text( "D", 0.4f * width, 0.95f * height );
}

void fill_text( canvas &that, float width, float height )
{
    that.set_linear_gradient( fill_style, 0.4f * width, 0.0f, 0.6f * width, 0.0f );
    that.add_color_stop( fill_style, 0.00f, 0.0f, 0.00f, 1.0f, 1.0f );
    that.add_color_stop( fill_style, 0.45f, 0.0f, 0.25f, 0.5f, 1.0f );
    that.add_color_stop( fill_style, 0.50f, 1.0f, 0.00f, 0.0f, 1.0f );
    that.add_color_stop( fill_style, 0.55f, 0.0f, 0.25f, 0.5f, 1.0f );
    that.add_color_stop( fill_style, 1.00f, 0.0f, 0.50f, 0.0f, 1.0f );
    that.set_font( &font_a[ 0 ], static_cast< int >( font_a.size() ), 0.3f * height );
    that.rotate( 0.2f );
    that.shadow_offset_x = 2.0f;
    that.shadow_offset_y = 2.0f;
    that.set_shadow_blur( 4.0f );
    that.set_shadow_color( 0.0f, 0.0f, 0.0f, 0.75f );
    that.move_to( 0.0f, 0.2f * height );
    that.fill_text( "Canvas", 0.1f * width, 0.2f * height );
    that.line_to( width, 0.2f * height );
    that.fill_text( "Ity\n*", 0.2f * width, 0.5f * height, width );
    that.move_to( 0.0f, 0.5f * height );
    that.fill_text( "*Canvas\fIty*", 0.2f * width, 0.8f * height, 0.7f * width );
    that.set_color( fill_style, 1.0f, 0.0f, 0.0f, 1.0f );
    that.fill_text( "****", 0.1f * width, 0.35f * height, 0.0f );
    that.line_to( width, 0.5f * height );
    that.set_shadow_color( 0.0f, 0.0f, 0.0f, 0.0f );
    that.set_color( stroke_style, 1.0f, 0.0f, 0.0f, 1.0f );
    that.set_line_width( 2.0f );
    that.stroke();
}

void stroke_text( canvas &that, float width, float height )
{
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
    that.set_font( &font_a[ 0 ], static_cast< int >( font_a.size() ), 0.3f * height );
    that.rotate( 0.2f );
    that.set_line_width( 2.0f );
    float segments[] = { 8.0f, 2.0f };
    that.set_line_dash( segments, 2 );
    that.move_to( 0.0f, 0.2f * height );
    that.stroke_text( "Canvas", 0.1f * width, 0.2f * height );
    that.line_to( width, 0.2f * height );
    that.stroke_text( "Ity\n*", 0.2f * width, 0.5f * height, width );
    that.move_to( 0.0f, 0.5f * height );
    that.stroke_text( "*Canvas\fIty*", 0.2f * width, 0.8f * height, 0.7f * width );
    that.set_color( stroke_style, 1.0f, 0.0f, 0.0f, 1.0f );
    that.stroke_text( "****", 0.1f * width, 0.35f * height, 0.0f );
    that.line_to( width, 0.5f * height );
    that.set_line_dash( 0, 0 );
    that.stroke();
}

void measure_text( canvas &that, float width, float height )
{
    that.set_color( fill_style, 0.0f, 0.0f, 0.0f, 1.0f );
    float place = 0.1f * width;
    place += that.measure_text( "C" );
    that.set_font( &font_a[ 0 ], static_cast< int >( font_a.size() ), 0.3f * height );
    that.rotate( 0.5f );
    that.scale( 1.15f, 1.0f );
    that.fill_text( "C", place, 0.2f * height );
    place += that.measure_text( "C" );
    that.fill_text( "a", place, 0.25f * height );
    place += that.measure_text( "a" );
    that.fill_text( "nv", place, 0.2f * height );
    place += that.measure_text( "nv" );
    that.fill_text( "a", place, 0.15f * height );
    place += that.measure_text( "a" );
    that.fill_text( "s", place, 0.2f * height );
}

void draw_image( canvas &that, float width, float height )
{
    unsigned char checker[ 1024 ];
    for ( int index = 0; index < 1024; ++index )
        checker[ index ] = static_cast< unsigned char >(
            ( ( ( index >> 2 & 1 ) ^ ( index >> 6 & 1 ) ) |
              ( ( index & 3 ) == 3 ) ) * 255 );
    that.draw_image( checker, 16, 16, 64,
                     0.0f, 0.0f, width * 0.75f, height * 0.75f );
    for ( float row = 0.0f; row < 4.0f; row += 1.0f )
        for ( float column = 0.0f; column < 4.0f; column += 1.0f )
            that.draw_image( checker, 16, 16, 64,
                             column * 17.25f, row * 17.25f, 16.0f, 16.0f );
    that.draw_image( checker, 16, 16, 64, 128.0f, 0.0f, 32.0f, 8.0f );
    that.draw_image( checker, 16, 16, 64, 128.0f, 48.0f, 32.0f, -32.0f );
    that.draw_image( checker, 16, 16, 64, 200.0f, 16.0f, -32.0f, 32.0f );
    that.draw_image( checker, 16, 16, 64, 128.0f, 64.0f, 32.0f, 0.0f );
    that.draw_image( 0, 16, 16, 64, 200.0f, 64.0f, 32.0f, 32.0f );
    unsigned char pixel[] = { 0, 255, 0, 255 };
    that.draw_image( pixel, 1, 1, 4,
                     width * 0.875f, height * 0.25f, 1.0f, 1.0f );
    that.draw_image( pixel, 1, 1, 4,
                     width * 0.875f, height * 0.5f, 16.0f, 16.0f );
    that.rotate( 0.2f );
    that.global_composite_operation = lighter;
    that.set_global_alpha( 1.0f );
    that.draw_image( checker, 16, 16, 64,
                     0.25f * width, 0.25f * height,
                     0.5f * width, 0.5f * height );
}

void draw_image_matted( canvas &that, float width, float height )
{
    that.set_color( fill_style, 0.0f, 1.0f, 0.0f, 0.0f );
    that.fill_rectangle( 0.0f, 0.0f, width, height );
    unsigned char checker[ 36 ] = {
        0, 0, 255, 255,  255, 0, 0, 0,    0, 0, 255, 255,
        255, 0, 0, 0,    0, 0, 255, 255,  255, 0, 0, 0,
        0, 0, 255, 255,  255, 0, 0, 0,    0, 0, 255, 255,
    };
    float y = 0.5f;
    float size_y = 3.0f;
    for ( int step_y = 0; step_y < 20 && y < height; ++step_y )
    {
        float x = 0.5f;
        float size_x = 3.0f;
        for ( int step_x = 0; step_x < 20 && x < width; ++step_x )
        {
            that.draw_image( checker, 3, 3, 12, x, y, size_x, size_y );
            x += size_x + 5.0f;
            size_x *= 1.5f;
        }
        y += size_y + 5.0f;
        size_y *= 1.5f;
    }
}

void get_image_data( canvas &that, float width, float height )
{
    for ( int index = 0; index < 100; ++index )
    {
        that.set_color( fill_style,
                        static_cast< float >( index /  2 % 2 ),
                        static_cast< float >( index /  4 % 2 ),
                        static_cast< float >( index /  8 % 2 ),
                        static_cast< float >( index / 16 % 2 ) );
        that.fill_rectangle( 3.0f * static_cast< float >( index % 10 ),
                             3.0f * static_cast< float >( index / 10 ),
                             3.0f, 3.0f );
    }
    unsigned char data[ 4939 ];
    data[ 0 ] = 150;
    for ( int index = 1; index < 4939; ++index )
        data[ index ] = static_cast< unsigned char >(
            ( data[ index - 1 ] * 137 + 53 ) & 255 );
    that.get_image_data( data + 2, 35, 35, 141, -10, -10 );
    unsigned hash = 0;
    for ( int index = 0; index < 4939; ++index )
        hash = ( ( ( hash & 0x1ffff ) << 15 ) | ( hash >> 17 ) ) ^ data[ index ];
    unsigned const expected = 0xf53f9792;
    that.set_color( fill_style, hash != expected, hash == expected, 0.0f, 1.0f );
    that.fill_rectangle( 30.0f, 0.0f, width, 30.0f );
    that.set_linear_gradient( fill_style, 0.0f, 0.0f, width, 0.0f );
    that.add_color_stop( fill_style, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f );
    that.add_color_stop( fill_style, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f );
    that.fill_rectangle( 0.0f, 0.45f * height, width, 0.1f * height );
    that.get_image_data( 0, 32, 32, 128, 0, 0 );
}

void put_image_data( canvas &that, float width, float height )
{
    unsigned char checker[ 2052 ];
    for ( int index = 0; index < 2048; ++index )
        checker[ index + 2 ] = static_cast< unsigned char >(
            ( ( ( ( index >> 2 & 1 ) ^ ( index >> 7 & 1 ) ) |
                ( ( index & 3 ) == 3 ) ) &
              ( index >> 10 & 1 ) ) * 255 );
    checker[ 0 ] = 157;
    checker[ 1 ] = 157;
    checker[ 2050 ] = 157;
    checker[ 2051 ] = 157;
    that.set_color( fill_style, 0.4f, 0.05f, 0.2f, 1.0f );
    that.fill_rectangle( 0.0f, 0.0f, 0.25f * width, 0.25f * height );
    that.set_global_alpha( 0.5f );
    that.global_composite_operation = lighter;
    that.rotate( 0.2f );
    for ( int y = -10; y < static_cast< int >( height ); y += 29 )
        for ( int x = -10; x < static_cast< int >( width ); x += 29 )
            that.put_image_data( checker + 6, 16, 16, 128, x, y );
    that.put_image_data( 0, 32, 32, 128, 0, 0 );
}

void save_restore( canvas &that, float width, float height )
{
    that.rectangle( width * 0.25f, height * 0.25f,
                    width * 0.25f, height * 0.25f );
    that.set_color( stroke_style, 0.0f, 0.0f, 1.0f, 1.0f );
    that.set_line_width( 8.0f );
    that.save();
    that.clip();
    that.begin_path();
    that.rectangle( width * 0.25f, height * 0.25f,
                    width * 0.5f, height * 0.5f );
    that.set_color( stroke_style, 1.0f, 0.0f, 0.0f, 1.0f );
    that.set_line_width( 1.0f );
    that.restore();
    that.restore();
    that.stroke();
    that.save();
    that.save();
}

void example_button( canvas &that, float width, float height )
{
    float left = roundf( 0.25f * width );
    float right = roundf( 0.75f * width );
    float top = roundf( 0.375f * height );
    float bottom = roundf( 0.625f * height );
    float mid_x = ( left + right ) * 0.5f;
    float mid_y = ( top + bottom ) * 0.5f;
    that.shadow_offset_x = 3.0f;
    that.shadow_offset_y = 3.0f;
    that.set_shadow_blur( 3.0f );
    that.set_shadow_color( 0.0f, 0.0f, 0.0f, 0.5f );
    that.set_linear_gradient( fill_style, 0.0f, top, 0.0f, bottom );
    that.add_color_stop( fill_style, 0.0f, 0.3f, 0.3f, 0.3f, 1.0f );
    that.add_color_stop( fill_style, 1.0f, 0.2f, 0.2f, 0.2f, 1.0f );
    that.move_to( left + 0.5f, mid_y );
    that.arc_to( left + 0.5f, top + 0.5f, mid_x, top + 0.5f, 4.0f );
    that.arc_to( right - 0.5f, top + 0.5f, right - 0.5f, mid_y, 4.0f );
    that.arc_to( right - 0.5f, bottom - 0.5f, mid_x, bottom - 0.5f, 4.0f );
    that.arc_to( left + 0.5f, bottom - 0.5f, left + 0.5f, mid_y, 4.0f );
    that.close_path();
    that.fill();
    that.set_shadow_color( 0.0f, 0.0f, 0.0f, 0.0f );
    that.set_font( &font_a[ 0 ], static_cast< int >( font_a.size() ), 0.075f * height );
    that.text_align = center;
    that.text_baseline = middle;
    that.set_color( fill_style, 0.8f, 0.8f, 0.8f, 1.0f );
    that.fill_text( "* Cats", 0.5f * width, 0.5f * height );
    that.set_color( fill_style, 0.4f, 0.4f, 0.4f, 1.0f );
    that.fill_rectangle( left + 4.0f, top + 1.0f, right - left - 8.0f, 1.0f );
    that.set_color( stroke_style, 0.1f, 0.1f, 0.1f, 1.0f );
    that.stroke();
}

void example_smiley( canvas &that, float width, float height )
{
    float center_x = 0.5f * width;
    float center_y = 0.5f * height;
    float radius = min( width, height ) * 0.4f;
    that.set_radial_gradient( fill_style,
                              center_x, center_y, 0.0f,
                              center_x, center_y, radius );
    that.add_color_stop( fill_style, 0.0f, 1.0f, 0.9f, 0.2f, 1.0f );
    that.add_color_stop( fill_style, 0.95f, 0.95f, 0.65f, 0.15f, 1.0f );
    that.add_color_stop( fill_style, 1.0f, 0.9f, 0.55f, 0.0f, 1.0f );
    that.arc( center_x, center_y, radius, 0.0f, 6.28318531f );
    that.fill();
    that.set_linear_gradient( fill_style,
                              center_x, center_y - 0.95f * radius,
                              center_x, center_y );
    that.add_color_stop( fill_style, 0.0f, 1.0f, 1.0f, 1.0f, 0.5f );
    that.add_color_stop( fill_style, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f );
    that.begin_path();
    that.arc( center_x, center_y - 0.15f * radius, 0.8f * radius,
              0.0f, 6.28318531f );
    that.fill();
    that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 0.95f );
    that.set_line_width( 0.2f * radius );
    that.line_cap = circle;
    that.begin_path();
    that.move_to( center_x - 0.2f * radius, center_y - 0.5f * radius );
    that.line_to( center_x - 0.2f * radius, center_y - 0.2f * radius );
    that.move_to( center_x + 0.2f * radius, center_y - 0.5f * radius );
    that.line_to( center_x + 0.2f * radius, center_y - 0.2f * radius );
    that.stroke();
    that.set_color( fill_style, 0.0f, 0.0f, 0.0f, 0.95f );
    that.begin_path();
    that.move_to( center_x - 0.6f * radius, center_y + 0.1f * radius );
    that.bezier_curve_to( center_x - 0.3f * radius, center_y + 0.8f * radius,
                          center_x + 0.3f * radius, center_y + 0.8f * radius,
                          center_x + 0.6f * radius, center_y + 0.1f * radius);
    that.bezier_curve_to( center_x + 0.3f * radius, center_y + 0.3f * radius,
                          center_x - 0.3f * radius, center_y + 0.3f * radius,
                          center_x - 0.6f * radius, center_y + 0.1f * radius);
    that.fill();
}

void example_knot( canvas &that, float width, float height )
{
    float points[ 6 ][ 8 ] = {
        {  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f },
        { -1.0f, -1.0f, -1.0f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f },
        {  2.0f,  1.0f,  2.0f, -2.0f, -1.0f, -2.0f, -1.0f, -1.0f },
        { -2.0f, -1.0f, -2.0f,  2.0f,  1.0f,  2.0f,  1.0f,  1.0f },
        { -2.0f, -1.0f, -2.0f, -3.0f,  0.0f, -3.0f,  0.0f, -1.0f },
        {  2.0f,  1.0f,  2.0f,  3.0f,  0.0f,  3.0f,  0.0f,  1.0f },
    };
    that.translate( width * 0.5f, height * 0.5f );
    that.scale( width * 0.17f, height * 0.17f );
    that.rotate( -15.0f * 3.14159265f / 180.0f );
    for ( int index = 0; index < 6; ++index )
    {
        that.begin_path();
        that.move_to(
            1.01f * points[ index ][ 0 ] - 0.01f * points[ index ][ 2 ],
            1.01f * points[ index ][ 1 ] - 0.01f * points[ index ][ 3 ] );
        that.line_to( points[ index ][ 0 ], points[ index ][ 1 ] );
        that.bezier_curve_to( points[ index ][ 2 ], points[ index ][ 3 ],
                              points[ index ][ 4 ], points[ index ][ 5 ],
                              points[ index ][ 6 ], points[ index ][ 7 ] );
        that.line_to(
            -0.01f * points[ index ][ 4 ] + 1.01f * points[ index ][ 6 ],
            -0.01f * points[ index ][ 5 ] + 1.01f * points[ index ][ 7 ] );
        that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
        that.set_line_width( 0.75f );
        that.line_cap = butt;
        that.stroke();
        that.set_radial_gradient( stroke_style,
                                  0.0f, 0.0f, 0.0f,
                                  0.0f, 0.0f, 3.0f );
        that.add_color_stop( stroke_style, 0.0f, 0.8f, 1.0f, 0.6f, 1.0f );
        that.add_color_stop( stroke_style, 1.0f, 0.1f, 0.5f, 0.1f, 1.0f );
        that.set_line_width( 0.5f );
        that.line_cap = circle;
        that.stroke();
    }
}

void example_icon( canvas &that, float width, float height )
{
    that.set_shadow_color( 0.0f, 0.0f, 0.0f, 1.0f );
    that.shadow_offset_x = width / 64.0f;
    that.shadow_offset_y = height / 64.0f;
    that.set_shadow_blur( std::min( width, height ) / 32.0f );
    that.scale( width / 32.0f, height / 32.0f );
    that.set_color( fill_style, 0.4f, 0.05f, 0.2f, 1.0f );
    that.move_to( 15.5f, 1.0f );
    that.arc_to( 30.0f, 1.0f, 30.0f, 15.5f, 6.0f );
    that.arc_to( 30.0f, 30.0f, 15.5f, 30.0f, 6.0f );
    that.arc_to( 1.0f, 30.0f, 1.0f, 15.5f, 6.0f );
    that.arc_to( 1.0f, 1.0f, 15.5f, 1.0f, 6.0f );
    that.fill();
    that.set_color( stroke_style, 0.5f, 0.5f, 0.5f, 1.0f );
    that.begin_path();
    that.move_to( 11.0f, 16.0f );
    that.line_to( 27.0f, 16.0f );
    that.move_to( 2.0f, 23.0f );
    that.line_to( 29.0f, 23.0f );
    that.stroke();
    that.set_color( stroke_style, 0.75f, 0.75f, 0.75f, 1.0f );
    that.begin_path();
    that.arc( 25.0f, 22.0f, 0.5f, 0.0f, 6.28318531f );
    that.move_to( 19.0f, 6.0f );
    that.line_to( 18.5f, 8.0f );
    that.move_to( 20.0f, 6.0f );
    that.line_to( 20.0f, 8.0f );
    that.move_to( 21.0f, 6.0f );
    that.line_to( 21.5f, 8.0f );
    that.move_to( 17.0f, 14.0f );
    that.line_to( 16.0f, 18.0f );
    that.move_to( 20.0f, 14.0f );
    that.line_to( 20.0f, 18.0f );
    that.move_to( 23.0f, 14.0f );
    that.line_to( 24.0f, 18.0f );
    that.move_to( 18.0f, 9.0f );
    that.line_to( 22.0f, 9.0f );
    that.move_to( 18.0f, 13.0f );
    that.line_to( 22.0f, 13.0f );
    that.rectangle( 16.0f, 8.0f, 8.0f, 6.0f );
    that.stroke();
    that.set_color( stroke_style, 1.0f, 1.0f, 1.0f, 1.0f );
    that.begin_path();
    that.arc( 19.0f, 12.0f, 9.0f, 0.0f, 6.28318531f );
    that.move_to( 12.3f, 17.3f );
    that.line_to( 3.3f, 26.3f );
    that.move_to( 13.0f, 18.0f );
    that.line_to( 4.0f, 27.0f );
    that.move_to( 13.7f, 18.7f );
    that.line_to( 4.7f, 27.7f );
    that.stroke();
}

void example_illusion( canvas &that, float width, float height )
{
    that.set_color( fill_style, 0.0f, 0.4f, 1.0f, 1.0f );
    that.fill_rectangle( 0.0f, 0.0f, width, height );
    that.set_color( fill_style, 0.8f, 0.8f, 0.0f, 1.0f );
    that.set_line_width( 0.4f );
    for ( float spot = 0.0f; spot < 240.0f; spot += 1.0f )
    {
        float angle = fmodf( spot * 0.61803398875f, 1.0f ) * 6.28318531f;
        float radius = spot / 240.0f * 0.5f * hypotf( width, height );
        float size = min( width, height ) * sqrtf( spot ) / 240.0f;
        that.set_transform( 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f );
        that.translate( 0.5f * width + radius * cosf( angle ),
                        0.5f * height + radius * sinf( angle ) );
        that.rotate( angle - 1.3f );
        that.scale( 0.8f * size, 0.6f * size );
        that.rotate( 1.3f );
        that.begin_path();
        that.arc( 0.0f, 0.0f, 1.0f, 0.0f, 6.28318531f );
        that.fill();
        that.begin_path();
        that.arc( 0.0f, 0.0f, 1.0f, 0.0f, 3.14159265f );
        that.set_color( stroke_style, 1.0f, 1.0f, 1.0f, 1.0f );
        that.stroke();
        that.begin_path();
        that.arc( 0.0f, 0.0f, 1.0f, 3.14159265f, 6.28318531f );
        that.set_color( stroke_style, 0.0f, 0.0f, 0.0f, 1.0f );
        that.stroke();
    }
}

void example_star( canvas &that, float width, float height )
{
    that.scale( width / 256.0f, height / 256.0f );
    that.move_to( 128.0f, 28.0f );
    that.line_to( 157.0f, 87.0f );
    that.line_to( 223.0f, 97.0f );
    that.line_to( 175.0f, 143.0f );
    that.line_to( 186.0f, 208.0f );
    that.line_to( 128.0f, 178.0f );
    that.line_to( 69.0f, 208.0f );
    that.line_to( 80.0f, 143.0f );
    that.line_to( 32.0f, 97.0f );
    that.line_to( 98.0f, 87.0f );
    that.close_path();
    that.set_shadow_blur( 8.0f );
    that.shadow_offset_y = 4.0f;
    that.set_shadow_color( 0.0f, 0.0f, 0.0f, 0.5f );
    that.set_color( fill_style, 1.0f, 0.9f, 0.2f, 1.0f );
    that.fill();
    that.line_join = rounded;
    that.set_line_width( 12.0f );
    that.set_color( stroke_style, 0.9f, 0.0f, 0.5f, 1.0f );
    that.stroke();
    float segments[] = { 21.0f, 9.0f, 1.0f, 9.0f, 7.0f, 9.0f, 1.0f, 9.0f };
    that.set_line_dash( segments, 8 );
    that.line_dash_offset = 10.0f;
    that.line_cap = circle;
    that.set_line_width( 6.0f );
    that.set_color( stroke_style, 0.95f, 0.65f, 0.15f, 1.0f );
    that.stroke();
    that.set_shadow_color( 0.0f, 0.0f, 0.0f, 0.0f );
    that.set_linear_gradient( fill_style, 64.0f, 0.0f, 192.0f, 256.0f );
    that.add_color_stop( fill_style, 0.30f, 1.0f, 1.0f, 1.0f, 0.0f );
    that.add_color_stop( fill_style, 0.35f, 1.0f, 1.0f, 1.0f, 0.8f );
    that.add_color_stop( fill_style, 0.45f, 1.0f, 1.0f, 1.0f, 0.8f );
    that.add_color_stop( fill_style, 0.50f, 1.0f, 1.0f, 1.0f, 0.0f );
    that.global_composite_operation = source_atop;
    that.fill_rectangle( 0.0f, 0.0f, 256.0f, 256.0f );
}

void example_neon( canvas &that, float width, float height )
{
    that.scale( width / 256.0f, height / 256.0f );
    that.set_color( fill_style, 0.0f, 0.0625f, 0.125f, 1.0f );
    that.fill_rectangle( 0.0f, 0.0f, 256.0f, 256.0f );
    that.move_to( 45.5f, 96.2f );
    that.bezier_curve_to( 45.5f, 96.2f, 31.3f, 106.2f, 31.5f, 113.1f );
    that.bezier_curve_to( 31.7f, 119.5f, 50.6f, 104.8f, 50.6f, 93.9f );
    that.bezier_curve_to( 50.6f, 91.1f, 46.6f, 89.1f, 43.3f, 89.4f );
    that.bezier_curve_to( 27.5f, 90.6f, 8.5f, 108.2f, 8.8f, 121.8f );
    that.bezier_curve_to( 9.1f, 133.1f, 21.3f, 136.6f, 29.8f, 136.3f );
    that.bezier_curve_to( 52.4f, 135.5f, 62.3f, 115.6f, 62.3f, 115.6f );
    that.move_to( 81.0f, 120.2f );
    that.bezier_curve_to( 81.0f, 120.2f, 60.2f, 123.0f, 59.7f, 130.8f );
    that.bezier_curve_to( 59.2f, 140.6f, 73.8f, 136.4f, 78.3f, 125.3f );
    that.move_to( 80.7f, 130.5f );
    that.bezier_curve_to( 79.5f, 132.4f, 80.9f, 135.0f, 83.4f, 135.0f );
    that.bezier_curve_to( 95.8f, 135.6f, 99.3f, 122.5f, 111.4f, 121.6f );
    that.bezier_curve_to( 112.8f, 121.5f, 114.0f, 123.0f, 114.0f, 124.3f );
    that.bezier_curve_to( 113.9f, 126.1f, 106.7f, 133.9f, 106.7f, 133.9f );
    that.move_to( 118.5f, 122.9f );
    that.bezier_curve_to( 118.5f, 122.9f, 122.1f, 118.8f, 126.1f, 122.0f );
    that.bezier_curve_to( 131.4f, 126.4f, 118.7f, 131.6f, 124.3f, 134.7f );
    that.bezier_curve_to( 130.0f, 137.8f, 150.0f, 116.5f, 156.0f, 120.2f );
    that.bezier_curve_to( 160.2f, 122.8f, 149.0f, 133.5f, 155.6f, 133.6f );
    that.bezier_curve_to( 162.0f, 133.4f, 173.8f, 118.3f, 168.0f, 117.8f );
    that.move_to( 173.1f, 123.2f );
    that.bezier_curve_to( 177.8f, 124.8f, 182.8f, 123.2f, 187.0f, 119.7f );
    that.move_to( 206.1f, 118.6f );
    that.bezier_curve_to( 206.1f, 118.6f, 185.3f, 121.3f, 185.1f, 129.1f );
    that.bezier_curve_to( 185.0f, 138.7f, 199.9f, 135.4f, 203.6f, 123.6f );
    that.move_to( 205.6f, 129.9f );
    that.bezier_curve_to( 204.4f, 131.8f, 205.8f, 134.4f, 208.3f, 134.4f );
    that.bezier_curve_to( 220.3f, 134.4f, 246.6f, 117.1f, 246.6f, 117.1f );
    that.move_to( 247.0f, 122.4f );
    that.bezier_curve_to( 245.9f, 128.5f, 243.9f, 139.7f, 231.2f, 131.5f );
    that.line_cap = circle;
    that.set_shadow_color( 1.0f, 0.5f, 0.0f, 1.0f );
    that.set_shadow_blur( 20.0f );
    that.set_line_width( 4.0f );
    that.set_color( stroke_style, 1.0f, 0.5f, 0.0f, 1.0f );
    that.stroke();
    that.set_shadow_blur( 5.0f );
    that.set_line_width( 3.0f );
    that.set_color( stroke_style, 1.0f, 0.625f, 0.0f, 1.0f );
    that.stroke();
}

}

// ======== TEST HARNESS ========

// This is the table of tests to run.  To add a new test to the suite, write a
// function for it above with the same function signature as the other tests,
// and then register it here.  Just use zero initially for the expected hash;
// the test will fail, but it will report the hash that it produced and that
// can then be put in here.  Alternately, run the program with --table to
// recompute hashes and output them in a form suitable for inserting here.
// Note that for computing expected hashes, this test program should be
// compiled with all optimizations disabled (e.g., -O0)!
//
struct test
{
    unsigned hash;
    int width, height;
    void ( *call )( canvas &, float, float );
    char const *name;
} const tests[] = {
    { 0xc99ddee7, 256, 256, scale_uniform, "scale_uniform" },
    { 0xe93d3c6f, 256, 256, scale_non_uniform, "scale_non_uniform" },
    { 0x05a0e377, 256, 256, rotate, "rotate" },
    { 0x36e7fa56, 256, 256, translate, "translate" },
    { 0xcfae3e4f, 256, 256, transform, "transform" },
    { 0x98f5594a, 256, 256, transform_fill, "transform_fill" },
    { 0x822964b0, 256, 256, transform_stroke, "transform_stroke" },
    { 0xb7056a3a, 256, 256, set_transform, "set_transform" },
    { 0x8f6dd6c3, 256, 256, global_alpha, "global_alpha" },
    { 0x98a0609d, 256, 256, global_composite_operation, "global_composite_operation" },
    { 0x9def5b00, 256, 256, shadow_color, "shadow_color" },
    { 0x8294edd8, 256, 256, shadow_offset, "shadow_offset" },
    { 0xcdeba51c, 256, 256, shadow_offset_offscreen, "shadow_offset_offscreen" },
    { 0x5b542224, 256, 256, shadow_blur, "shadow_blur" },
    { 0xd6c150e6, 256, 256, shadow_blur_offscreen, "shadow_blur_offscreen" },
    { 0x5affc092, 256, 256, shadow_blur_composite, "shadow_blur_composite" },
    { 0x1720e9b2, 256, 256, line_width, "line_width" },
    { 0xf8d2bb0d, 256, 256, line_width_angular, "line_width_angular" },
    { 0x7bda8673, 256, 256, line_cap, "line_cap" },
    { 0x53639198, 256, 256, line_cap_offscreen, "line_cap_offscreen" },
    { 0x8f49c41d, 256, 256, line_join, "line_join" },
    { 0xca27ce8c, 256, 256, line_join_offscreen, "line_join_offscreen" },
    { 0xe68273e2, 256, 256, miter_limit, "miter_limit" },
    { 0x27c38a8a, 256, 256, line_dash_offset, "line_dash_offset" },
    { 0x129f9595, 256, 256, line_dash, "line_dash" },
    { 0x88a74152, 256, 256, line_dash_closed, "line_dash_closed" },
    { 0x064f194d, 256, 256, line_dash_overlap, "line_dash_overlap" },
    { 0xf7259c0f, 256, 256, line_dash_offscreen, "line_dash_offscreen" },
    { 0xeb4338e8, 256, 256, color, "color" },
    { 0x6dc35a07, 256, 256, linear_gradient, "linear_gradient" },
    { 0x418fe678, 256, 256, radial_gradient, "radial_gradient" },
    { 0x67aada11, 256, 256, color_stop, "color_stop" },
    { 0xc6c721d6, 256, 256, pattern, "pattern" },
    { 0xb0b391cd, 256, 256, begin_path, "begin_path" },
    { 0xf79ed394, 256, 256, move_to, "move_to" },
    { 0xe9602309, 256, 256, close_path, "close_path" },
    { 0x3160ace7, 256, 256, line_to, "line_to" },
    { 0xb6176812, 256, 256, quadratic_curve_to, "quadratic_curve_to" },
    { 0x5f523029, 256, 256, bezier_curve_to, "bezier_curve_to" },
    { 0x1f847aaf, 256, 256, arc_to, "arc_to" },
    { 0x26457553, 256, 256, arc, "arc" },
    { 0x7520990c, 256, 256, rectangle, "rectangle" },
    { 0xf1d774dc, 256, 256, fill, "fill" },
    { 0x5e6e6b75, 256, 256, fill_rounding, "fill_rounding" },
    { 0xf0cf6566, 256, 256, fill_converging, "fill_converging" },
    { 0x3692d10e, 256, 256, fill_zone_plate, "fill_zone_plate" },
    { 0x2003f926, 256, 256, stroke, "stroke" },
    { 0xc44fc157, 256, 256, stroke_wide, "stroke_wide" },
    { 0x691cfe49, 256, 256, stroke_inner_join, "stroke_inner_join" },
    { 0xc0bd9324, 256, 256, stroke_spiral, "stroke_spiral" },
    { 0x3b2dae15, 256, 256, stroke_long, "stroke_long" },
    { 0xa7e06559, 256, 256, clip, "clip" },
    { 0x31e6112b, 256, 256, clip_winding, "clip_winding" },
    { 0xc2188d67, 256, 256, is_point_in_path, "is_point_in_path" },
    { 0x6505bdc9, 256, 256, is_point_in_path_offscreen, "is_point_in_path_offscreen" },
    { 0x5e792c96, 256, 256, clear_rectangle, "clear_rectangle" },
    { 0x286e96fa, 256, 256, fill_rectangle, "fill_rectangle" },
    { 0xc2b0803d, 256, 256, stroke_rectangle, "stroke_rectangle" },
    { 0xe6c4d9c7, 256, 256, text_align, "text_align" },
    { 0x72cb6b06, 256, 256, text_baseline, "text_baseline" },
    { 0x4d41daa2, 256, 256, font, "font" },
    { 0x70e3232d, 256, 256, fill_text, "fill_text" },
    { 0xed6477c8, 256, 256, stroke_text, "stroke_text" },
    { 0x32d1ee3b, 256, 256, measure_text, "measure_text" },
    { 0x78cb460c, 256, 256, draw_image, "draw_image" },
    { 0xb530077b, 256, 256, draw_image_matted, "draw_image_matted" },
    { 0xaf04e7a2, 256, 256, get_image_data, "get_image_data" },
    { 0x5acae0b6, 256, 256, put_image_data, "put_image_data" },
    { 0xb6e854b1, 256, 256, save_restore, "save_restore" },
    { 0x62bc9606, 256, 256, example_button, "example_button" },
    { 0x92731a7b, 256, 256, example_smiley, "example_smiley" },
    { 0xe2f1e1de, 256, 256, example_knot, "example_knot" },
    { 0xc02d01ea, 256, 256, example_icon, "example_icon" },
    { 0xa1607c4a, 256, 256, example_illusion, "example_illusion" },
    { 0x7c861f87, 256, 256, example_star, "example_star" },
    { 0x429ca194, 256, 256, example_neon, "example_neon" },
};

// Simple glob style string matcher.  This accepts both * and ? glob
// characters.  It potentially has exponential run time, but as it is only
// used for matching against the names of tests, this is acceptable.
//
bool glob_match(
    char const *pattern,
    char const *name )
{
    if ( !*pattern && !*name )
        return true;
    if ( *pattern == '*' )
        return glob_match( pattern + 1, name ) ||
            ( *name && glob_match( pattern, name + 1 ) );
    if ( *pattern == '?' )
        return *name && glob_match( pattern + 1, name + 1 );
    return *name == *pattern && glob_match( pattern + 1, name + 1 );
}

// Simple Base64 decoder.  This is used at startup to decode the string
// literals containing embedded resource data, namely font files in TTF form.
//
void base64_decode(
    char const *input,
    vector< unsigned char > &output )
{
    int index = 0;
    int data = 0;
    int held = 0;
    while ( int symbol = input[ index++ ] )
    {
        if ( symbol == '=' )
            break;
        int value = ( 'A' <= symbol && symbol <= 'Z' ? symbol - 'A' :
                      'a' <= symbol && symbol <= 'z' ? symbol - 'a' + 26 :
                      '0' <= symbol && symbol <= '9' ? symbol - '0' + 52 :
                      symbol == '+' ? 62 :
                      symbol == '/' ? 63 :
                      0 );
        data = data << 6 | value;
        held += 6;
        if ( held >= 8 )
        {
            held -= 8;
            output.push_back( static_cast< unsigned char >( ( data >> held ) & 0xff ) );
            data &= ( 1 << held ) - 1;
        }
    }
}

// Time in seconds since an arbitrary point.  This is only used for the
// relative difference between the values before and after a test runs, so the
// starting point does not particularly matter as long as it is consistent.
//
double get_seconds()
{
#if defined( __linux__ )
    timespec now;
    clock_gettime( CLOCK_MONOTONIC, &now );
    return ( static_cast< double >( now.tv_sec ) +
             static_cast< double >( now.tv_nsec ) * 1.0e-9 );
#elif defined( _WIN32 )
    static double rate = 0.0;
    if ( rate == 0.0 )
    {
        static LARGE_INTEGER frequency;
        QueryPerformanceFrequency( &frequency );
        rate = 1.0 / frequency.QuadPart;
    }
    LARGE_INTEGER now;
    QueryPerformanceCounter( &now );
    return now.QuadPart * rate;
#elif defined( __MACH__ )
    static double rate = 0.0;
    if ( rate == 0.0 )
    {
        static mach_timebase_info_data_t frequency;
        mach_timebase_info( &frequency );
        rate = frequency.numer * 1.0e-9 / frequency.denom;
    }
    return mach_absolute_time() * rate;
#else
    timeval now;
    gettimeofday( &now, 0 );
    return now.tv_sec + now.tv_usec * 1.0e-6;
#endif
}

// Generate a 32-bit hash for an RGBA8 image.  This is a custom image hash
// function that is somewhere between a checksum/cryptographic/CRC style
// hash that detects bit-level differences, and a typical perceptual image
// hash (e.g., dhash) that matches images even with substantial alterations.
// It is inspired by locality sensitive hashing techniques and is designed
// to be tolerant of small pixel variations produced by numeric differences
// from moderately aggressive compiler optimizations, while at the same time
// detecting color changes or pixel-sized shifts.  For each channel within a
// pixel, it compares the value against its neighboring pixel to the right
// and down (with wrapping) to check for edge crossings.  Depending on the
// strength and direction of the edge, it may then toggle some bits within
// a group at a pseudorandom position within the hash.  The hashes can then
// be compared by their Hamming distance.
//
unsigned hash_image(
    unsigned char const *image,
    int width,
    int height )
{
    unsigned hash = 0;
    unsigned state = ~0u;
    for ( int y = 0; y < height; ++y )
        for ( int x = 0; x < width; ++x )
            for ( int channel = 0; channel < 4; ++channel )
            {
                int next_x = ( x + 1 ) % width;
                int next_y = ( y + 1 ) % height;
                int current = image[ ( y * width + x ) * 4 + channel ];
                int down = image[ ( next_y * width + x ) * 4 + channel ];
                int right = image[ ( y * width + next_x ) * 4 + channel ];
                int threshold = 8;
                if ( channel < 3 )
                {
                    current *= image[ ( y * width + x ) * 4 + 3 ];
                    down *= image[ ( next_y * width + x ) * 4 + 3 ];
                    right *= image[ ( y * width + next_x ) * 4 + 3 ];
                    threshold *= 255;
                }
                unsigned edges =
                    ( current - down  > threshold * 16 ? 128u : 0u ) |
                    ( current - down  > threshold      ?  64u : 0u ) |
                    ( down - current  > threshold * 16 ?  32u : 0u ) |
                    ( down - current  > threshold      ?  16u : 0u ) |
                    ( current - right > threshold * 16 ?   8u : 0u ) |
                    ( current - right > threshold      ?   4u : 0u ) |
                    ( right - current > threshold * 16 ?   2u : 0u ) |
                    ( right - current > threshold      ?   1u : 0u );
                state ^= ( state & 0x7ffff ) << 13;
                state ^= state >> 17;
                state ^= ( state & 0x7ffffff ) << 5;
                if ( unsigned roll = state >> 27 )
                    edges = ( edges & ( ~0u >> roll ) ) << roll |
                        edges >> ( 32 - roll );
                hash ^= edges;
            }
    return hash;
}

// Simple single-function PNG writer.  This writes perfectly valid,
// though uncompressed, PNG files from sRGBA8 image data, using deflate's
// uncompressed storage mode and wrapping it in a zlib and PNG container.
// There are much simpler formats for RGBA8 images, such as TGA, but support
// for reading the PNG format is nearly universal.
//
void write_png(
    string const &filename,
    unsigned char const *image,
    int width,
    int height )
{
    ofstream output( filename.c_str(), ios::binary );
    unsigned table[ 256 ];
    for ( unsigned index = 0; index < 256; ++index )
    {
        unsigned value = index;
        for ( int step = 0; step < 8; ++step )
            value = ( value & 1 ? 0xedb88320u : 0u ) ^ ( value >> 1 );
        table[ index ] = value;
    }
    int idat_size = 6 + height * ( 6 + width * 4 );
    unsigned char header[] =
    {
        /*  0 */ 137, 80, 78, 71, 13, 10, 26, 10,                 // Signature
        /*  8 */ 0, 0, 0, 13, 73, 72, 68, 82,                     // IHDR
        /* 16 */ static_cast< unsigned char >( width  >> 24 ),
        /* 17 */ static_cast< unsigned char >( width  >> 16 ),
        /* 18 */ static_cast< unsigned char >( width  >>  8 ),
        /* 19 */ static_cast< unsigned char >( width  >>  0 ),
        /* 20 */ static_cast< unsigned char >( height >> 24 ),
        /* 21 */ static_cast< unsigned char >( height >> 16 ),
        /* 22 */ static_cast< unsigned char >( height >>  8 ),
        /* 23 */ static_cast< unsigned char >( height >>  0 ),
        /* 24 */ 8, 6, 0, 0, 0,
        /* 29 */ 0, 0, 0, 0,
        /* 33 */ 0, 0, 0, 1, 115, 82, 71, 66,                     // sRGB
        /* 41 */ 0,
        /* 42 */ 174, 206, 28, 233,
        /* 46 */ static_cast< unsigned char >( idat_size >> 24 ), // IDAT
        /* 47 */ static_cast< unsigned char >( idat_size >> 16 ),
        /* 48 */ static_cast< unsigned char >( idat_size >>  8 ),
        /* 49 */ static_cast< unsigned char >( idat_size >>  0 ),
        /* 50 */ 73, 68, 65, 84,
        /* 54 */ 120, 1,
    };
    unsigned crc = ~0u;
    for ( int index = 12; index < 29; ++index )
        crc = table[ ( crc ^ header[ index ] ) & 0xff ] ^ ( crc >> 8 );
    header[ 29 ] = static_cast< unsigned char >( ~crc >> 24 );
    header[ 30 ] = static_cast< unsigned char >( ~crc >> 16 );
    header[ 31 ] = static_cast< unsigned char >( ~crc >>  8 );
    header[ 32 ] = static_cast< unsigned char >( ~crc >>  0 );
    output.write( reinterpret_cast< char * >( header ), 56 );
    crc = ~0u;
    for ( int index = 50; index < 56; ++index )
        crc = table[ ( crc ^ header[ index ] ) & 0xff ] ^ ( crc >> 8 );
    int check_1 = 1;
    int check_2 = 0;
    int row_size = 1 + width * 4;
    for ( int y = 0; y < height; ++y, image += width * 4 )
    {
        unsigned char prefix[] = {
            /* 0 */ static_cast< unsigned char >( y + 1 == height ),
            /* 1 */ static_cast< unsigned char >(  ( row_size >> 0 ) ),
            /* 2 */ static_cast< unsigned char >(  ( row_size >> 8 ) ),
            /* 3 */ static_cast< unsigned char >( ~( row_size >> 0 ) ),
            /* 4 */ static_cast< unsigned char >( ~( row_size >> 8 ) ),
            /* 5 */ 0,
        };
        output.write( reinterpret_cast< char * >( prefix ), 6 );
        for ( int index = 0; index < 6; ++index )
            crc = table[ ( crc ^ prefix[ index ] ) & 0xff ] ^ ( crc >> 8 );
        output.write( reinterpret_cast< char const * >( image ), width * 4 );
        check_2 = ( check_2 + check_1 ) % 65521;
        for ( int index = 0; index < width * 4; ++index )
        {
            check_1 = ( check_1 + image[ index ] ) % 65521;
            check_2 = ( check_2 + check_1 ) % 65521;
            crc = table[ ( crc ^ image[ index ] ) & 0xff ] ^ ( crc >> 8 );
        }
    }
    unsigned char footer[] = {
        /*  0 */ static_cast< unsigned char >( check_2 >> 8 ),
        /*  1 */ static_cast< unsigned char >( check_2 >> 0 ),
        /*  2 */ static_cast< unsigned char >( check_1 >> 8 ),
        /*  3 */ static_cast< unsigned char >( check_1 >> 0 ),
        /*  4 */ 0, 0, 0, 0,
        /*  8 */ 0, 0, 0, 0, 73, 69, 78, 68,                      // IEND
        /* 16 */ 174, 66, 96, 130,
    };
    for ( int index = 0; index < 4; ++index )
        crc = table[ ( crc ^ footer[ index ] ) & 0xff ] ^ ( crc >> 8 );
    footer[ 4 ] = static_cast< unsigned char >( ~crc >> 24 );
    footer[ 5 ] = static_cast< unsigned char >( ~crc >> 16 );
    footer[ 6 ] = static_cast< unsigned char >( ~crc >>  8 );
    footer[ 7 ] = static_cast< unsigned char >( ~crc >>  0 );
    output.write( reinterpret_cast< char * >( footer ), 20 );
}

// Main test runner.  This parses the command line options, runs the tests,
// and may verify their output, report times and results, and write the images
// the tests produce.
//
int main(
    int argc,
    char **argv )
{
    string subset;
    bool plain = false;
    bool table = false;
    bool pngs = false;
    string suffix;
    bool fails = false;
    int bench = 1;
    for ( int index = 1; index < argc; ++index )
    {
        string option( argv[ index ] );
        if ( option == "--subset" && index < argc - 1 )
            subset = argv[ ++index ];
        else if ( option == "--plain" )
            plain = true;
        else if ( option == "--table" )
            table = true;
        else if ( option == "--pngs" )
            pngs = true;
        else if ( option == "--suffix" && index < argc - 1 )
            suffix = argv[ ++index ];
        else if ( option == "--fails" )
            fails = true;
        else if ( option == "--bench" && index < argc - 1 )
            bench = max( atoi( argv[ ++index ] ), 1 );
        else
        {
            cout <<
                "Usage: " << argv[ 0 ] << " [options...]\n"
                "Options:\n"
                "  --subset <str> : Only run tests with names globbing <str>\n"
                "  --plain        : Do not colorize output or use term codes\n"
                "  --table        : Regenerate the code for table of tests\n"
                "  --pngs         : Write PNG images showing output of tests\n"
                "  --suffix <str> : Append <str> to the filenames of PNGs\n"
                "  --fails        : Generate output only for failures\n"
                "  --bench <int>  : Run each test <int> times, show fastest\n";
            return 1;
        }
    }
#ifdef _WIN32
    HANDLE out = GetStdHandle( STD_OUTPUT_HANDLE );
    DWORD mode = 0;
    if ( out == INVALID_HANDLE_VALUE ||
         !GetConsoleMode( out, &mode ) ||
         !SetConsoleMode( out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING ) )
        plain = true;
#else
    if ( !isatty( fileno( stdout ) ) )
        plain = true;
#endif
    int count = sizeof( tests ) / sizeof( test );
    int total = count;
    if ( !subset.empty() )
    {
        total = 0;
        for ( int index = 0; index < count; ++index )
            if ( glob_match( subset.c_str(), tests[ index ].name ) )
                ++total;
    }
    base64_decode( font_a_base64, font_a );
    base64_decode( font_b_base64, font_b );
    base64_decode( font_c_base64, font_c );
    base64_decode( font_d_base64, font_d );
    base64_decode( font_e_base64, font_e );
    base64_decode( font_f_base64, font_f );
    base64_decode( font_g_base64, font_g );
    int failed = 0;
    int done = 0;
    double geo = 0.0;
    for ( int index = 0; index < count; ++index )
    {
        test const &entry = tests[ index ];
        if ( !subset.empty() && !glob_match( subset.c_str(), entry.name ) )
            continue;
        ++done;
        if ( !fails && !table && !plain )
            cout << "\33[90m" << setw( 3 ) << done << "/" << total
                 << " \33[33m[RUN ] \33[0;90m???????? ?????.??ms\33[m "
                 << entry.name << endl;
        int width = entry.width;
        int height = entry.height;
        unsigned char *image = new unsigned char[ 4 * width * height ];
        double best = 1.0e100;
        for ( int run = 0; run < bench; ++run )
        {
            canvas subject( width, height );
            double start = get_seconds();
            entry.call( subject,
                        static_cast< float >( width ),
                        static_cast< float >( height ) );
            double end = get_seconds();
            best = min( best, end - start );
            if ( run == 0 )
                subject.get_image_data( image, width, height,
                                        4 * width, 0, 0 );
        }
        geo += log( best );
        unsigned hash = hash_image( image, width, height );
        int distance = 0;
        for ( unsigned bits = hash ^ entry.hash; bits; bits &= bits - 1 )
            ++distance;
        bool passed = distance <= 5;
        if ( !passed )
            ++failed;
        else if ( fails )
        {
            delete[] image;
            continue;
        }
        if ( table )
            cout << "    { 0x" << hex << setfill( '0' ) << setw( 8 ) << hash
                 << ", " << dec << width << ", " << height
                 << ", " << entry.name << ", \"" << entry.name << "\" }," << endl;
        else if ( plain )
            cout << setw( 3 ) << done << "/" << total << " ["
                 << ( passed ? "PASS" : "FAIL" ) << "] "
                 << hex << setfill( '0' ) << setw( 8 ) << hash << " "
                 << dec << setfill( ' ' ) << setw( 8 )
                 << fixed << setprecision( 2 ) << ( best * 1000.0 )
                 << "ms " << entry.name << endl;
        else
            cout << ( fails ? "" : "\33[A" ) << "\33[90m"
                 << setw( 3 ) << done << "/" << total << " \33["
                 << ( passed ? "32m[PASS" : "31;1m[FAIL" ) << "]\33[0;90m "
                 << hex << setfill( '0' ) << setw( 8 ) << hash << " "
                 << dec << setfill( ' ' ) << setw( 8 )
                 << fixed << setprecision( 2 ) << ( best * 1000.0 )
                 << "ms\33[m " << entry.name << endl;
        if ( pngs )
            write_png( string( entry.name ) + suffix + ".png",
                       image, width, height );
        delete[] image;
    }
    geo = done ? exp( geo / done ) : 0.0;
    if ( !table && ( !fails || failed ) )
        cout << failed << " failed, "
             << setprecision( 3 ) << fixed << ( geo * 1000.0 ) << "ms geo mean\n";
    return !!failed;
}
