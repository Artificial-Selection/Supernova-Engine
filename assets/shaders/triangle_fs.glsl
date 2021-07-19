#version 460 core

uniform sampler2D _DiffuseTexture;

in vec3 outColor;
in vec2 texCoord0;

out vec4 FragColor;


void main()
{
    vec3 textureColor = texture(_DiffuseTexture, texCoord0).xyz * 2.5;
    FragColor = vec4(textureColor, 1.0f);
    //FragColor = vec4(outColor, 1.0f);
}
