///////////////////////////////////////////////////////////////////////////////
///
///	@file GraphicsShader.cpp
/// 
/// @brief GraphicsShader.cpp provides the function definition for
///        GLSL shaders for linking, merging and creating shader program handles.
///	
///	@Authors: Victor lee
///	Copyright 2024, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include <GraphicsShader.h>
#include <glew.h>
#include <iostream>
#include <fstream>
#include <sstream>

GLint
UE_Shader::GetUniformLocation(GLchar const* name, bool exit_on_error) 
{
    GLint location = glGetUniformLocation(pgm_handle, name);
    if (location < 0) 
    {
        std::cout << "Uniform variable " << name << " doesn't exist" << std::endl;

        if (exit_on_error) std::exit(EXIT_FAILURE);
    }
    return location;
}

GLboolean
UE_Shader::FileExists(std::string const& file_name)
{
  std::ifstream infile(file_name); return infile.good();
}

std::string UE_Shader::GetShaderSource(GLenum shaderType) const
{
    // Assuming you have member variables to store source code
    if (shaderType == GL_VERTEX_SHADER)
    {
        return vertexSourceCode; // Assuming vertexSourceCode stores the vertex shader's source
    }
    else if (shaderType == GL_FRAGMENT_SHADER)
    {
        return fragmentSourceCode; // Assuming fragmentSourceCode stores the fragment shader's source
    }
    return ""; // Return an empty string if shaderType is unrecognized
}

void
UE_Shader::DeleteShaderProgram()
{
  if (pgm_handle > 0) {
    glDeleteProgram(pgm_handle);
  }
}

bool UE_Shader::LoadFromFiles(const std::string& vertexPath, const std::string& fragPath)
{
    std::vector<std::pair<GLenum, std::string>> shaders = {
        { GL_VERTEX_SHADER, vertexPath },
        { GL_FRAGMENT_SHADER, fragPath }
    };

    for (const auto& shader : shaders) {
        if (!CompileShaderFromFile(shader.first, shader.second)) {
            std::cout << "Failed to load shader: " << shader.second << std::endl;
            return false;
        }
    }

    if (!Link() || !Validate()) {
        std::cout << "Shader program linking or validation failed!" << std::endl;
        return false;
    }

    return true;
}

GLboolean
UE_Shader::CompileLinkValidate(std::vector<std::pair<GLenum, std::string>> vec)
{
  for (auto& elem : vec) 
  {
    if (GL_FALSE == CompileShaderFromFile(elem.first, elem.second.c_str())) 
    {
      return GL_FALSE;
    }
  }
  if (GL_FALSE == Link()) {
    return GL_FALSE;
  }
  if (GL_FALSE == Validate()) {
    return GL_FALSE;
  }
  PrintActiveAttribs();
  PrintActiveUniforms();

  return GL_TRUE;
}

GLboolean
UE_Shader::CompileShaderFromFile(GLenum shader_type, const std::string& file_name) {
  if (GL_FALSE == FileExists(file_name)) {
    log_string = "File not found";
    return GL_FALSE;
  }
  if (pgm_handle <= 0) {
    pgm_handle = glCreateProgram();
    if (0 == pgm_handle) {
      log_string = "Cannot create program handle";
      return GL_FALSE;
    }
  }

  std::ifstream shader_file(file_name, std::ifstream::in);
  if (!shader_file) {
    log_string = "Error opening file " + file_name;
    return GL_FALSE;
  }
  std::stringstream buffer;
  buffer << shader_file.rdbuf();
  shader_file.close();
  return CompileShaderFromString(shader_type, buffer.str());
}

GLboolean
UE_Shader::CompileShaderFromString(GLenum shader_type,
  const std::string& shader_src) {
  if (pgm_handle <= 0) {
    pgm_handle = glCreateProgram();
    if (0 == pgm_handle) {
      log_string = "Cannot create program handle";
      return GL_FALSE;
    }
  }

  GLuint shader_handle = 0;
  switch (shader_type) {
  case VERTEX_SHADER: shader_handle = glCreateShader(GL_VERTEX_SHADER); break;
  case FRAGMENT_SHADER: shader_handle = glCreateShader(GL_FRAGMENT_SHADER); break;
  case GEOMETRY_SHADER: shader_handle = glCreateShader(GL_GEOMETRY_SHADER); break;
  case TESS_CONTROL_SHADER: shader_handle = glCreateShader(GL_TESS_CONTROL_SHADER); break;
  case TESS_EVALUATION_SHADER: shader_handle = glCreateShader(GL_TESS_EVALUATION_SHADER); break;
    //case COMPUTE_SHADER: shader_handle = glCreateShader(GL_COMPUTE_SHADER); break;
  default:
    log_string = "Incorrect shader type";
    return GL_FALSE;
  }

  // load shader source code into shader object
  GLchar const* shader_code[] = { shader_src.c_str() };
  glShaderSource(shader_handle, 1, shader_code, NULL);

  // compile the shader
  glCompileShader(shader_handle);

  // check compilation status
  GLint comp_result;
  glGetShaderiv(shader_handle, GL_COMPILE_STATUS, &comp_result);
  if (GL_FALSE == comp_result) {
    log_string = "Vertex shader compilation failed\n";
    GLint log_len;
    glGetShaderiv(shader_handle, GL_INFO_LOG_LENGTH, &log_len);
    if (log_len > 0) {
      GLchar* log = new GLchar[log_len];
      GLsizei written_log_len;
      glGetShaderInfoLog(shader_handle, log_len, &written_log_len, log);
      log_string += log;
      delete[] log;
    }
    return GL_FALSE;
  } else { // attach the shader to the program object
    glAttachShader(pgm_handle, shader_handle);
    return GL_TRUE;
  }
}

GLboolean UE_Shader::Link() {
  if (GL_TRUE == is_linked) {
    return GL_TRUE;
  }
  if (pgm_handle <= 0) {
    return GL_FALSE;
  }

  glLinkProgram(pgm_handle); // link the various compiled shaders

  // verify the link status
  GLint lnk_status;
  glGetProgramiv(pgm_handle, GL_LINK_STATUS, &lnk_status);
  if (GL_FALSE == lnk_status) {
    log_string = "Failed to link shader program\n";
    GLint log_len;
    glGetProgramiv(pgm_handle, GL_INFO_LOG_LENGTH, &log_len);
    if (log_len > 0) {
      GLchar* log_str = new GLchar[log_len];
      GLsizei written_log_len;
      glGetProgramInfoLog(pgm_handle, log_len, &written_log_len, log_str);
      log_string += log_str;
      delete[] log_str;
    }
    return GL_FALSE;
  }
  return is_linked = GL_TRUE;
}

void UE_Shader::Use() {
  if (pgm_handle > 0 && is_linked == GL_TRUE) {
    glUseProgram(pgm_handle);
  }
}

void UE_Shader::UnUse() {
  glUseProgram(0);
}

GLboolean UE_Shader::Validate() {
  if (pgm_handle <= 0 || is_linked == GL_FALSE) {
    return GL_FALSE;
  }

  glValidateProgram(pgm_handle);
  GLint status;
  glGetProgramiv(pgm_handle, GL_VALIDATE_STATUS, &status);
  if (GL_FALSE == status) {
    log_string = "Failed to validate shader program for current OpenGL context\n";
    GLint log_len;
    glGetProgramiv(pgm_handle, GL_INFO_LOG_LENGTH, &log_len);
    if (log_len > 0) {
      GLchar* log_str = new GLchar[log_len];
      GLsizei written_log_len;
      glGetProgramInfoLog(pgm_handle, log_len, &written_log_len, log_str);
      log_string += log_str;
      delete[] log_str;
    }
    return GL_FALSE;
  } else {
    return GL_TRUE;
  }
}

GLuint UE_Shader::GetHandle() const {
  return pgm_handle;
}

GLboolean UE_Shader::IsLinked() const {
  return is_linked;
}

std::string UE_Shader::GetLog() const {
  return log_string;
}

void UE_Shader::BindAttribLocation(GLuint index, GLchar const* name) {
  glBindAttribLocation(pgm_handle, index, name);
}

void UE_Shader::BindFragDataLocation(GLuint color_number, GLchar const* name) {
  glBindFragDataLocation(pgm_handle, color_number, name);
}

void UE_Shader::PrintActiveAttribs() const {
#if 1
  GLint max_length, num_attribs;
  glGetProgramiv(pgm_handle, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &max_length);
  glGetProgramiv(pgm_handle, GL_ACTIVE_ATTRIBUTES, &num_attribs);
  GLchar* pname = new GLchar[max_length];
  std::cout << "Index\t|\tName\n";
  std::cout << "----------------------------------------------------------------------\n";
  for (GLint i = 0; i < num_attribs; ++i) {
    GLsizei written;
    GLint size;
    GLenum type;
    glGetActiveAttrib(pgm_handle, i, max_length, &written, &size, &type, pname);
    GLint loc = glGetAttribLocation(pgm_handle, pname);
    std::cout << loc << "\t\t" << pname << std::endl;
  }
  std::cout << "----------------------------------------------------------------------\n";
  delete[] pname;
#else
  GLint numAttribs;
  glGetProgramInterfaceiv(pgm_handle, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, &numAttribs);
  GLenum properties[] = { GL_NAME_LENGTH, GL_TYPE, GL_LOCATION };
  std::cout << "Active attributes:" << std::endl;
  for (GLint i = 0; i < numAttribs; ++i) {
    GLint results[3];
    glGetProgramResourceiv(pgm_handle, GL_PROGRAM_INPUT, i, 3, properties, 3, NULL, results);

    GLint nameBufSize = results[0] + 1;
    GLchar* pname = new GLchar[nameBufSize];
    glGetProgramResourceName(pgm_handle, GL_PROGRAM_INPUT, i, nameBufSize, NULL, pname);
    //   std::cout << results[2] << " " << pname << " " << getTypeString(results[1]) << std::endl;
    std::cout << results[2] << " " << pname << " " << results[1] << std::endl;
    delete[] pname;
  }
#endif
}

void UE_Shader::PrintActiveUniforms() const {
  GLint max_length;
  glGetProgramiv(pgm_handle, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_length);
  GLchar* pname = new GLchar[max_length];
  GLint num_uniforms;
  glGetProgramiv(pgm_handle, GL_ACTIVE_UNIFORMS, &num_uniforms);
  std::cout << "Location\t|\tName\n";
  std::cout << "----------------------------------------------------------------------\n";
  for (GLint i = 0; i < num_uniforms; ++i) {
    GLsizei written;
    GLint size;
    GLenum type;
    glGetActiveUniform(pgm_handle, i, max_length, &written, &size, &type, pname);
    GLint loc = glGetUniformLocation(pgm_handle, pname);
    std::cout << loc << "\t\t" << pname << std::endl;
  }
  std::cout << "----------------------------------------------------------------------\n";
  delete[] pname;
}
