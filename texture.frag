#version 330 core


in vec2 chTex;  // Koordinate teksture
uniform sampler2D uTex; 
out vec4 outCol;  // Izlazna boja


uniform vec4 lightColor;
uniform bool isLightOn;
uniform bool isFood;
uniform bool isSmoke;
uniform float lightIntensity;
uniform float smokeIntensity;



void main()
{
	outCol = texture(uTex, chTex) *lightIntensity; 

	if(isLightOn && isFood) {
		outCol = texture(uTex, chTex) * lightColor * 0.9;
	}

	if(isSmoke){
		outCol = texture(uTex, chTex)*smokeIntensity; 
	}


		
    

}