#version 330 core
in vec2 TexCoords;
out vec4 color;
uniform float uA;
uniform bool isError;
uniform float lightIntensity;


uniform sampler2D text;
uniform vec3 textColor;

void main()
{    
    if(isError){
        vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
        color = vec4(textColor, uA) * sampled*lightIntensity;
        
    } else {

        vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
        color = vec4(textColor, 1.0) * sampled * lightIntensity;
    }
}  