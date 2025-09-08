///////////////////////////////////////////////////////////////////////////////
///
///	@file Graphics.h
/// 
/// @brief Declaration of graphics.cpp
///	
///	@Authors: Victor lee
///	Copyright 2024, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////

#pragma once //Makes sure this header is only included once
#ifndef _GRAPHICS_H_
#define _GRAPHICS_H_

#include <glew.h>
#include <vec2.hpp>
#include <array>
#include <iostream>
#include <sstream>
#include <iomanip> 
#include <gtc/constants.hpp>
#include <cstdlib>
#include <unordered_map> 
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>  

//FILES
#include "System.h"
#include "GraphicsWindows.h"
#include <GraphicsShader.h>
#include <InputHandler.h>
#include <PhysicsSystem.h>
#include <FontSystem.h>
#include "AssetManager.h"
#include <ComponentList.h>
#include "imgui.h"

namespace Framework {

	class Graphics : public ISystem
	{
	public:
		Graphics(GraphicsWindows* graphicWindows);
		~Graphics();

		void Initialize() override;
		void Update(float deltaTime) override;
		std::string GetName() override;

		std::string OpenFileDialog();
		std::string SaveFileDialog();

		static void DropCallback(GLFWwindow* window, int count, const char** paths);
		static void RenderErrorPopup();
		std::wstring GetCurrentWorkingDirectory();
		void ChangeWorkingDirectory(const std::wstring& newDirectory);
		std::string ExtractBasePath(const std::string& filePath);
		std::wstring ExtractParentDirectory(const std::wstring& directory, int levels);
		static bool IsMouseOutsideViewport(ImVec2 viewportMin, ImVec2 viewportMax);
		void RenderFPS(float projWidth, float projHeight);

		//linking
		GraphicsWindows* graphicWindows;
		InputHandler* InputHandlerInstance;
		FontSystem fontSystem; // Define the instance

		// Sorting Layer Variables
		unsigned int CurrentSize = static_cast<unsigned int>(-1);
		
		// Main viewport storing
		static float viewportWidth;
		static float viewportHeight;
		static float viewportOffsetX;
		static float viewportOffsetY;
		static std::vector<Entity> sortedEntities;


		class Model
		{
			public:
				GLenum primitive_type{};
				GLuint primitive_cnt{};
				GLuint vaoid{};
				GLuint vbo_hdl{};
				GLuint ebo_hdl{};
				GLuint tex_vbo_hdl{};
				GLuint draw_cnt{};
				UE_Shader shdr_pgm{};
				glm::vec3 color{};
				float alpha{};
				GLuint textureID{};
				glm::mat4 modelMatrix{};
				glm::mat4 viewMatrix{};
				glm::mat4 projectionMatrix{};
				std::string name{};
				bool isMovable = false; // New flag for movability

				glm::vec2 translation = { 0.0f, 0.0f };
				float rotation = 0.0f;
				glm::vec2 scale = { 1.0f, 1.0f };

				void draw();
				void setup_shdrpgm(std::string const& vtx_shdr, std::string const& frag_shdr);
				void cleanup();
		};

		class Camera
		{
		public:
			glm::vec2 position;     // Camera position in world coordinates
			float zoom;             // Zoom factor
			glm::vec2 viewportSize; // Size of the viewport

			// Constructor
			Camera(const glm::vec2& viewportSize) : position(0.0f, 0.0f), zoom(1.0f), viewportSize(viewportSize) {}

			glm::vec3 getPosition() const
			{
				return glm::vec3(position, 0.0f); // Convert to 3D vector (z = 0 for 2D camera)
			}

			// Returns the 2D view matrix
			glm::mat4 getViewMatrix() const;

			// Get the projection matrix
			glm::mat4 getProjectionMatrix() const
			{
				// Calculate an orthographic projection matrix based on the viewport size
				float halfWidth = viewportSize.x * 0.5f / zoom;
				float halfHeight = viewportSize.y * 0.5f / zoom;

				// Left, right, bottom, top boundaries
				float left = -halfWidth;
				float right = halfWidth;
				float bottom = -halfHeight;
				float top = halfHeight;

				// Create an orthographic projection matrix
				return glm::ortho(left, right, bottom, top, -1.0f, 1.0f);
			}

			void move(const glm::vec2& delta);

			void setZoom(float newZoom);

			void setPosition(const glm::vec2& newPosition);

		};

		// CONTAINERS
		static std::vector<Model> models;
		static std::unordered_map<std::string, GLuint> textures;
		static std::unordered_map<std::string, Model> meshes;
		static Camera camera;

		//creating frame buffer for imGUI
		static GLuint gameFramebuffer, gameTexture, rbo, pickingFBO;
		static unsigned char* data;
		static float projHeight;
		static float projWidth;
		static float projMousex;
		static float projMousey;

		//Particles emit temporary
		static float EmitX;
		static float EmitY;

		/*******    MODEL FUNCTIONS    ********/
		//Points Model
		static Model points_model(const glm::vec2& coordinate, const glm::vec3& clr_vtx);

		//Lines Model
		static Model lines_model(const std::pair<glm::vec2, glm::vec2>& line_segment, const glm::vec3& clr_vtx);

		//Circle Model
		static Model trifans_model(float radius, float centerX, float centerY, const glm::vec3& clr_vtx);

		//Square Model
		static Model sqaure_model(glm::vec2 position, float width, float height, const glm::vec3& clr_vtx);

		//Debug Model Drawing capability
		static void DrawDebugBox(const glm::vec2& center, float width, float height);

			//Creating Frame Buffer for imGUI
		static GLuint CreateFramebuffer(int width, int height, GLuint& texture, GLuint& rbo);


		//** USAGE FUNCTIONS ** //

		//Functions below are used by clients to create a game.
		//Doxygen-level file documentation has been provided to aid clients in using functions.

		/**
		 * @brief Draws a circle on the viewport.
		 *
		 * This function draws a circle at the specified position and radius
		 * in non-normalized device coordinates (non-NDC).
		 *
		 * @param [float] radius The radius of the circle in non-NDC value.
		 * @param [float] centerX The X-coordinate of the circle's center in non-NDC value.
		 * @param [float] centerY The Y-coordinate of the circle's center in non-NDC value.
		 * @param [const glm::vec3&] color The color of the circle in RGB, represented as a glm::vec3.
		 */
		static void DrawCircle(float radius, float centerX, float centerY, const glm::vec3& clr_vtx);

		/**
		 * @brief Draws a line segment on the viewport.
		 *
		 * This function draws a line between two specified points in non-NDC coordinates.
		 *
		 * @param [const std::pair<glm::vec2, glm::vec2>&] line_segment A pair of glm::vec2 representing the start and end points of the line.
		 * @param [const glm::vec3&] color The color of the line in RGB, represented as a glm::vec3.
		 */
		static void DrawLine(const std::pair<glm::vec2, glm::vec2>& line_segment, const glm::vec3& clr_vtx);

		/**
		 * @brief Draws a point on the viewport.
		 *
		 * This function draws a single point at the specified coordinates in non-NDC coordinates.
		 *
		 * @param [const glm::vec2&] coordinates The X and Y coordinates of the point in non-NDC value (glm::vec2).
		 * @param [const glm::vec3&] color The color of the point in RGB, represented as a glm::vec3.
		 */
		static void DrawPoint(const glm::vec2& coordinates, const glm::vec3& clr_vtx);


		/**
		 * @brief Draws a square on the viewport.
		 *
		 * This function draws a square at a given position, with specified width and height, in non-NDC coordinates.
		 *
		 * @param [glm::vec2] position The X and Y coordinates of the top-left corner of the square in non-NDC value (glm::vec2).
		 * @param [float] width The width of the square.
		 * @param [float] height The height of the square.
		 * @param [const glm::vec3&] color The color of the square in RGB, represented as a glm::vec3.
		 */
		static void DrawSquare(glm::vec2 position, float width, float height, const glm::vec3& clr_vtx);

		/**
		 * @brief Creates and loads a mesh for rendering a game object.
		 *
		 * This function creates a mesh using vertex coordinates, texture coordinates,
		 * and vertex color for rendering game objects. The mesh is associated with a
		 * texture file and a unique name.
		 *
		 * @param vtx_coord : A vector of 2D vertex coordinates defining the shape of the mesh.
		 * @param txt_coords : A vector of 2D texture coordinates mapping textures to the mesh vertices.
		 * @param clr_vtx : A 3D vector representing the color to be applied to each vertex (RGB values).
		 * @param meshName : A string representing the name of the mesh, used for identification.
		 * @param textureFilePath : A string representing the path to the texture file to be applied to the mesh.
		 *
		 * @return A `Graphics::Model` object representing the created mesh, ready for rendering.
		 */
		static Graphics::Model createMesh(const std::vector<glm::vec2>& vtx_coord,
			const std::vector<glm::vec2>& txt_coords,
			const glm::vec3& clr_vtx,
			const std::string& meshName,
			const std::string& textureFilePath);


		/**
		 * @brief Retrieves a mesh by its name.
		 *
		 * This function returns a reference to a previously created mesh identified by the provided name.
		 * It ensures that the mesh exists in the `meshes` collection.
		 *
		 * @param name : A string representing the name of the mesh to retrieve.
		 *
		 * @return A reference to the `Graphics::Model` object corresponding to the mesh with the given name.
		 *         If the mesh does not exist, behavior depends on the implementation (e.g., could throw an exception or return a default mesh).
		 */
		static Graphics::Model& getMesh(const std::string& name);


		/**
		 * @brief Drawing meshes with animation spritesheet
		 *
		 * This function must be paired with a Mesh Creation function and use before .draw()
		 *
		 * @param mdl : Model binded to mesh
		 *
		 * @param currentFrame = (deltaTime * speed(float)) % number_of_sprite_images_per_png
		 *
		 */
		static void drawMeshWithAnimation(Graphics::Model& mdl, int currentFrame, int cols, int rows);

		/**
		 * @brief Calculate 2D Transformation With Scale, Rotate and Translate;
		 *
		 * This function is used to combine all SRT together
		 *
		 * @param [const glm::vec2&] translation value.
		 * @param [float]            rotation value.
		 * @param [const glm::vec2&] scaling value.
		 */
		static glm::mat4 calculate2DTransform(const glm::vec2& translation, float rotation, const glm::vec2& scale);


		/*
		@brief Setting Background Color
		 *
		 * This function is used in initialise function after initialising GLFW.
		 *
		 * @param r : 256-bit RED color
		 * @param g : 256-bit GREEN color
		 * @param b : 256-bit BLUE color
		 * @param alpha : set alpha
		*/
		static void SetBackgroundColor(int r, int g, int b, GLclampf alpha);

		//CAMERA SHAKE
		static void updateShake(float deltaTime);
		void startShake(float duration, float magnitude);


		//imgui
		void showImGUI();
		static bool toggleImGUI;
	};
}
#endif // !_GRAPHICS_H_