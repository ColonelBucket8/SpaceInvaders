#pragma once
#pragma once

#include <glad/glad.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>


class OurShader
{
public:
	// the program ID
	unsigned int ID;

	// constructor reads and builds the shader
	OurShader(const char* vertexPath, const char* fragmentPath);
	// use/activade the shader
	void use();
	// utility uniform functions
	void setBool(const std::string& name, bool value) const;
	void setInt(const std::string& name, int value) const;
	void setFloat(const std::string& name, float value) const;
	/*void setMat4(const std::string& name, glm::mat4& value) const;
	void setVec3(const std::string& name, glm::vec3& value) const;
	void setVec3(const std::string& name, const float x, const float y,
		const float z) const;*/

private:
	void checkCompileErrors(unsigned int shader, std::string type);
};
