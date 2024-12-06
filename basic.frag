#version 330 core


in vec2 chTex;  // Koordinate teksture
uniform sampler2D uTex; 
out vec4 outCol;  // Izlazna boja
uniform float uA;

uniform vec4 color;  // Uniforma za boje (sivi pravougaonik)
uniform bool isFrame; 
uniform bool isDoor;
uniform bool isLightBulb;
uniform bool isIndicator;
uniform float lightIntensity;


void main()
{
	

	if(isDoor == true){
		if(chTex. x < 0.9 && chTex. y < 0.9 && chTex.x > 0.1 && chTex.y > 0.1)
		{
				
				outCol = vec4(0.0f, 0.5f, 1.0f, 0.4f)*lightIntensity;
			
		}
		else 
		{
			outCol = color*lightIntensity;
		}
	} else if(isLightBulb){
		outCol = color*lightIntensity;

	}else if (isFrame) {
		if(chTex. x < 0.9 && chTex. y < 0.9 && chTex.x > 0.1 && chTex.y > 0.1)
		{
			outCol = vec4(0.3, 0.3, 0.3, 1.0)*lightIntensity;
		}
		else 
		{
			outCol = color*lightIntensity;
		}

	} else if(isIndicator) {
		
		outCol = vec4(uA, 0.0, 0.0, 1.0)*lightIntensity;
	}
	else {
		outCol = color*lightIntensity;
	}


	
	
    

}