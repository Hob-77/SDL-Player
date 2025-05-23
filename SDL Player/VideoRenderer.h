#pragma once

#include <glad/glad.h>
#include <cstdint>

class VideoRenderer {
public:
	VideoRenderer(int width, int height);
	~VideoRenderer();

	void uploadFrame(uint8_t* data); // uploads raw RGB frame data
	void render();                   // draws the texture to the screen

private:
	void initGLObjects();

	int width, height;
	GLuint textureID = 0;
	GLuint VAO = 0, VBO = 0;
	GLuint shaderProgram = 0;
};