/*************************************************************************/
/*  material_storage.h                                                   */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifndef MATERIAL_STORAGE_GLES3_H
#define MATERIAL_STORAGE_GLES3_H

#ifdef GLES3_ENABLED

#include "core/templates/local_vector.h"
#include "core/templates/rid_owner.h"
#include "core/templates/self_list.h"
#include "servers/rendering/renderer_compositor.h"
#include "servers/rendering/renderer_storage.h"
#include "servers/rendering/shader_compiler.h"
#include "servers/rendering/shader_language.h"
#include "servers/rendering/storage/material_storage.h"

#include "../shaders/canvas.glsl.gen.h"
#include "../shaders/cubemap_filter.glsl.gen.h"
#include "../shaders/scene.glsl.gen.h"
#include "../shaders/sky.glsl.gen.h"

namespace GLES3 {

/* Shader Structs */

struct ShaderData {
	virtual void set_code(const String &p_Code) = 0;
	virtual void set_default_texture_param(const StringName &p_name, RID p_texture, int p_index) = 0;
	virtual void get_param_list(List<PropertyInfo> *p_param_list) const = 0;

	virtual void get_instance_param_list(List<RendererMaterialStorage::InstanceShaderParam> *p_param_list) const = 0;
	virtual bool is_param_texture(const StringName &p_param) const = 0;
	virtual bool is_animated() const = 0;
	virtual bool casts_shadows() const = 0;
	virtual Variant get_default_parameter(const StringName &p_parameter) const = 0;
	virtual RS::ShaderNativeSourceCode get_native_source_code() const { return RS::ShaderNativeSourceCode(); }

	virtual ~ShaderData() {}
};

typedef ShaderData *(*ShaderDataRequestFunction)();

struct Material;

struct Shader {
	ShaderData *data = nullptr;
	String code;
	RS::ShaderMode mode;
	HashMap<StringName, HashMap<int, RID>> default_texture_parameter;
	HashSet<Material *> owners;
};

/* Material structs */

struct MaterialData {
	void update_uniform_buffer(const HashMap<StringName, ShaderLanguage::ShaderNode::Uniform> &p_uniforms, const uint32_t *p_uniform_offsets, const HashMap<StringName, Variant> &p_parameters, uint8_t *p_buffer, uint32_t p_buffer_size, bool p_use_linear_color);
	void update_textures(const HashMap<StringName, Variant> &p_parameters, const HashMap<StringName, HashMap<int, RID>> &p_default_textures, const Vector<ShaderCompiler::GeneratedCode::Texture> &p_texture_uniforms, RID *p_textures, bool p_use_linear_color);

	virtual void set_render_priority(int p_priority) = 0;
	virtual void set_next_pass(RID p_pass) = 0;
	virtual void update_parameters(const HashMap<StringName, Variant> &p_parameters, bool p_uniform_dirty, bool p_textures_dirty) = 0;
	virtual void bind_uniforms() = 0;
	virtual ~MaterialData();

	// Used internally by all Materials
	void update_parameters_internal(const HashMap<StringName, Variant> &p_parameters, bool p_uniform_dirty, bool p_textures_dirty, const HashMap<StringName, ShaderLanguage::ShaderNode::Uniform> &p_uniforms, const uint32_t *p_uniform_offsets, const Vector<ShaderCompiler::GeneratedCode::Texture> &p_texture_uniforms, const HashMap<StringName, HashMap<int, RID>> &p_default_texture_params, uint32_t p_ubo_size);

protected:
	Vector<uint8_t> ubo_data;
	GLuint uniform_buffer = GLuint(0);
	Vector<RID> texture_cache;

private:
	friend class MaterialStorage;
	RID self;
	List<RID>::Element *global_buffer_E = nullptr;
	List<RID>::Element *global_texture_E = nullptr;
	uint64_t global_textures_pass = 0;
	HashMap<StringName, uint64_t> used_global_textures;

	//internally by update_parameters_internal
};

typedef MaterialData *(*MaterialDataRequestFunction)(ShaderData *);

struct Material {
	RID self;
	MaterialData *data = nullptr;
	Shader *shader = nullptr;
	//shortcut to shader data and type
	RS::ShaderMode shader_mode = RS::SHADER_MAX;
	uint32_t shader_id = 0;
	bool uniform_dirty = false;
	bool texture_dirty = false;
	HashMap<StringName, Variant> params;
	int32_t priority = 0;
	RID next_pass;
	SelfList<Material> update_element;

	RendererStorage::Dependency dependency;

	Material() :
			update_element(this) {}
};

/* CanvasItem Materials */

struct CanvasShaderData : public ShaderData {
	enum BlendMode { //used internally
		BLEND_MODE_MIX,
		BLEND_MODE_ADD,
		BLEND_MODE_SUB,
		BLEND_MODE_MUL,
		BLEND_MODE_PMALPHA,
		BLEND_MODE_DISABLED,
	};

	bool valid;
	RID version;
	String path;
	BlendMode blend_mode = BLEND_MODE_MIX;

	HashMap<StringName, ShaderLanguage::ShaderNode::Uniform> uniforms;
	Vector<ShaderCompiler::GeneratedCode::Texture> texture_uniforms;

	Vector<uint32_t> ubo_offsets;
	uint32_t ubo_size;

	String code;
	HashMap<StringName, HashMap<int, RID>> default_texture_params;

	bool uses_screen_texture = false;
	bool uses_sdf = false;
	bool uses_time = false;

	virtual void set_code(const String &p_Code);
	virtual void set_default_texture_param(const StringName &p_name, RID p_texture, int p_index);
	virtual void get_param_list(List<PropertyInfo> *p_param_list) const;
	virtual void get_instance_param_list(List<RendererMaterialStorage::InstanceShaderParam> *p_param_list) const;

	virtual bool is_param_texture(const StringName &p_param) const;
	virtual bool is_animated() const;
	virtual bool casts_shadows() const;
	virtual Variant get_default_parameter(const StringName &p_parameter) const;
	virtual RS::ShaderNativeSourceCode get_native_source_code() const;

	CanvasShaderData();
	virtual ~CanvasShaderData();
};

ShaderData *_create_canvas_shader_func();

struct CanvasMaterialData : public MaterialData {
	CanvasShaderData *shader_data = nullptr;

	virtual void set_render_priority(int p_priority) {}
	virtual void set_next_pass(RID p_pass) {}
	virtual void update_parameters(const HashMap<StringName, Variant> &p_parameters, bool p_uniform_dirty, bool p_textures_dirty);
	virtual void bind_uniforms();
	virtual ~CanvasMaterialData();
};

MaterialData *_create_canvas_material_func(ShaderData *p_shader);

/* Sky Materials */

struct SkyShaderData : public ShaderData {
	bool valid;
	RID version;

	HashMap<StringName, ShaderLanguage::ShaderNode::Uniform> uniforms;
	Vector<ShaderCompiler::GeneratedCode::Texture> texture_uniforms;

	Vector<uint32_t> ubo_offsets;
	uint32_t ubo_size;

	String path;
	String code;
	HashMap<StringName, HashMap<int, RID>> default_texture_params;

	bool uses_time;
	bool uses_position;
	bool uses_half_res;
	bool uses_quarter_res;
	bool uses_light;

	virtual void set_code(const String &p_Code);
	virtual void set_default_texture_param(const StringName &p_name, RID p_texture, int p_index);
	virtual void get_param_list(List<PropertyInfo> *p_param_list) const;
	virtual void get_instance_param_list(List<RendererMaterialStorage::InstanceShaderParam> *p_param_list) const;
	virtual bool is_param_texture(const StringName &p_param) const;
	virtual bool is_animated() const;
	virtual bool casts_shadows() const;
	virtual Variant get_default_parameter(const StringName &p_parameter) const;
	virtual RS::ShaderNativeSourceCode get_native_source_code() const;
	SkyShaderData();
	virtual ~SkyShaderData();
};

ShaderData *_create_sky_shader_func();

struct SkyMaterialData : public MaterialData {
	SkyShaderData *shader_data = nullptr;
	bool uniform_set_updated = false;

	virtual void set_render_priority(int p_priority) {}
	virtual void set_next_pass(RID p_pass) {}
	virtual void update_parameters(const HashMap<StringName, Variant> &p_parameters, bool p_uniform_dirty, bool p_textures_dirty);
	virtual void bind_uniforms();
	virtual ~SkyMaterialData();
};

MaterialData *_create_sky_material_func(ShaderData *p_shader);

/* Scene Materials */

struct SceneShaderData : public ShaderData {
	enum BlendMode { //used internally
		BLEND_MODE_MIX,
		BLEND_MODE_ADD,
		BLEND_MODE_SUB,
		BLEND_MODE_MUL,
		BLEND_MODE_ALPHA_TO_COVERAGE
	};

	enum DepthDraw {
		DEPTH_DRAW_DISABLED,
		DEPTH_DRAW_OPAQUE,
		DEPTH_DRAW_ALWAYS
	};

	enum DepthTest {
		DEPTH_TEST_DISABLED,
		DEPTH_TEST_ENABLED
	};

	enum Cull {
		CULL_DISABLED,
		CULL_FRONT,
		CULL_BACK
	};

	enum AlphaAntiAliasing {
		ALPHA_ANTIALIASING_OFF,
		ALPHA_ANTIALIASING_ALPHA_TO_COVERAGE,
		ALPHA_ANTIALIASING_ALPHA_TO_COVERAGE_AND_TO_ONE
	};

	bool valid;
	RID version;

	String path;

	HashMap<StringName, ShaderLanguage::ShaderNode::Uniform> uniforms;
	Vector<ShaderCompiler::GeneratedCode::Texture> texture_uniforms;

	Vector<uint32_t> ubo_offsets;
	uint32_t ubo_size;

	String code;
	HashMap<StringName, HashMap<int, RID>> default_texture_params;

	BlendMode blend_mode;
	AlphaAntiAliasing alpha_antialiasing_mode;
	DepthDraw depth_draw;
	DepthTest depth_test;
	Cull cull_mode;

	bool uses_point_size;
	bool uses_alpha;
	bool uses_blend_alpha;
	bool uses_alpha_clip;
	bool uses_depth_pre_pass;
	bool uses_discard;
	bool uses_roughness;
	bool uses_normal;
	bool uses_particle_trails;
	bool wireframe;

	bool unshaded;
	bool uses_vertex;
	bool uses_position;
	bool uses_sss;
	bool uses_transmittance;
	bool uses_screen_texture;
	bool uses_depth_texture;
	bool uses_normal_texture;
	bool uses_time;
	bool writes_modelview_or_projection;
	bool uses_world_coordinates;
	bool uses_tangent;
	bool uses_color;
	bool uses_uv;
	bool uses_uv2;
	bool uses_custom0;
	bool uses_custom1;
	bool uses_custom2;
	bool uses_custom3;
	bool uses_bones;
	bool uses_weights;

	uint32_t vertex_input_mask = 0;

	uint64_t last_pass = 0;
	uint32_t index = 0;

	virtual void set_code(const String &p_Code);
	virtual void set_default_texture_param(const StringName &p_name, RID p_texture, int p_index);
	virtual void get_param_list(List<PropertyInfo> *p_param_list) const;
	virtual void get_instance_param_list(List<RendererMaterialStorage::InstanceShaderParam> *p_param_list) const;

	virtual bool is_param_texture(const StringName &p_param) const;
	virtual bool is_animated() const;
	virtual bool casts_shadows() const;
	virtual Variant get_default_parameter(const StringName &p_parameter) const;
	virtual RS::ShaderNativeSourceCode get_native_source_code() const;

	SceneShaderData();
	virtual ~SceneShaderData();
};

ShaderData *_create_scene_shader_func();

struct SceneMaterialData : public MaterialData {
	SceneShaderData *shader_data = nullptr;
	uint64_t last_pass = 0;
	uint32_t index = 0;
	RID next_pass;
	uint8_t priority = 0;
	virtual void set_render_priority(int p_priority);
	virtual void set_next_pass(RID p_pass);
	virtual void update_parameters(const HashMap<StringName, Variant> &p_parameters, bool p_uniform_dirty, bool p_textures_dirty);
	virtual void bind_uniforms();
	virtual ~SceneMaterialData();
};

MaterialData *_create_scene_material_func(ShaderData *p_shader);

/* Global variable structs */
struct GlobalVariables {
	enum {
		BUFFER_DIRTY_REGION_SIZE = 1024
	};
	struct Variable {
		HashSet<RID> texture_materials; // materials using this

		RS::GlobalVariableType type;
		Variant value;
		Variant override;
		int32_t buffer_index; //for vectors
		int32_t buffer_elements; //for vectors
	};

	HashMap<StringName, Variable> variables;

	struct Value {
		float x;
		float y;
		float z;
		float w;
	};

	struct ValueInt {
		int32_t x;
		int32_t y;
		int32_t z;
		int32_t w;
	};

	struct ValueUInt {
		uint32_t x;
		uint32_t y;
		uint32_t z;
		uint32_t w;
	};

	struct ValueUsage {
		uint32_t elements = 0;
	};

	List<RID> materials_using_buffer;
	List<RID> materials_using_texture;

	GLuint buffer = GLuint(0);
	Value *buffer_values = nullptr;
	ValueUsage *buffer_usage = nullptr;
	bool *buffer_dirty_regions = nullptr;
	uint32_t buffer_dirty_region_count = 0;

	uint32_t buffer_size;

	bool must_update_texture_materials = false;
	bool must_update_buffer_materials = false;

	HashMap<RID, int32_t> instance_buffer_pos;
};

class MaterialStorage : public RendererMaterialStorage {
private:
	friend struct MaterialData;
	static MaterialStorage *singleton;

	/* GLOBAL VARIABLE API */

	GlobalVariables global_variables;

	int32_t _global_variable_allocate(uint32_t p_elements);
	void _global_variable_store_in_buffer(int32_t p_index, RS::GlobalVariableType p_type, const Variant &p_value);
	void _global_variable_mark_buffer_dirty(int32_t p_index, int32_t p_elements);

	/* SHADER API */

	ShaderDataRequestFunction shader_data_request_func[RS::SHADER_MAX];
	mutable RID_Owner<Shader, true> shader_owner;

	/* MATERIAL API */
	MaterialDataRequestFunction material_data_request_func[RS::SHADER_MAX];
	mutable RID_Owner<Material, true> material_owner;

	SelfList<Material>::List material_update_list;

public:
	static MaterialStorage *get_singleton();

	MaterialStorage();
	virtual ~MaterialStorage();

	struct Shaders {
		CanvasShaderGLES3 canvas_shader;
		SkyShaderGLES3 sky_shader;
		SceneShaderGLES3 scene_shader;
		CubemapFilterShaderGLES3 cubemap_filter_shader;

		ShaderCompiler compiler_canvas;
		ShaderCompiler compiler_scene;
		ShaderCompiler compiler_particles;
		ShaderCompiler compiler_sky;
	} shaders;

	/* GLOBAL VARIABLE API */

	void _update_global_variables();

	virtual void global_variable_add(const StringName &p_name, RS::GlobalVariableType p_type, const Variant &p_value) override;
	virtual void global_variable_remove(const StringName &p_name) override;
	virtual Vector<StringName> global_variable_get_list() const override;

	virtual void global_variable_set(const StringName &p_name, const Variant &p_value) override;
	virtual void global_variable_set_override(const StringName &p_name, const Variant &p_value) override;
	virtual Variant global_variable_get(const StringName &p_name) const override;
	virtual RS::GlobalVariableType global_variable_get_type(const StringName &p_name) const override;
	RS::GlobalVariableType global_variable_get_type_internal(const StringName &p_name) const;

	virtual void global_variables_load_settings(bool p_load_textures = true) override;
	virtual void global_variables_clear() override;

	virtual int32_t global_variables_instance_allocate(RID p_instance) override;
	virtual void global_variables_instance_free(RID p_instance) override;
	virtual void global_variables_instance_update(RID p_instance, int p_index, const Variant &p_value) override;

	GLuint global_variables_get_uniform_buffer() const;

	/* SHADER API */

	Shader *get_shader(RID p_rid) { return shader_owner.get_or_null(p_rid); };
	bool owns_shader(RID p_rid) { return shader_owner.owns(p_rid); };

	void _shader_make_dirty(Shader *p_shader);

	virtual RID shader_allocate() override;
	virtual void shader_initialize(RID p_rid) override;
	virtual void shader_free(RID p_rid) override;

	virtual void shader_set_code(RID p_shader, const String &p_code) override;
	virtual String shader_get_code(RID p_shader) const override;
	virtual void shader_get_param_list(RID p_shader, List<PropertyInfo> *p_param_list) const override;

	virtual void shader_set_default_texture_param(RID p_shader, const StringName &p_name, RID p_texture, int p_index) override;
	virtual RID shader_get_default_texture_param(RID p_shader, const StringName &p_name, int p_index) const override;
	virtual Variant shader_get_param_default(RID p_shader, const StringName &p_param) const override;

	virtual RS::ShaderNativeSourceCode shader_get_native_source_code(RID p_shader) const override;

	/* MATERIAL API */

	Material *get_material(RID p_rid) { return material_owner.get_or_null(p_rid); };
	bool owns_material(RID p_rid) { return material_owner.owns(p_rid); };

	void _material_queue_update(Material *material, bool p_uniform, bool p_texture);
	void _update_queued_materials();

	virtual RID material_allocate() override;
	virtual void material_initialize(RID p_rid) override;
	virtual void material_free(RID p_rid) override;

	virtual void material_set_shader(RID p_material, RID p_shader) override;

	virtual void material_set_param(RID p_material, const StringName &p_param, const Variant &p_value) override;
	virtual Variant material_get_param(RID p_material, const StringName &p_param) const override;

	virtual void material_set_next_pass(RID p_material, RID p_next_material) override;
	virtual void material_set_render_priority(RID p_material, int priority) override;

	virtual bool material_is_animated(RID p_material) override;
	virtual bool material_casts_shadows(RID p_material) override;

	virtual void material_get_instance_shader_parameters(RID p_material, List<InstanceShaderParam> *r_parameters) override;

	virtual void material_update_dependency(RID p_material, RendererStorage::DependencyTracker *p_instance) override;

	_FORCE_INLINE_ uint32_t material_get_shader_id(RID p_material) {
		Material *material = material_owner.get_or_null(p_material);
		return material->shader_id;
	}

	_FORCE_INLINE_ MaterialData *material_get_data(RID p_material, RS::ShaderMode p_shader_mode) {
		Material *material = material_owner.get_or_null(p_material);
		if (!material || material->shader_mode != p_shader_mode) {
			return nullptr;
		} else {
			return material->data;
		}
	}
};

} // namespace GLES3

#endif // GLES3_ENABLED

#endif // !MATERIAL_STORAGE_GLES3_H