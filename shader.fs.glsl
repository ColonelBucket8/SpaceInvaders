#version 330

uniform sampler2D buffer;
noperspective in vec2 TexCoord;

out vec3 outColor;

void main(void)
{
	outColor = texture(buffer, TexCoord).rgb;
};