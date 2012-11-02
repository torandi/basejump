#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "material.hpp"
#include "texture.hpp"
#include <GL/glew.h>

Material::Material()
	: texture(nullptr)
	, normal_map(nullptr) {

	texture      = Texture2D::default_colormap();
	normal_map   = Texture2D::default_normalmap();
	specular_map = Texture2D::default_specularmap();

	shininess = 1;
	diffuse   = glm::vec4(1.f);
	specular  = glm::vec4(0.0f);
	ambient   = glm::vec4(1.f);
	emission  = glm::vec4(0.0f);
};

Material::~Material(){

}

void Material::bind() const {
	texture->texture_bind(Shader::TEXTURE_COLORMAP);
	normal_map->texture_bind(Shader::TEXTURE_NORMALMAP);
	specular_map->texture_bind(Shader::TEXTURE_SPECULARMAP);

	Shader::upload_material(*this);
}
