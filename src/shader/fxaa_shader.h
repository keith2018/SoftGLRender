/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#pragma once

#include "renderer/shader.h"

namespace SoftGL {

#define FXAAShaderAttributes Vertex

struct FXAAShaderUniforms : BaseShaderUniforms {
  Sampler2D u_screenTexture;
  glm::vec2 u_inverseScreenSize;
};

struct FXAAShaderVaryings : BaseShaderVaryings {
  glm::vec2 v_texCoord;
};

struct FXAAVertexShader : BaseVertexShader {

  void shader_main() override {
    auto *a = (FXAAShaderAttributes *) attributes;
    auto *v = (FXAAShaderVaryings *) varyings;

    gl_Position = glm::vec4(a->a_position, 1.0f);
    v->v_texCoord = a->a_texCoord;
  }

  CLONE_VERTEX_SHADER(FXAAVertexShader)
};


// Ref: https://github.com/kosua20/Rendu/blob/master/resources/common/shaders/screens/fxaa.frag

// Settings for FXAA.
#define EDGE_THRESHOLD_MIN 0.0312f
#define EDGE_THRESHOLD_MAX 0.125f
#define QUALITY(q) ((q) < 5.f ? 1.0f : ((q) > 5.f ? ((q) < 10.f ? 2.0f : ((q) < 11.f ? 4.0f : 8.0f)) : 1.5f))
#define ITERATIONS 12
#define SUBPIXEL_QUALITY 0.75f

struct FXAAFragmentShader : BaseFragmentShader {
  FXAAShaderUniforms *u = nullptr;
  FXAAShaderVaryings *v = nullptr;

  static float rgb2luma(glm::vec3 rgb) {
    return glm::dot(rgb, glm::vec3(0.299, 0.587, 0.114));
  }

  glm::vec3 fxaa() {
    glm::vec3 colorCenter = u->u_screenTexture.texture2D(v->v_texCoord);
    // Luma at the current fragment
    float lumaCenter = rgb2luma(colorCenter);

    // Luma at the four direct neighbours of the current fragment.
    float lumaDown = rgb2luma(u->u_screenTexture.texture2DLodOffset(v->v_texCoord, 0.f, glm::ivec2(0, -1)));
    float lumaUp = rgb2luma(u->u_screenTexture.texture2DLodOffset(v->v_texCoord, 0.f, glm::ivec2(0, 1)));
    float lumaLeft = rgb2luma(u->u_screenTexture.texture2DLodOffset(v->v_texCoord, 0.f, glm::ivec2(-1, 0)));
    float lumaRight = rgb2luma(u->u_screenTexture.texture2DLodOffset(v->v_texCoord, 0.f, glm::ivec2(1, 0)));

    // Find the maximum and minimum luma around the current fragment.
    float lumaMin = glm::min(lumaCenter, glm::min(glm::min(lumaDown, lumaUp), glm::min(lumaLeft, lumaRight)));
    float lumaMax = glm::max(lumaCenter, glm::max(glm::max(lumaDown, lumaUp), glm::max(lumaLeft, lumaRight)));

    // Compute the delta.
    float lumaRange = lumaMax - lumaMin;

    // If the luma variation is lower that a threshold (or if we are in a really dark area), we are not on an edge, don't perform any AA.
    if (lumaRange < glm::max(EDGE_THRESHOLD_MIN, lumaMax * EDGE_THRESHOLD_MAX)) {
      return colorCenter;
    }

    // Query the 4 remaining corners lumas.
    float lumaDownLeft = rgb2luma(u->u_screenTexture.texture2DLodOffset(v->v_texCoord, 0.f, glm::ivec2(-1, -1)));
    float lumaUpRight = rgb2luma(u->u_screenTexture.texture2DLodOffset(v->v_texCoord, 0.f, glm::ivec2(1, 1)));
    float lumaUpLeft = rgb2luma(u->u_screenTexture.texture2DLodOffset(v->v_texCoord, 0.f, glm::ivec2(-1, 1)));
    float lumaDownRight = rgb2luma(u->u_screenTexture.texture2DLodOffset(v->v_texCoord, 0.f, glm::ivec2(1, -1)));

    // Combine the four edges lumas (using intermediary variables for future computations with the same values).
    float lumaDownUp = lumaDown + lumaUp;
    float lumaLeftRight = lumaLeft + lumaRight;

    // Same for corners
    float lumaLeftCorners = lumaDownLeft + lumaUpLeft;
    float lumaDownCorners = lumaDownLeft + lumaDownRight;
    float lumaRightCorners = lumaDownRight + lumaUpRight;
    float lumaUpCorners = lumaUpRight + lumaUpLeft;

    // Compute an estimation of the gradient along the horizontal and vertical axis.
    float edgeHorizontal = abs(-2.0f * lumaLeft + lumaLeftCorners) + abs(-2.0f * lumaCenter + lumaDownUp) * 2.0f
        + abs(-2.0f * lumaRight + lumaRightCorners);
    float edgeVertical = abs(-2.0f * lumaUp + lumaUpCorners) + abs(-2.0f * lumaCenter + lumaLeftRight) * 2.0f
        + abs(-2.0f * lumaDown + lumaDownCorners);

    // Is the local edge horizontal or vertical ?
    bool isHorizontal = (edgeHorizontal >= edgeVertical);

    // Choose the step size (one pixel) accordingly.
    float stepLength = isHorizontal ? u->u_inverseScreenSize.y : u->u_inverseScreenSize.x;

    // Select the two neighboring texels lumas in the opposite direction to the local edge.
    float luma1 = isHorizontal ? lumaDown : lumaLeft;
    float luma2 = isHorizontal ? lumaUp : lumaRight;
    // Compute gradients in this direction.
    float gradient1 = luma1 - lumaCenter;
    float gradient2 = luma2 - lumaCenter;

    // Which direction is the steepest ?
    bool is1Steepest = abs(gradient1) >= abs(gradient2);

    // Gradient in the corresponding direction, normalized.
    float gradientScaled = 0.25f * glm::max(abs(gradient1), abs(gradient2));

    // Average luma in the correct direction.
    float lumaLocalAverage = 0.0f;
    if (is1Steepest) {
      // Switch the direction
      stepLength = -stepLength;
      lumaLocalAverage = 0.5f * (luma1 + lumaCenter);
    } else {
      lumaLocalAverage = 0.5f * (luma2 + lumaCenter);
    }

    // Shift UV in the correct direction by half a pixel.
    glm::vec2 currentUv = v->v_texCoord;
    if (isHorizontal) {
      currentUv.y += stepLength * 0.5f;
    } else {
      currentUv.x += stepLength * 0.5f;
    }

    // Compute offset (for each iteration step) in the right direction.
    glm::vec2
        offset = isHorizontal ? glm::vec2(u->u_inverseScreenSize.x, 0.0f) : glm::vec2(0.0f, u->u_inverseScreenSize.y);
    // Compute UVs to explore on each side of the edge, orthogonally. The QUALITY allows us to step faster.
    glm::vec2 uv1 = currentUv - offset * QUALITY(0.f);
    glm::vec2 uv2 = currentUv + offset * QUALITY(0.f);

    float lumaEnd1;
    float lumaEnd2;

    // If the luma deltas at the current extremities is larger than the local gradient, we have reached the side of the edge.
    bool reached1 = false;
    bool reached2 = false;
    bool reachedBoth = false;

    // If both sides have not been reached, continue to explore.
    for (int i = 1; i < ITERATIONS; i++) {
      // If needed, read luma in 1st direction, compute delta.
      if (!reached1) {
        lumaEnd1 = rgb2luma(u->u_screenTexture.texture2D(uv1));
        lumaEnd1 = lumaEnd1 - lumaLocalAverage;
        // If the luma deltas at the current extremities is larger than the local gradient, we have reached the side of the edge.
        reached1 = abs(lumaEnd1) >= gradientScaled;
      }
      // If needed, read luma in opposite direction, compute delta.
      if (!reached2) {
        lumaEnd2 = rgb2luma(u->u_screenTexture.texture2D(uv2));
        lumaEnd2 = lumaEnd2 - lumaLocalAverage;
        // If the luma deltas at the current extremities is larger than the local gradient, we have reached the side of the edge.
        reached2 = abs(lumaEnd2) >= gradientScaled;
      }
      reachedBoth = reached1 && reached2;

      // If the side is not reached, we continue to explore in this direction, with a variable quality.
      if (!reached1) {
        uv1 -= offset * QUALITY(i);
      }
      if (!reached2) {
        uv2 += offset * QUALITY(i);
      }

      // If both sides have been reached, stop the exploration.
      if (reachedBoth) { break; }
    }

    // Compute the distances to each side edge of the edge (!).
    float distance1 = isHorizontal ? (v->v_texCoord.x - uv1.x) : (v->v_texCoord.y - uv1.y);
    float distance2 = isHorizontal ? (uv2.x - v->v_texCoord.x) : (uv2.y - v->v_texCoord.y);

    // In which direction is the side of the edge closer ?
    bool isDirection1 = distance1 < distance2;
    float distanceFinal = glm::min(distance1, distance2);

    // Is the luma at center smaller than the local average ?
    bool isLumaCenterSmaller = lumaCenter < lumaLocalAverage;

    // If the luma at center is smaller than at its neighbour, the delta luma at each end should be positive (same variation).
    bool correctVariation1 = (lumaEnd1 < 0.0f) != isLumaCenterSmaller;
    bool correctVariation2 = (lumaEnd2 < 0.0f) != isLumaCenterSmaller;

    // Only keep the result in the direction of the closer side of the edge.
    bool correctVariation = isDirection1 ? correctVariation1 : correctVariation2;

    // Length of the edge.
    float edgeLength = (distance1 + distance2);

    // UV offset: read in the direction of the closest side of the edge.
    float pixelOffset = -distanceFinal / edgeLength + 0.5f;

    // If the luma variation is incorrect, do not offset.
    float finalOffset = correctVariation ? pixelOffset : 0.0f;

    // Sub-pixel shifting
    // Full weighted average of the luma over the 3x3 neighborhood.
    float lumaAverage = (1.0f / 12.0f) * (2.0f * (lumaDownUp + lumaLeftRight) + lumaLeftCorners + lumaRightCorners);
    // Ratio of the delta between the global average and the center luma, over the luma range in the 3x3 neighborhood.
    float subPixelOffset1 = glm::clamp(glm::abs(lumaAverage - lumaCenter) / lumaRange, 0.0f, 1.0f);
    float subPixelOffset2 = (-2.0f * subPixelOffset1 + 3.0f) * subPixelOffset1 * subPixelOffset1;
    // Compute a sub-pixel offset based on this delta.
    float subPixelOffsetFinal = subPixelOffset2 * subPixelOffset2 * SUBPIXEL_QUALITY;

    // Pick the biggest of the two offsets.
    finalOffset = glm::max(finalOffset, subPixelOffsetFinal);

    // Compute the final UV coordinates.
    glm::vec2 finalUv = v->v_texCoord;
    if (isHorizontal) {
      finalUv.y += finalOffset * stepLength;
    } else {
      finalUv.x += finalOffset * stepLength;
    }

    // Read the color at the new UV coordinates, and use it.
    glm::vec3 finalColor = u->u_screenTexture.texture2D(finalUv);
    return finalColor;
  }

  void shader_main() override {
    BaseFragmentShader::shader_main();
    u = (FXAAShaderUniforms *) uniforms;
    v = (FXAAShaderVaryings *) varyings;

    gl_FragColor = glm::vec4(fxaa(), 1.f);
  }

  CLONE_FRAGMENT_SHADER(FXAAFragmentShader)
};

}