/*************************************************************************/
/*  shader_include.cpp                                                   */
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

#include "shader_include.h"
#include "servers/rendering/shader_language.h"
#include "servers/rendering/shader_preprocessor.h"

void ShaderInclude::_dependency_changed() {
	emit_changed();
}

void ShaderInclude::set_code(const String &p_code) {
	HashSet<Ref<ShaderInclude>> new_dependencies;
	code = p_code;

	for (Ref<ShaderInclude> E : dependencies) {
		E->disconnect(SNAME("changed"), callable_mp(this, &ShaderInclude::_dependency_changed));
	}

	{
		String pp_code;
		ShaderPreprocessor preprocessor;
		preprocessor.preprocess(p_code, pp_code, nullptr, nullptr, &new_dependencies);
	}

	// This ensures previous include resources are not freed and then re-loaded during parse (which would make compiling slower)
	dependencies = new_dependencies;

	for (Ref<ShaderInclude> E : dependencies) {
		E->connect(SNAME("changed"), callable_mp(this, &ShaderInclude::_dependency_changed));
	}

	emit_changed();
}

String ShaderInclude::get_code() const {
	return code;
}

void ShaderInclude::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_code", "code"), &ShaderInclude::set_code);
	ClassDB::bind_method(D_METHOD("get_code"), &ShaderInclude::get_code);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "code", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR), "set_code", "get_code");
}

// ResourceFormatLoaderShaderInclude

Ref<Resource> ResourceFormatLoaderShaderInclude::load(const String &p_path, const String &p_original_path, Error *r_error, bool p_use_sub_threads, float *r_progress, CacheMode p_cache_mode) {
	if (r_error) {
		*r_error = ERR_FILE_CANT_OPEN;
	}

	Ref<ShaderInclude> shader_inc;
	shader_inc.instantiate();

	Vector<uint8_t> buffer = FileAccess::get_file_as_array(p_path);

	String str;
	str.parse_utf8((const char *)buffer.ptr(), buffer.size());

	shader_inc->set_code(str);

	if (r_error) {
		*r_error = OK;
	}

	return shader_inc;
}

void ResourceFormatLoaderShaderInclude::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("gdshaderinc");
}

bool ResourceFormatLoaderShaderInclude::handles_type(const String &p_type) const {
	return (p_type == "ShaderInclude");
}

String ResourceFormatLoaderShaderInclude::get_resource_type(const String &p_path) const {
	String extension = p_path.get_extension().to_lower();
	if (extension == "gdshaderinc") {
		return "ShaderInclude";
	}
	return "";
}

// ResourceFormatSaverShaderInclude

Error ResourceFormatSaverShaderInclude::save(const String &p_path, const Ref<Resource> &p_resource, uint32_t p_flags) {
	Ref<ShaderInclude> shader_inc = p_resource;
	ERR_FAIL_COND_V(shader_inc.is_null(), ERR_INVALID_PARAMETER);

	String source = shader_inc->get_code();

	Error error;
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE, &error);

	ERR_FAIL_COND_V_MSG(error, error, "Cannot save shader include '" + p_path + "'.");

	file->store_string(source);
	if (file->get_error() != OK && file->get_error() != ERR_FILE_EOF) {
		return ERR_CANT_CREATE;
	}

	return OK;
}

void ResourceFormatSaverShaderInclude::get_recognized_extensions(const Ref<Resource> &p_resource, List<String> *p_extensions) const {
	const ShaderInclude *shader_inc = Object::cast_to<ShaderInclude>(*p_resource);
	if (shader_inc != nullptr) {
		p_extensions->push_back("gdshaderinc");
	}
}

bool ResourceFormatSaverShaderInclude::recognize(const Ref<Resource> &p_resource) const {
	return p_resource->get_class_name() == "ShaderInclude"; //only shader, not inherited
}
