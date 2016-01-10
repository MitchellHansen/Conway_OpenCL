#version 330 core
in vec3 ourColor;
in vec2 TexCoord;

out vec4 color;

// Texture samplers
uniform sampler2D pixel_texture;

void main()
{
	// Linearly interpolate between both textures (second texture is only slightly combined)
	//color = vec4(1.0f, 0.5f, 0.2f, 1.0f);
	color = texture(pixel_texture, TexCoord);
}