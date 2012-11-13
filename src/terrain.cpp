#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "terrain.hpp"
#include "globals.hpp"
#include "logging.hpp"
#include "mesh.hpp"
#include "texture.hpp"
#include "utils.hpp"
#include "config.hpp"
#include "color.hpp"

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#define RENDER_DEBUG 0

#if RENDER_DEBUG
	static Shader * debug_shader;
#endif

Terrain::~Terrain() {
	if(map_ != NULL)
		delete map_;
	free_surface();
}

Terrain::Terrain(const std::string &file) {
	Config config = Config::parse(file);

	data_map_  = TextureBase::load_image(config["/heightmap"]->as_string() , &size_);
	horizontal_scale_ = config["/horizontal_size"]->as_float() / static_cast<float>(size_.x);
	vertical_scale_ = config["/vertical_size"]->as_float();;
	uv_scale_ = config["/uv_repeat"]->as_float();
	const std::vector<ConfigEntry*> texture_selection = config["/texture_selection"]->as_list();
	texture_selection_[0] = glm::radians(texture_selection[0]->as_float());
	texture_selection_[1] = glm::radians(texture_selection[1]->as_float()) - texture_selection_[0];
	/* 
	 * second value is given as end, but algorithm handles length (delta)
	*/

	diffuse_textures_ = TextureArray::from_filename(
			config["/materials/flat/diffuse"]->as_string().c_str(),
			config["/materials/steep/diffuse"]->as_string().c_str(),
			config["/materials/override/diffuse"]->as_string().c_str(),
			nullptr
		);

	normal_textures_ = TextureArray::from_filename(
			config["/materials/flat/normal"]->as_string().c_str(),
			config["/materials/steep/normal"]->as_string().c_str(),
			config["/materials/override/normal"]->as_string().c_str(),
			nullptr
		);

	material_shininess_[0] = config["/materials/flat/shininess"]->as_float();
	material_shininess_[1] = config["/materials/steep/shininess"]->as_float();
	material_shininess_[2] = config["/materials/override/shininess"]->as_float();

	material_specular_[0] = config["/materials/flat/specular"]->as_color().to_vec4();
	material_specular_[1] = config["/materials/steep/specular"]->as_color().to_vec4();
	material_specular_[2] = config["/materials/override/specular"]->as_color().to_vec4();

	diffuse_textures_->texture_bind(Shader::TEXTURE_ARRAY_0);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
	normal_textures_->texture_bind(Shader::TEXTURE_ARRAY_0);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
	normal_textures_->texture_unbind();

	shader_ = Shader::create_shader("/shaders/terrain");
	u_texture_selection_[0] = shader_->uniform_location("texture_fade_start");
	u_texture_selection_[1] = shader_->uniform_location("texture_fade_length");

	u_material_shininess_ = shader_->uniform_location("material_shininess");
	u_material_specular_ = shader_->uniform_location("material_specular");

	generate_terrain();

#if RENDER_DEBUG
	debug_shader = Shader::create_shader("/shaders/debug");
#endif
}

void Terrain::free_surface() {
	if(data_map_ != nullptr) {
		SDL_FreeSurface(data_map_);
		data_map_ = nullptr;
	}
}

const glm::ivec2 &Terrain::heightmap_size() const {
	return size_;
}

void Terrain::generate_terrain() {
	unsigned long numVertices = size_.x*size_.y;

	map_ = new float[numVertices];

	Logging::verbose("Generating terrain...\n"
	                 "  - World size: %dx%d\n"
	                 "  - scale: %fx%f\n", size_.x, size_.y, horizontal_scale_, vertical_scale_);

	vertices_ = std::vector<Shader::Shader::vertex_t>(numVertices);
	for(int y=0; y<size_.y; ++y) {
		for(int x=0; x<size_.x; ++x) {
			Shader::Shader::vertex_t v;
			int i = y * size_.x + x;
			glm::vec4 color = get_pixel_color(x, y, data_map_, size_);
			float h = height_from_color(color);
			v.pos = glm::vec3(horizontal_scale_*static_cast<float>(x), h*vertical_scale_, horizontal_scale_*static_cast<float>(y));
			v.uv = glm::vec2(static_cast<float>(x) / static_cast<float>(size_.x), 1.f - static_cast<float>(y) / static_cast<float>(size_.y)) * uv_scale_;
			vertices_[i] = v;
			map_[i] =  h*vertical_scale_;
		}
	}
	unsigned long indexCount = (size_.y - 1 ) * (size_.x -1) * 6;

	//build indices
	std::vector<unsigned int> indices = std::vector<unsigned int>(indexCount);
	for(int x=0; x<size_.x- 1; ++x) {
		for(int y=0; y<size_.y- 1; ++y) {
			int i = y * (size_.x-1) + x;
			indices[i*6 + 2] = (x + 1) + y*size_.x;
			indices[i*6 + 1] = x + (y+1)*size_.x;
			indices[i*6 + 0] = x + y*size_.x;

			indices[i*6 + 5] = (x + 1) + y*size_.x;
			indices[i*6 + 4] = (x+1) + (y+1)*size_.x;
			indices[i*6 + 3] = x + (y+1)*size_.x;
		}
	}

	add_indices(indices);

	generate_normals();
	generate_tangents_and_bitangents();
	ortonormalize_tangent_space();
	generate_vbos();
}

float Terrain::height_from_color(const glm::vec4 &color) const {
	return color.r;
}

float Terrain::height_at(int x, int y) const {
	return map_[y*size_.x + x];
}

float Terrain::height_at(float x_, float y_) const {
	if(x_ > static_cast<float>(size_.x) * horizontal_scale_|| x_ < 0 || y_ > static_cast<float>(size_.y)*horizontal_scale_ || y_ < 0)
		return 0.f;
	int x = (int) (x_/horizontal_scale_);
	int y = (int) (y_/horizontal_scale_);
	float dx = (x_/horizontal_scale_) - (float)x;
	float dy = (y_/horizontal_scale_) - (float)y;
	float height=0;
	height += (1.f-dx) * (1.f-dy) * height_at((float)x,(float)y);
	height += dx * (1.f-dy) * height_at((float)y,(float)(x+1));
	height += (1.f-dx) * dy * height_at((float)(y+1),(float)x);
	height += dx * dy * height_at((float)(y+1), (float)(x+1));
	return height;
}

const glm::vec3 &Terrain::normal_at(int x, int y) const {
	return vertices_[y*size_.x + x].normal;
}

glm::vec3 Terrain::normal_at(float x_, float y_) const {
	if(x_ > static_cast<float>(size_.x) * horizontal_scale_|| x_ < 0 || y_ > static_cast<float>(size_.y)*horizontal_scale_ || y_ < 0)
		return glm::vec3(0.f, 1.f, 0.f);
	int x = (int) (x_/horizontal_scale_);
	int y = (int) (y_/horizontal_scale_);
	float dx = (x_/horizontal_scale_) - (float)x;
	float dy = (y_/horizontal_scale_) - (float)y;
	glm::vec3 normal(0.f);
	normal += (1.f-dx) * (1.f-dy) * normal_at((float)x,(float)y);
	normal += dx * (1.f-dy) * normal_at((float)y,(float)(x+1));
	normal += (1.f-dx) * dy * normal_at((float)(y+1),(float)x);
	normal += dx * dy * normal_at((float)(y+1), (float)(x+1));
	return normal;
}

glm::vec4 Terrain::get_pixel_color(int x, int y, SDL_Surface * surface, const glm::ivec2 &size) {
	glm::ivec4 c = TextureBase::get_pixel_color(x, y, surface, size);
	glm::vec4 color;
	color.r = (float)c.x/0xFF;
	color.g = (float)c.y/0xFF;
	color.b = (float)c.z/0xFF;
	color.a = (float)c.w/0xFF;

	return color;
}

void Terrain::render(const glm::mat4& m) {
	prepare_shader();

	Mesh::render();

#if RENDER_DEBUG
	//Render debug:
	glLineWidth(2.0f);
	debug_shader->bind();

	Mesh::render();
#endif
}

void Terrain::prepare_shader() {
	shader_->bind();
	glUniform1f(u_texture_selection_[0], texture_selection_[0]);
	glUniform1f(u_texture_selection_[1], texture_selection_[1]);
	glUniform1fv(u_material_shininess_, 3, material_shininess_);
	glUniform4fv(u_material_specular_, 3, (float*)material_specular_);

	diffuse_textures_->texture_bind(Shader::TEXTURE_ARRAY_0);
	normal_textures_->texture_bind(Shader::TEXTURE_ARRAY_1);
}

void Terrain::render_cull(const Camera &cam, const glm::mat4& m) {

	prepare_shader();

	prepare_submesh_rendering(m);

	submesh_tree->traverse(Terrain::cull_or_render);
}

void Terrain::render_geometry_cull( const Camera &cam, const glm::mat4& m) {
	prepare_submesh_rendering(m);

	submesh_tree->traverse(Terrain::cull_or_render);
}

bool Terrain::cull_or_render(QuadTree * node) {
	if(node->data != nullptr) ( (SubMesh*) node->data )->render_geometry();
	return true;
}
