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

#ifndef TV_GEOMETRY_HXX
#define TV_GEOMETRY_HXX

#include "Rect.hxx"
#include "bspf.hxx"

/**
  The shape of a CRT's front glass, as a triangle mesh the rendering backend
  uses to present the composited TIA image.  Pipeline stage 3 (display device)
  in Television: it models the tube, changing only where light lands, never its
  colour.  'Geometry' in the TV-service sense -- barrel, pincushion, bow --
  of which only face curvature is modelled today.

  The deflection yoke sweeps at a constant angular rate, so the raster is
  uniform in *arc length* on the curved glass; viewed head-on, a point whose
  arc subtends theta lands at sin(theta).  For a raster point (u, v) in
  [-1, 1], with alphaX/alphaY the half-angles the raster subtends:

    a = (u * alphaX, v * alphaY)     arc position, as an angle
    p = a * sin(|a|) / |a|           where it appears, before normalising

  That is an azimuthal-equidistant parameterisation of the sphere followed by
  an orthographic projection (Snyder, J.P., "Map Projections -- A Working
  Manual", USGS Professional Paper 1395, 1987, sections "Azimuthal Equidistant
  Projection" and "Orthographic Projection"; https://doi.org/10.3133/pp1395).

  Both real effects fall out of it together -- the border bows outward AND
  content compresses toward the edges -- which is the point of using a surface
  model.  A `1 + k*r^2` radial push (crt-lottes' Warp(), libretro
  glsl-shaders, crt/shaders/crt-lottes.glsl) gets the bow right but *inverts*
  the compression, stretching the edges instead.

  Dividing by sin(alpha) per axis puts the edge mid-points exactly on the
  destination rectangle and every other point inside it, so a curved image
  never spills outside the rectangle a flat one occupied -- which is what
  keeps it from fighting the bezel artwork drawn around it.

  @author  Stephen Anthony
*/
class TVGeometry
{
  public:
    // Setting names owned by this stage
    static constexpr string_view SETTING_CURVATURE = "tv.curvature";
    static constexpr string_view SETTING_CURVATURE_Y = "tv.curvature.y";

  public:
    /**
      One mesh vertex.  'x/y' is where it lands on screen, in the coordinate
      space of the rectangle passed to build(); 'u/v' is the point of the
      *flat* image it samples, normalised to that rectangle.
    */
    struct Vertex {
      float x{0.F}, y{0.F};
      float u{0.F}, v{0.F};

      // RGB modulation of the sampled texel; always 1.0 today.  The hook a
      // vignette or glass-glare pass would drive, free to carry since the
      // vertex format has a colour regardless.
      float shade{1.F};
    };

  public:
    TVGeometry() = default;

    /**
      Set the curvature of the tube face.

      @param amount    Overall curvature, 0 (flat) to 100 (maximum)
      @param vertical  Vertical curvature as a percentage of spherical: 100
                       gives a spherical tube, 0 a cylindrical one that is
                       vertically flat, as Sony's Trinitron sets were
    */
    void setCurvature(uInt32 amount, uInt32 vertical);

    /**
      Is the tube curved at all?  When false no mesh is built, and callers
      should present the image with a plain rectangular blit.
    */
    bool enabled() const { return myAmount > 0; }

    /**
      Build the mesh for the given destination rectangle, whose coordinate
      space the vertex positions come back in.  No-op when neither the
      rectangle nor the curvature has changed.
    */
    void build(const Common::Rect& dst);

    const std::vector<Vertex>& vertices() const { return myVertices; }

    // Empty until a mesh exists, so that the two always describe each other
    IntSpan indices() const;

    // Bumped on every actual rebuild, so a backend can tell when the
    // API-specific vertex array it derives from vertices() has gone stale
    uInt32 generation() const { return myGeneration; }

    /**
      Map a point on the curved screen back to the point of the flat image
      drawn there, both normalised to the destination rectangle ([0,0] top
      left, [1,1] bottom right).  Needed by anything reasoning about where the
      user is pointing -- see Lightgun.

      Exact, not an approximation: undoing the normalisation leaves a vector
      of length sin(theta), so theta comes back in closed form.
    */
    void unwarp(float& u, float& v) const;

  private:
    // Half-angle the raster subtends at curvature 100, in radians (~34
    // degrees): clearly curved without caricature
    static constexpr float MAX_ANGLE = 0.6F;

    // Mesh density, in quads.  Every vertex-array API maps a triangle
    // affinely, so straight lines survive as chains of short segments; this
    // keeps the kinks below a pixel at any window size Stella can open.
    static constexpr uInt32 GRID_X = 40, GRID_Y = 30;

    using IndexArray =
      std::array<uInt32, static_cast<size_t>(GRID_X) * GRID_Y * 6>;

    // Two triangles per quad.  Only the vertex positions ever change, never
    // how they are joined up, so the topology is built once at compile time.
    // SDL_RenderGeometry does no culling, so winding is not significant.
    static constexpr IndexArray makeIndices()
    {
      IndexArray idx{};
      size_t i = 0;

      for(uInt32 row = 0; row < GRID_Y; ++row)
        for(uInt32 col = 0; col < GRID_X; ++col)
        {
          const uInt32 topLeft = row * (GRID_X + 1) + col,
                       topRight = topLeft + 1,
                       bottomLeft = topLeft + GRID_X + 1,
                       bottomRight = bottomLeft + 1;

          idx[i++] = topLeft;  idx[i++] = bottomLeft; idx[i++] = topRight;
          idx[i++] = topRight; idx[i++] = bottomLeft; idx[i++] = bottomRight;
        }
      return idx;
    }

    // Map a flat image position ([-1,1], origin at the centre) to its
    // position on the curved face, in that same space
    void warp(float u, float v, float& x, float& y) const;

    // sin(t)/t, continuous at the origin
    static float sinc(float t) {
      return t < 1.0E-6F ? 1.F : std::sin(t) / t;
    }

  private:
    // User settings, as percentages
    uInt32 myAmount{0}, myVertical{100};

    // Half-angles the raster subtends, horizontally and vertically
    float myAlphaX{0.F}, myAlphaY{0.F};

    // Per-axis normalisation, sin(alpha); zero means "this axis is flat"
    float myNormX{0.F}, myNormY{0.F};

    // Rectangle the current mesh was built for
    Common::Rect myRect;

    std::vector<Vertex> myVertices;

    uInt32 myGeneration{0};
    bool myDirty{true};

  private:
    // Following constructors and assignment operators not supported
    TVGeometry(const TVGeometry&) = delete;
    TVGeometry(TVGeometry&&) = delete;
    TVGeometry& operator=(const TVGeometry&) = delete;
    TVGeometry& operator=(TVGeometry&&) = delete;
};

#endif  // TV_GEOMETRY_HXX
