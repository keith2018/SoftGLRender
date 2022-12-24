/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */


#include "material.h"

namespace SoftGL {

#define MATERIAL_BIND_TEXTURE(type, idx) textures[type]->BindProgram(sampler_loc[type], idx)

void TextureGLSL::Create2D(Texture &tex) {
  target_ = GL_TEXTURE_2D;
  glGenTextures(1, &texId_);
  glBindTexture(target_, texId_);

  glTexParameteri(target_, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(target_, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(target_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(target_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  auto &buffer = tex.buffer;
  glTexImage2D(target_,
               0,
               GL_RGBA,
               buffer->GetWidth(),
               buffer->GetHeight(),
               0,
               GL_RGBA,
               GL_UNSIGNED_BYTE,
               buffer->GetRawDataPtr());
  glGenerateMipmap(target_);
}

void TextureGLSL::CreateCube(Texture tex[6]) {
  target_ = GL_TEXTURE_CUBE_MAP;
  glGenTextures(1, &texId_);
  glBindTexture(target_, texId_);

  glTexParameteri(target_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(target_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(target_, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glTexParameteri(target_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(target_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  for (int i = 0; i < 6; i++) {
    auto &buffer = tex[i].buffer;
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                 0,
                 GL_RGBA,
                 buffer->GetWidth(),
                 buffer->GetHeight(),
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 buffer->GetRawDataPtr());
  }
  glGenerateMipmap(target_);
}

bool TextureGLSL::Empty() const {
  return 0 == texId_;
}

void TextureGLSL::BindProgram(GLint samplerLoc, GLuint binding) const {
  if (Empty()) {
    return;
  }
#define BIND_TEX(n) case n: glActiveTexture(GL_TEXTURE##n); break;
  switch (binding) {
    BIND_TEX(0)
    BIND_TEX(1)
    BIND_TEX(2)
    BIND_TEX(3)
    BIND_TEX(4)
    BIND_TEX(5)
    BIND_TEX(6)
    BIND_TEX(7)
    default: {
      LOGE("texture unit not support");
      break;
    }
  }
  glBindTexture(target_, texId_);
  glUniform1i(samplerLoc, binding);
}

TextureGLSL::~TextureGLSL() {
  if (Empty()) {
    return;
  }
  glDeleteTextures(1, &texId_);
}

void MaterialBaseColor::Init() {
  if (!program.Empty()) {
    return;
  }

  program.LoadSource(GetVertexShader(), GetFragmentShader());
  uniforms_loc_mvp = glGetUniformBlockIndex(program.GetId(), "UniformsMVP");
  uniforms_loc_color = glGetUniformBlockIndex(program.GetId(), "UniformsColor");
}

void MaterialBaseColor::Use(RendererUniforms &uniforms, TextureMap &textures) {
  program.Use();

  uniforms.uniforms_mvp.BindProgram(program.GetId(), uniforms_loc_mvp, 0);
  uniforms.uniforms_mvp.UpdateData();

  uniforms.uniforms_color.BindProgram(program.GetId(), uniforms_loc_color, 1);
  uniforms.uniforms_color.UpdateData();
}

void MaterialBlinnPhong::Init() {
  if (!program.Empty()) {
    return;
  }

  if (enable_normal_map_) {
    program.AddDefine("NORMAL_MAP");
  }
  if (enable_emissive_map_) {
    program.AddDefine("EMISSIVE_MAP");
  }
  if (enable_ao_map_) {
    program.AddDefine("AO_MAP");
  }
  if (enable_alpha_discard_) {
    program.AddDefine("ALPHA_DISCARD");
  }

  program.LoadSource(GetVertexShader(), GetFragmentShader());

  uniforms_loc_mvp = glGetUniformBlockIndex(program.GetId(), "UniformsMVP");
  uniforms_loc_color = glGetUniformBlockIndex(program.GetId(), "UniformsColor");
  uniforms_loc_light = glGetUniformBlockIndex(program.GetId(), "UniformsLight");
  uniforms_loc_alpha_mask = glGetUniformBlockIndex(program.GetId(), "UniformsAlphaMask");

  sampler_loc[TextureType_DIFFUSE] = glGetUniformLocation(program.GetId(), "u_diffuseMap");
  sampler_loc[TextureType_NORMALS] = glGetUniformLocation(program.GetId(), "u_normalMap");
  sampler_loc[TextureType_EMISSIVE] = glGetUniformLocation(program.GetId(), "u_emissiveMap");
  sampler_loc[TextureType_PBR_AMBIENT_OCCLUSION] = glGetUniformLocation(program.GetId(), "u_aoMap");
}

void MaterialBlinnPhong::Use(RendererUniforms &uniforms, TextureMap &textures) {
  program.Use();

  // uniform blocks
  GLint ubIdx = 0;
  uniforms.uniforms_mvp.BindProgram(program.GetId(), uniforms_loc_mvp, ubIdx++);
  uniforms.uniforms_mvp.UpdateData();

  uniforms.uniforms_light.BindProgram(program.GetId(), uniforms_loc_light, ubIdx++);
  uniforms.uniforms_light.UpdateData();

  if (enable_alpha_discard_) {
    uniforms.uniforms_alpha_mask.BindProgram(program.GetId(), uniforms_loc_alpha_mask, ubIdx++);
    uniforms.uniforms_alpha_mask.UpdateData();
  }

  // uniform samplers
  samplerIdx = 0;

  if (enable_diffuse_map_) {
    MATERIAL_BIND_TEXTURE(TextureType_DIFFUSE, samplerIdx++);
  }

  if (enable_normal_map_) {
    MATERIAL_BIND_TEXTURE(TextureType_NORMALS, samplerIdx++);
  }

  if (enable_emissive_map_) {
    MATERIAL_BIND_TEXTURE(TextureType_EMISSIVE, samplerIdx++);
  }

  if (enable_ao_map_) {
    MATERIAL_BIND_TEXTURE(TextureType_PBR_AMBIENT_OCCLUSION, samplerIdx++);
  }
}

void MaterialPbrBase::Init() {
  MaterialBlinnPhong::Init();
  sampler_loc[TextureType_PBR_ALBEDO] = glGetUniformLocation(program.GetId(), "u_albedoMap");
  sampler_loc[TextureType_PBR_METAL_ROUGHNESS] = glGetUniformLocation(program.GetId(), "u_metalRoughnessMap");
}

void MaterialPbrBase::Use(RendererUniforms &uniforms, TextureMap &textures) {
  MaterialBlinnPhong::Use(uniforms, textures);
  MATERIAL_BIND_TEXTURE(TextureType_PBR_ALBEDO, samplerIdx++);
  MATERIAL_BIND_TEXTURE(TextureType_PBR_METAL_ROUGHNESS, samplerIdx++);
}

void MaterialPbrIBL::Init() {
  if (!program.Empty()) {
    return;
  }
  program.AddDefine("PBR_IBL");
  MaterialPbrBase::Init();
}

void MaterialPbrIBL::Use(RendererUniforms &uniforms, TextureMap &textures) {
  MaterialPbrBase::Use(uniforms, textures);
  MATERIAL_BIND_TEXTURE(TextureType_IBL_IRRADIANCE, samplerIdx++);
  MATERIAL_BIND_TEXTURE(TextureType_IBL_PREFILTER, samplerIdx++);
}

void MaterialSkybox::Init() {
  if (!program.Empty()) {
    return;
  }

  if (enable_equirectangular_map_) {
    program.AddDefine("EQUIRECTANGULAR_MAP");
  }

  program.LoadSource(GetVertexShader(), GetFragmentShader());
  uniforms_loc_mvp = glGetUniformBlockIndex(program.GetId(), "UniformsMVP");

  sampler_loc[TextureType_CUBE] = glGetUniformLocation(program.GetId(), "u_cubeMap");
  sampler_loc[TextureType_EQUIRECTANGULAR] = glGetUniformLocation(program.GetId(), "u_equirectangularMap");
}

void MaterialSkybox::Use(RendererUniforms &uniforms, TextureMap &textures) {
  program.Use();

  // uniform blocks
  GLint ubIdx = 0;
  uniforms.uniforms_mvp.BindProgram(program.GetId(), uniforms_loc_mvp, ubIdx++);
  uniforms.uniforms_mvp.UpdateData();

  if (enable_equirectangular_map_) {
    MATERIAL_BIND_TEXTURE(TextureType_EQUIRECTANGULAR, 0);
  } else {
    MATERIAL_BIND_TEXTURE(TextureType_CUBE, 0);
  }
}

}
