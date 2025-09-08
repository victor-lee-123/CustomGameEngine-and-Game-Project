///////////////////////////////////////////////////////////////////////////////
///
///	@file GraphicsShader.h
/// 
/// @brief Header files for Graphics Shader.
///	
///	@Authors: Victor lee
///	Copyright 2024, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////

#ifndef GRAPHICSSHADER_H
#define GRAPHICSSHADER_H

/*                                                                   includes
----------------------------------------------------------------------------- */
#include <glew.h> // for access to OpenGL API declarations 
#include <glm.hpp>
#include <string>
#include <vector>

/*  _________________________________________________________________________ */
class UE_Shader {
  /*! GLShader class.
  */
public:

  // default constructor required to initialize GLShader object to safe state
    UE_Shader() : pgm_handle(0), is_linked(GL_FALSE) {}

    bool LoadFromFiles(const std::string& vertexPath, const std::string& fragPath);
/**
    * @brief Compiles individual shader sources and links multiple shader objects
    *        to create an executable shader program.
    *
    * This function requires the full path to each shader source file and the
    * type of shader program (vertex, fragment, geometry, or tessellation)
    * in the form of an std::pair object. The pairs are supplied in a
    * std::vector. The function implements the six steps described in
    * CompileShaderFromFile() for each shader file. After creating shader objects,
    * it calls Link() to create a shader executable program, followed by Validate()
    * to ensure the program can execute in the current OpenGL state.
    *
    * @param[in] shaderPairs A vector of pairs where each pair consists of
    *                        the shader type (GLenum) and the file path
    *                        (std::string) of the shader source.
    * @return GLboolean Returns GL_TRUE if the program was successfully
    *                   compiled, linked, and validated; GL_FALSE otherwise.
    */
  GLboolean CompileLinkValidate(std::vector<std::pair<GLenum, std::string>>);


  /**
   * @brief Compiles a shader from a file and attaches it to the shader program.
   *
   * This function performs the following steps:
   * 1. Creates a shader program object if one doesn't exist.
   * 2. Creates a shader object using the specified shader type.
   * 3. Loads shader source code from the file specified by the second parameter.
   * 4. Compiles the shader source using CompileShaderFromString.
   * 5. Checks the compilation status and logs any messages to the member
   *    "log_string".
   * 6. If compilation is successful, attaches the shader object to the
   *    previously created shader program object.
   *
   * @param[in] shader_type The type of shader to be compiled (e.g., GL_VERTEX_SHADER).
   * @param[in] file_name The full path to the shader source file.
   * @return GLboolean Returns GL_TRUE if compilation and attachment are successful;
   *                   GL_FALSE otherwise.
   */
  GLboolean CompileShaderFromFile(GLenum shader_type, std::string const& file_name);


  /**
   * @brief Compiles a shader from a string and attaches it to the shader program.
   *
   * This function performs the following steps:
   * 1. Creates a shader program object if one doesn't exist.
   * 2. Creates a shader object using the specified shader type.
   * 3. Loads the shader code from the provided shader source string.
   * 4. Compiles the shader source.
   * 5. Checks the compilation status and logs any messages to the member
   *    "log_string".
   * 6. If compilation is successful, attaches the shader object to the
   *    previously created shader program object.
   *
   * @param[in] shader_type The type of shader to be compiled (e.g., GL_VERTEX_SHADER).
   * @param[in] shader_src The source code of the shader as a string.
   * @return GLboolean Returns GL_TRUE if compilation and attachment are successful;
   *                   GL_FALSE otherwise.
   */
  GLboolean CompileShaderFromString(GLenum shader_type, std::string const& shader_src);


  /**
   * @brief Links shader objects attached to the program handle.
   *
   * This function links the shader objects and verifies the status of
   * the link operation. If the linking fails, it retrieves and writes
   * the program object's information log to the member "log_string".
   *
   * @return GLboolean Returns GL_TRUE if linking is successful; GL_FALSE otherwise.
   */
  GLboolean Link();


  /**
   * @brief Activates the shader program for use in rendering.
   *
   * This function installs the shader program object whose handle is
   * encapsulated by the member variable.
   */
  void Use();


  /**
   * @brief Deactivates the currently installed shader program.
   *
   * This function effectively sets the current rendering state to refer
   * to an invalid program object.
   */
  void UnUse();


  /**
   * @brief Validates the executable shader program against the current OpenGL state.
   *
   * This function checks whether the shader program can execute in the
   * current OpenGL state.
   *
   * @return GLboolean Returns GL_TRUE if validation succeeds; GL_FALSE otherwise.
   */
  GLboolean Validate();


/**
    * @brief Retrieves the handle to the shader program object.
    *
    * @return GLuint The handle to the shader program object.
    */
  GLuint GetHandle() const;


  /**
   * @brief Checks if the shader program has been linked.
   *
   * @return GLboolean Returns GL_TRUE if the shader program is linked;
   *                   GL_FALSE otherwise.
   */
  GLboolean IsLinked() const;


  /**
   * @brief Retrieves the logged information from the GLSL compiler and linker.
   *
   * This function returns information obtained after calling Validate().
   *
   * @return std::string The log string containing compiler, linker, and
   *                     validation information.
   */
  std::string GetLog() const;


  /**
   * @brief Binds a generic vertex attribute index to a named attribute variable.
   *
   * This function associates a generic vertex attribute index with a named
   * input attribute variable. If layout qualifiers are not used, this
   * function can be utilized to create the association dynamically.
   *
   * @param[in] index The index of the vertex attribute.
   * @param[in] name The name of the attribute variable in the shader.
   */
  void BindAttribLocation(GLuint index, GLchar const* name);


  /**
   * @brief Binds a fragment shader index location to a user-defined output variable.
   *
   * This function associates a fragment shader index location that a
   * user-defined out variable will write to. If layout qualifiers are not
   * used, this function can be utilized to create the association dynamically.
   *
   * @param[in] color_number The index location for the output variable.
   * @param[in] name The name of the output variable in the fragment shader.
   */
  void BindFragDataLocation(GLuint color_number, GLchar const* name);


  /**
   * @brief Deletes the shader program object.
   *
   * This function deletes the program object associated with the shader.
   */
  void DeleteShaderProgram();


  /**
   * @brief Displays the active vertex attributes used by the vertex shader.
   *
   * This function is used for debugging purposes and should be called
   * only when necessary.
   */
  void PrintActiveAttribs() const;


  /**
   * @brief Displays the active uniform variables used by the shader program.
   *
   * This function is used for debugging purposes and should be called
   * only when necessary.
   */
  void PrintActiveUniforms() const;


  /**
   * @brief Retrieves the location of a uniform variable by name.
   *
   * This function returns the location of a uniform variable using
   * the program handle encapsulated by this class. If exit_on_error
   * is true, the program will terminate if the uniform variable is not found.
   *
   * @param[in] name The name of the uniform variable.
   * @param[in] exit_on_error Flag indicating whether to exit on error.
   * @return GLint The location of the uniform variable, or -1 if not found.
   */
  GLint GetUniformLocation(GLchar const* name, bool exit_on_error = false);


  /**
   * @brief Checks if a file exists at the specified relative path.
   *
   * This function is a debugging tool that checks the existence of
   * the file given in the relative path.
   *
   * @param[in] file_name The name of the file to check for existence.
   * @return GLboolean Returns GL_TRUE if the file exists; GL_FALSE otherwise.
   */
  GLboolean FileExists(std::string const& file_name);

  /**
     * @brief Retrieves the source code of the shader.
     * @param shaderType The type of shader (GL_VERTEX_SHADER or GL_FRAGMENT_SHADER).
     * @return The source code as a string.
     */
  std::string GetShaderSource(GLenum shaderType) const;

private:
  enum ShaderType 
  {
    VERTEX_SHADER = GL_VERTEX_SHADER,
    FRAGMENT_SHADER = GL_FRAGMENT_SHADER,
    GEOMETRY_SHADER = GL_GEOMETRY_SHADER,
    TESS_CONTROL_SHADER = GL_TESS_CONTROL_SHADER,
    TESS_EVALUATION_SHADER = GL_TESS_EVALUATION_SHADER,
  };

  GLuint pgm_handle = 0;  
  GLboolean is_linked = GL_FALSE;
  std::string log_string; // log for OpenGL compiler and linker messages

  std::string vertexSourceCode; // Store vertex shader source code
  std::string fragmentSourceCode; // Store fragment shader source code
};

#endif
