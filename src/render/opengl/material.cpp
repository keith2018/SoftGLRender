/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */


#include "material.h"

namespace SoftGL {

void TextureGLSL::Create(Texture &tex) {
  if (tex.buffer->Empty()) {
    return;
  }

  glGenTextures(1, &texId_);
  glBindTexture(GL_TEXTURE_2D, texId_);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  auto &buffer = tex.buffer;
  glTexImage2D(GL_TEXTURE_2D,
               0,
               GL_RGBA,
               buffer->GetWidth(),
               buffer->GetHeight(),
               0,
               GL_RGBA,
               GL_UNSIGNED_BYTE,
               buffer->GetRawDataPtr());
  glGenerateMipmap(GL_TEXTURE_2D);
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
  glBindTexture(GL_TEXTURE_2D, texId_);
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

  program.LoadSource(BASIC_VS, BASIC_FS);
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

void MaterialBaseTexture::Init() {
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
  program.LoadSource(BLINN_PHONG_VS, BLINN_PHONG_FS);
  uniforms_loc_mvp = glGetUniformBlockIndex(program.GetId(), "UniformsMVP");
  uniforms_loc_color = glGetUniformBlockIndex(program.GetId(), "UniformsColor");
  uniforms_loc_light = glGetUniformBlockIndex(program.GetId(), "UniformsLight");
  uniforms_loc_alpha_mask = glGetUniformBlockIndex(program.GetId(), "UniformsAlphaMask");

  sampler_loc[TextureType_DIFFUSE] = glGetUniformLocation(program.GetId(), "u_diffuseMap");
  sampler_loc[TextureType_NORMALS] = glGetUniformLocation(program.GetId(), "u_normalMap");
  sampler_loc[TextureType_EMISSIVE] = glGetUniformLocation(program.GetId(), "u_emissiveMap");
  sampler_loc[TextureType_PBR_AMBIENT_OCCLUSION] = glGetUniformLocation(program.GetId(), "u_aoMap");
}

void MaterialBaseTexture::Use(RendererUniforms &uniforms, TextureMap &textures) {
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

#define MATERIAL_BIND_TEXTURE(type) textures[type]->BindProgram(sampler_loc[type], samplerIdx++)

  // uniform samplers
  GLint samplerIdx = 0;
  MATERIAL_BIND_TEXTURE(TextureType_DIFFUSE);

  if (enable_normal_map_) {
    MATERIAL_BIND_TEXTURE(TextureType_NORMALS);
  }

  if (enable_emissive_map_) {
    MATERIAL_BIND_TEXTURE(TextureType_EMISSIVE);
  }

  if (enable_ao_map_) {
    MATERIAL_BIND_TEXTURE(TextureType_PBR_AMBIENT_OCCLUSION);
  }
}

void MaterialBlinnPhong::Init() {
  if (!program.Empty()) {
    return;
  }
  program.AddDefine("POINT_LIGHT");
  MaterialBaseTexture::Init();
}

}
