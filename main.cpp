//Autor: Nedeljko Tesanovic
//Opis: Primjer upotrebe tekstura

#define _CRT_SECURE_NO_WARNINGS
#define CRES 30 // Circle Resolution = Rezolucija kruga

#include <iostream>
#include <fstream>
#include <sstream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


#include <ft2build.h>
#include FT_FREETYPE_H 

//stb_image.h je header-only biblioteka za ucitavanje tekstura.
//Potrebno je definisati STB_IMAGE_IMPLEMENTATION prije njenog ukljucivanja
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <map>

unsigned int compileShader(GLenum type, const char* source);
unsigned int createShader(const char* vsSource, const char* fsSource);
static unsigned loadImageToTexture(const char* filePath); //Ucitavanje teksture, izdvojeno u funkciju
static void RenderText(unsigned int& shader, std::string text, float x, float y, float scale, glm::vec3 color);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
static void updateTimer();

/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
    unsigned int TextureID; // ID handle of the glyph texture
    glm::ivec2   Size;      // Size of glyph
    glm::ivec2   Bearing;   // Offset from baseline to left/top of glyph
    unsigned int Advance;   // Horizontal offset to advance to next glyph
};

std::map<GLchar, Character> Characters;
unsigned int letterVAO, letterVBO;


std::string displayText = "   :";
int digitCount = 0;  
bool isColon = false;  
bool inputLocked = false;  
int minutes = 0;
int seconds = 0;
bool isTimerRunning = false; 
float elapsedTime = 0.0f;    
float lastTime = 0.0f;

bool isDoorClosed = true;

int main(void)
{

    if (!glfwInit())
    {
        std::cout<<"GLFW Biblioteka se nije ucitala! :(\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window;
    unsigned int wWidth = 800;
    unsigned int wHeight = 800;
    const char wTitle[] = "[Generic Title]";
    window = glfwCreateWindow(wWidth, wHeight, wTitle, NULL, NULL);
    
    if (window == NULL)
    {
        std::cout << "Prozor nije napravljen! :(\n";
        glfwTerminate();
        return 2;
    }

    glfwMakeContextCurrent(window);


    if (glewInit() != GLEW_OK)
    {
        std::cout << "GLEW nije mogao da se ucita! :'(\n";
        return 3;
    }

    // FreeType
    // --------
    FT_Library ft;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return -1;
    }

    // load font as face
    FT_Face face;
    if (FT_New_Face(ft, "fonts/Turret_Road/TurretRoad-Regular.ttf", 0, &face))
    {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return -1;
    }
    else {
        // set size to load glyphs as
        FT_Set_Pixel_Sizes(face, 0, 48);

        // disable byte-alignment restriction
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // load first 128 characters of ASCII set
        for (unsigned char c = 0; c < 128; c++)
        {
            // Load character glyph 
            if (FT_Load_Char(face, c, FT_LOAD_RENDER))
            {
                std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
                continue;
            }
            // generate texture
            unsigned int texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
            );
            // set texture options
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            // now store character for later use
            Character character = {
                texture,
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                static_cast<unsigned int>(face->glyph->advance.x)
            };
            Characters.insert(std::pair<char, Character>(c, character));
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    // destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    glm::mat4 projection = glm::ortho(0.0f, 800.0f, 0.0f, 600.0f);


    // configure VAO/VBO for texture quads
    // -----------------------------------
    glGenVertexArrays(1, &letterVAO);
    glGenBuffers(1, &letterVBO);
    glBindVertexArray(letterVAO);
    glBindBuffer(GL_ARRAY_BUFFER, letterVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    unsigned int textShader = createShader("text.vert", "text.frag");
    glUseProgram(textShader);
    unsigned projectionLocation = glGetUniformLocation(textShader, "projection");
    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));
    glUseProgram(0);
    
    unsigned int unifiedShader = createShader("basic.vert", "basic.frag");
    unsigned int textureShader = createShader("basic.vert", "texture.frag");

    float vertices[] = //osnova
    {   //X    Y      S    T 
        -0.95, 0.6,   0.0, 1.0,
        -0.95, -0.6,    0.0, 0.0,
        0.4, 0.6,      1.0, 1.0, 
        0.4, -0.6,     1.0, 0.0,

    };

    float foodVertices[] = {
        // X      Y      S     T
        -0.7f, -0.15f,  0.0f, 1.0f, // Gornji levi   
        -0.7f,  -0.4f,  0.0f, 0.0f, // Donji levi
        0.1f,  -0.15f,  1.0f, 1.0f,  // Gornji desni        
        0.1f, -0.4f,  1.0f, 0.0f // Donji desni
        
    };

    float circle[(CRES + 2) * 2];
    float r = 0.05; //poluprecnik

    circle[0] = -0.275; //Centar X0
    circle[1] = 0.43; //Centar Y0
    int i;
    for (i = 0; i <= CRES; i++)
    {

        circle[2 + 2 * i] = r * cos((3.141592 / 180) * (i * 360 / CRES)) + circle[0]; //Xi (Matematicke funkcije rade sa radijanima)
        circle[2 + 2 * i + 1] = r * sin((3.141592 / 180) * (i * 360 / CRES)) + circle[1]; //Yi
    }

    float rightVertices[] = //osnovva desno
    {   //X    Y      S    T 
        0.4, 0.6,   0.0, 1.0,
        0.4, -0.6,    0.0, 0.0,
        0.95, 0.6,      1.0, 1.0,
        0.95, -0.6,     1.0, 0.0,

    };

    float displayVertices[] = 
    {   //X    Y      S    T 
        0.45, 0.55,   0.0, 1.0,
        0.45, 0.35,    0.0, 0.0,
        0.90, 0.55,      1.0, 1.0,
        0.90, 0.35,     1.0, 0.0,

    };

    float keyboardVertices[] = 
    {   //X    Y      S    T 
        0.45, 0.3,   0.0, 1.0, //gornji levi
        0.45, -0.1,    0.0, 0.0, //donji levi
        0.90, 0.3,      1.0, 1.0, //gornji desni
        0.90, -0.1,     1.0, 0.0, //donji desni

    };

    float indicator[(CRES + 2) * 2];
    float r1 = 0.02; //poluprecnik

    indicator[0] = 0.675; //Centar X0
    indicator[1] = -0.15; //Centar Y0
    for (i = 0; i <= CRES; i++)
    {

        indicator[2 + 2 * i] = r1 * cos((3.141592 / 180) * (i * 360 / CRES)) + indicator[0]; //Xi (Matematicke funkcije rade sa radijanima)
        indicator[2 + 2 * i + 1] = r1 * sin((3.141592 / 180) * (i * 360 / CRES)) + indicator[1]; //Yi
    }



    float buttonsVertices[] = 
    {   //X    Y      S    T 
        0.45, -0.2,   0.0, 1.0, //gornji levi
        0.45, -0.5,    0.0, 0.0, //donji levi
        0.90, -0.2,      1.0, 1.0, //gornji desni
        0.90, -0.5,     1.0, 0.0, //donji desni

    };

    float smokeVertices[] = //osnova
    {   //X    Y      S    T 
        -1.0, 1.0,   0.4, 0.2,
        -1.0, -1.0,    0.4, 0.0,
        1.0, 1.0,      0.6, 0.2,
        1.0, -1.0,     0.6, 0.0,

    };




    

    
    unsigned int stride = (2 + 2) * sizeof(float);

    unsigned int VAO[9];
    glGenVertexArrays(9, VAO);
    unsigned int VBO[9];
    glGenBuffers(9, VBO);

    //za osnovu
    glBindVertexArray(VAO[0]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
   
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);


    //za hranu
    glBindVertexArray(VAO[1]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(foodVertices), foodVertices, GL_STATIC_DRAW);  

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);  // Pozicija
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));  // Teksture
    glEnableVertexAttribArray(1);


    //za sijalicu
    glBindVertexArray(VAO[2]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(circle), circle, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);


    //za desni deo
    glBindVertexArray(VAO[3]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[3]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rightVertices), rightVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    //za displej
    glBindVertexArray(VAO[4]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[4]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(displayVertices), displayVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    //za tastaturu
    glBindVertexArray(VAO[5]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[5]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(keyboardVertices), keyboardVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    //za dugmice
    glBindVertexArray(VAO[6]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[6]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(buttonsVertices), buttonsVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    //za indikator
    glBindVertexArray(VAO[7]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[7]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(indicator), indicator, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //za dim
    glBindVertexArray(VAO[8]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[8]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(smokeVertices), smokeVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    


    

    // Oslobodi VBO, jer je već učitan u VAO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);  // Oslobodi VAO

    
    //Tekstura za hranu
    unsigned foodTexture = loadImageToTexture("res/food_plate.png"); //Ucitavamo teksturu
    glBindTexture(GL_TEXTURE_2D, foodTexture); //Podesavamo teksturu
    glGenerateMipmap(GL_TEXTURE_2D); //Generisemo mipmape 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);//S = U = X    GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);// T = V = Y
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);   //GL_NEAREST, GL_LINEAR
    glBindTexture(GL_TEXTURE_2D, 0);

    // Tekstura za tastaturu
    unsigned keyboardTexture = loadImageToTexture("res/keybord.png"); // Učitavamo teksturu
    glBindTexture(GL_TEXTURE_2D, keyboardTexture); // Podesavamo teksturu
    glGenerateMipmap(GL_TEXTURE_2D); // Generišemo mipmape 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); // S = U = X  GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER); // T = V = Y
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); //GL_NEAREST, GL_LINEAR
    glBindTexture(GL_TEXTURE_2D, 0);

    // Tekstura za dugmice
    unsigned buttonsTexture = loadImageToTexture("res/buttons.png"); // Učitavamo teksturu
    glBindTexture(GL_TEXTURE_2D, buttonsTexture); // Podesavamo 
    glGenerateMipmap(GL_TEXTURE_2D); // Generišemo mipmape 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); // S = U = X   GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER); // T = V = Y
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); //GL_NEAREST, GL_LINEAR
    glBindTexture(GL_TEXTURE_2D, 0);


    // Tekstura za dim
    unsigned smokeTexture = loadImageToTexture("res/smoke.png"); // Učitavamo teksturu
    glBindTexture(GL_TEXTURE_2D, smokeTexture); // Podesavamo teksturu
    glGenerateMipmap(GL_TEXTURE_2D); // Generišemo mipmape
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); // S = U = X   GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER); // T = V = Y
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); //GL_NEAREST, GL_LINEAR
    glBindTexture(GL_TEXTURE_2D, 0);



    glUseProgram(textureShader);
    unsigned uTexLoc = glGetUniformLocation(textureShader, "uTex");
    glUniform1i(uTexLoc, 0); // Indeks teksturne jedinice (sa koje teksture ce se citati boje)
    glUseProgram(0);
    //Odnosi se na glActiveTexture(GL_TEXTURE0) u render petlji
    //Moguce je sabirati indekse, tj GL_TEXTURE5 se moze dobiti sa GL_TEXTURE0 + 5 , sto je korisno za iteriranje kroz petlje


    float x = -0.0;
    float y = 0.0;
    unsigned int uPosLoc = glGetUniformLocation(unifiedShader, "uPos");
    unsigned int uWLoc = glGetUniformLocation(unifiedShader, "uW");
    unsigned int uHLoc = glGetUniformLocation(unifiedShader, "uH");


    
    bool isBlendingEnabled = true;
    bool isBroken = false;
    float lightIntensity = 1.0f;
    float smokeIntensity = 0.0f;
    glfwSetTime(0);
    while (!glfwWindowShouldClose(window))
    {
        
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }

        //otvaranje i zatvaranje vrata
        if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
            x = 0.0;
            isDoorClosed = true;
        }

        if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
            x = -1.2;
            isDoorClosed = false;
            isTimerRunning = false;

        }

        
        //smanjivanje tajmera
        if (isTimerRunning) {
            int t = glfwGetTime();
            if (t >= 1.0) {
                updateTimer();
                glfwSetTime(0);
            }          
           
        }

       
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSetMouseButtonCallback(window, mouse_button_callback);

        

        glUseProgram(unifiedShader); 
        glUniform1f(glGetUniformLocation(unifiedShader, "lightIntensity"), lightIntensity);
        

        //okvir
        
        glBindVertexArray(VAO[0]);
        glUniform1f(glGetUniformLocation(unifiedShader, "isLightBulb"), GL_FALSE);
        glUniform1f(glGetUniformLocation(unifiedShader, "isTexture"), GL_FALSE);
        glUniform1f(glGetUniformLocation(unifiedShader, "isDoor"), GL_FALSE);
        glUniform1f(glGetUniformLocation(unifiedShader, "isFrame"), GL_TRUE);
        glUniform4f(glGetUniformLocation(unifiedShader, "color"), 0.5f, 0.5f, 0.5f, 1.0f);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glUniform1f(glGetUniformLocation(unifiedShader, "isFrame"), GL_FALSE);



        //desni deo 
        glBindVertexArray(VAO[3]);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        //displej
        glBindVertexArray(VAO[4]);
        glUniform4f(glGetUniformLocation(unifiedShader, "color"), 0.0f, 0.0f, 0.0f, 1.0f);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        //indikator
        glBindVertexArray(VAO[7]);
        if (isTimerRunning) {
            glUniform1f(glGetUniformLocation(unifiedShader, "isIndicator"), GL_TRUE);
            float pulse = sin(glfwGetTime()) * 0.5 + 0.5;
            glUniform1f(glGetUniformLocation(unifiedShader, "uA"), pulse);
        }
        else {

            glUniform4f(glGetUniformLocation(unifiedShader, "color"), 0.0f, 0.0f, 0.0f, 1.0f);
        }
        glDrawArrays(GL_TRIANGLE_FAN, 0, sizeof(indicator) / (2 * sizeof(float)));
        glUniform1f(glGetUniformLocation(unifiedShader, "isIndicator"), GL_FALSE);


        





        
        //sijalica
        glBindVertexArray(VAO[2]);
        if (isTimerRunning) {
            glUniform4f(glGetUniformLocation(unifiedShader, "color"), 0.6f, 0.5f, 0.0f, 1.0f);
            
        }
        else {
            glUniform4f(glGetUniformLocation(unifiedShader, "color"), 0.5f, 0.5f, 0.5f, 1.0f);
        }
        
        glUniform1f(glGetUniformLocation(unifiedShader, "isLightBulb"), GL_TRUE);
        glDrawArrays(GL_TRIANGLE_FAN, 0, sizeof(circle) / (2 * sizeof(float)));
        glUniform1f(glGetUniformLocation(unifiedShader, "isLightBulb"), GL_FALSE);




        
        glUseProgram(textureShader);
        glUniform1f(glGetUniformLocation(textureShader, "lightIntensity"), lightIntensity);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        //tanjir sa hranom        
        glBindTexture(GL_TEXTURE_2D, foodTexture);
        glBindVertexArray(VAO[1]);
        glUniform1f(glGetUniformLocation(textureShader, "isFood"), GL_TRUE);
        if (isTimerRunning) {
            glUniform1f(glGetUniformLocation(textureShader, "isLightOn"), GL_TRUE);
            glUniform4f(glGetUniformLocation(textureShader, "lightColor"), 0.6f, 0.5f, 0.0f, 1.0f);

        }
        else {
            glUniform1f(glGetUniformLocation(textureShader, "isLightOn"), GL_FALSE);
        }
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glUniform1f(glGetUniformLocation(textureShader, "isFood"), GL_FALSE);

        //tastatura
        glBindTexture(GL_TEXTURE_2D, keyboardTexture);
        glBindVertexArray(VAO[5]);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        //dugmici
        glBindTexture(GL_TEXTURE_2D, buttonsTexture);
        glBindVertexArray(VAO[6]);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindTexture(GL_TEXTURE_2D, 0);

        //dim
        
        glUniform1f(glGetUniformLocation(textureShader, "isSmoke"), GL_TRUE);
        glBindTexture(GL_TEXTURE_2D, smokeTexture);
        glBindVertexArray(VAO[8]);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUniform1f(glGetUniformLocation(textureShader, "isSmoke"), GL_FALSE);
        
        glUniform1f(glGetUniformLocation(textureShader, "smokeIntensity"), smokeIntensity);

        


        glDisable(GL_BLEND);




        if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
            isBlendingEnabled = true;  
        }
        if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
            isBlendingEnabled = false; 
        }

        if (isBlendingEnabled) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
        }
        else {
            glDisable(GL_BLEND);  
        }


        


        //vrata
        glUseProgram(unifiedShader);
        glUniform4f(glGetUniformLocation(unifiedShader, "color"), 0.5f, 0.5f, 0.5f, 1.0f);
        glUniform2f(uPosLoc, x, y);
        glUniform1i(uWLoc, wWidth);
        glUniform1i(uHLoc, wHeight);
        glBindVertexArray(VAO[0]);
        glUniform1f(glGetUniformLocation(unifiedShader, "isDoor"), GL_TRUE);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);  
        glUniform1f(glGetUniformLocation(unifiedShader, "isDoor"), GL_FALSE);

        
        glUseProgram(textShader);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


        //kvarenje mikrotalasne
        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
            isBroken = true;
        }

        //servisiranje mikrotalasne
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            isBroken = false;
            displayText = "   :";
            minutes = 0;
            seconds = 0;
            digitCount = 0;
            isColon = false;
            inputLocked = false;
            isTimerRunning = false;
        }

        if (isBroken) {
            glUniform1f(glGetUniformLocation(textShader, "isError"), GL_TRUE);
            isTimerRunning = false;
            displayText = "";
            RenderText(textShader, "ERROR", 581.0, 420.0, 1.2f, glm::vec3(1.0f, 0.0f, 0.1f));
            float pulse1 = abs(sin(glfwGetTime())) * 10.0 ;
            glUniform1f(glGetUniformLocation(textShader, "uA"), pulse1);
            glUniform1f(glGetUniformLocation(textShader, "isError"), GL_FALSE);
            lightIntensity -= 0.0001f;  
            if (lightIntensity < 0.0f) lightIntensity = 0.0f;
            smokeIntensity += 0.001f;
            if (smokeIntensity > 1.0f) smokeIntensity = 1.0f;
        }

        if (!isBroken) {
            lightIntensity += 0.001f;
            if (lightIntensity > 1.0f) lightIntensity = 1.0f;
            smokeIntensity -= 0.0001f;
            if (smokeIntensity < 0.0f) smokeIntensity = 0.0f;
        }
        glUniform1f(glGetUniformLocation(textShader, "lightIntensity"), lightIntensity);
        
        
        
        RenderText(textShader, displayText, 582.0, 420.0, 1.2f, glm::vec3(0.0f, 0.2f, 0.7f));  
        RenderText(textShader, "Masa Mastilovic RA194/2021", 10.0, 10.0, 0.5f, glm::vec3(0.0, 0.2f, 0.7f));


        

        glDisable(GL_BLEND);

        glBindVertexArray(0);
        glUseProgram(0);
        

        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    glDeleteTextures(1, &foodTexture);
    glDeleteBuffers(1, VBO);
    glDeleteVertexArrays(1, VAO);
    glDeleteProgram(unifiedShader);

    glfwTerminate();
    return 0;
}

unsigned int compileShader(GLenum type, const char* source)
{
    std::string content = "";
    std::ifstream file(source);
    std::stringstream ss;
    if (file.is_open())
    {
        ss << file.rdbuf();
        file.close();
        std::cout << "Uspjesno procitao fajl sa putanje \"" << source << "\"!" << std::endl;
    }
    else {
        ss << "";
        std::cout << "Greska pri citanju fajla sa putanje \"" << source << "\"!" << std::endl;
    }
     std::string temp = ss.str();
     const char* sourceCode = temp.c_str();

    int shader = glCreateShader(type);
    
    int success;
    char infoLog[512];
    glShaderSource(shader, 1, &sourceCode, NULL);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        if (type == GL_VERTEX_SHADER)
            printf("VERTEX");
        else if (type == GL_FRAGMENT_SHADER)
            printf("FRAGMENT");
        printf(" sejder ima gresku! Greska: \n");
        printf(infoLog);
    }
    return shader;
}
unsigned int createShader(const char* vsSource, const char* fsSource)
{
    
    unsigned int program;
    unsigned int vertexShader;
    unsigned int fragmentShader;

    program = glCreateProgram();

    vertexShader = compileShader(GL_VERTEX_SHADER, vsSource);
    fragmentShader = compileShader(GL_FRAGMENT_SHADER, fsSource);

    
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program);
    glValidateProgram(program);

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_VALIDATE_STATUS, &success);
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(program, 512, NULL, infoLog);
        std::cout << "Objedinjeni sejder ima gresku! Greska: \n";
        std::cout << infoLog << std::endl;
    }

    glDetachShader(program, vertexShader);
    glDeleteShader(vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(fragmentShader);

    return program;
}
static unsigned loadImageToTexture(const char* filePath) {
    int TextureWidth;
    int TextureHeight;
    int TextureChannels;
    unsigned char* ImageData = stbi_load(filePath, &TextureWidth, &TextureHeight, &TextureChannels, 0);
    if (ImageData != NULL)
    {
        //Slike se osnovno ucitavaju naopako pa se moraju ispraviti da budu uspravne
        stbi__vertical_flip(ImageData, TextureWidth, TextureHeight, TextureChannels);

        // Provjerava koji je format boja ucitane slike
        GLint InternalFormat = -1;
        switch (TextureChannels) {
        case 1: InternalFormat = GL_RED; break;
        case 2: InternalFormat = GL_RG; break;
        case 3: InternalFormat = GL_RGB; break;
        case 4: InternalFormat = GL_RGBA; break;
        default: InternalFormat = GL_RGB; break;
        }

        unsigned int Texture;
        glGenTextures(1, &Texture);
        glBindTexture(GL_TEXTURE_2D, Texture);
        glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, TextureWidth, TextureHeight, 0, InternalFormat, GL_UNSIGNED_BYTE, ImageData);
        glBindTexture(GL_TEXTURE_2D, 0);
        // oslobadjanje memorije zauzete sa stbi_load posto vise nije potrebna
        stbi_image_free(ImageData);
        return Texture;
    }
    else
    {
        std::cout << "Textura nije ucitana! Putanja texture: " << filePath << std::endl;
        stbi_image_free(ImageData);
        return 0;
    }
}

// Globalne promenljive za koordinate dugmadi
float buttonWidth = 0.15f;   // širina dugmica tastature
float buttonHeight = 0.1f;  // visina svakog dugmeta
float buttonWidth2 = 0.45f;   // širina velikih dugmica

// Koordinate za svako dugme
float button1_x = 0.45f, button1_y = 0.2f;  // Dugme 1
float button2_x = 0.60f, button2_y = 0.2f;  // Dugme 2
float button3_x = 0.75f, button3_y = 0.2f;  // Dugme 3
float button4_x = 0.45f, button4_y = 0.1f; // Dugme 4
float button5_x = 0.60f, button5_y = 0.1f; // Dugme 5
float button6_x = 0.75f, button6_y = 0.1f; // Dugme 6
float button7_x = 0.45f, button7_y = 0.0f;  // Dugme 7
float button8_x = 0.60f, button8_y = 0.0f;  // Dugme 8
float button9_x = 0.75f, button9_y = 0.0f;  // Dugme 9
float button0_x = 0.60f, button0_y = -0.1f; // Dugme 0

float start_x = 0.45f, start_y = -0.3f;  // Dugme Start
float stop_x = 0.45f, stop_y = -0.4f;  // Dugme Stop
float reset_x = 0.45f, reset_y = -0.5f;  // Dugme Reset



void checkCondtions(std::string num) {
    if (digitCount == 0 || digitCount == 2) {
        if (std::stoi(num) > 5) {
            std::cout << "Nepravilan unos." << std::endl;
            return;
        }
    }
    if (digitCount < 4) {
        
        if (!isColon) {
            minutes = minutes * 10 + std::stoi(num); 
        }
        else {
            seconds = seconds * 10 + std::stoi(num); 
        }
        displayText += num;
        digitCount++;
    }
}


void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {

    if (action == GLFW_PRESS ) { 
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);  
        


        mouseX = (2.0f * mouseX) / 800.0 - 1.0f;
        mouseY = 1.0f - (2.0f * mouseY) / 800.0;

        if (mouseX > 0.45 && mouseX < 0.9 && mouseY> -0.1 && mouseY < 0.3) {
            if (displayText == "   :" || displayText == "00:00") {
                displayText = "";
            }
        }

        

        if (!inputLocked) {
            
            if (mouseX >= button1_x && mouseX <= button1_x + buttonWidth && mouseY >= button1_y && mouseY <= button1_y + buttonHeight) {
                checkCondtions("1");
            }
            else if (mouseX >= button2_x && mouseX <= button2_x + buttonWidth && mouseY >= button2_y && mouseY <= button2_y + buttonHeight) {
                checkCondtions("2");
            }
            else if (mouseX >= button3_x && mouseX <= button3_x + buttonWidth && mouseY >= button3_y && mouseY <= button3_y + buttonHeight) {
                checkCondtions("3");
            }
            else if (mouseX >= button4_x && mouseX <= button4_x + buttonWidth && mouseY >= button4_y && mouseY <= button4_y + buttonHeight) {
                checkCondtions("4");
            }
            else if (mouseX >= button5_x && mouseX <= button5_x + buttonWidth && mouseY >= button5_y && mouseY <= button5_y + buttonHeight) {
                checkCondtions("5");
            }
            else if (mouseX >= button6_x && mouseX <= button6_x + buttonWidth && mouseY >= button6_y && mouseY <= button6_y + buttonHeight) {
                checkCondtions("6");
            }
            else if (mouseX >= button7_x && mouseX <= button7_x + buttonWidth && mouseY >= button7_y && mouseY <= button7_y + buttonHeight) {
                checkCondtions("7");
            }
            else if (mouseX >= button8_x && mouseX <= button8_x + buttonWidth && mouseY >= button8_y && mouseY <= button8_y + buttonHeight) {
                checkCondtions("8");
            }
            else if (mouseX >= button9_x && mouseX <= button9_x + buttonWidth && mouseY >= button9_y && mouseY <= button9_y + buttonHeight) {
                checkCondtions("9");
            }
            else if (mouseX >= button0_x && mouseX <= button0_x + buttonWidth && mouseY >= button0_y && mouseY <= button0_y + buttonHeight) {
                checkCondtions("0");
            }

        }
        
        if (mouseX >= start_x && mouseX <= start_x + buttonWidth2 && mouseY >= start_y && mouseY <= start_y + buttonHeight) {
            if (!isDoorClosed) {
                std::cout << "Vrata nisu zatvorena" << std::endl;
                return;
            }
            if(!inputLocked) {
                std::cout << "Neispravan unos." << std::endl;
                return;
            }
            if (displayText == "00:00") {
                std::cout << "Kraj" << std::endl;
                return;
            }

            std::cout << "Kliknut start." << std::endl;
            //upali svetlo, zapocni tajmer
            isTimerRunning = true;

        }
        else if (mouseX >= stop_x && mouseX <= stop_x + buttonWidth2 && mouseY >= stop_y && mouseY <= stop_y + buttonHeight) {
            //zaustavi tajmer
            std::cout << "Kliknut stop." << std::endl;
            isTimerRunning = false;
        }
        else if (mouseX >= reset_x && mouseX <= reset_x + buttonWidth2 && mouseY >= reset_y && mouseY <= reset_y + buttonHeight) {
            std::cout << "Kliknut reset." << std::endl;
            displayText = "   :";
            minutes = 0;
            seconds = 0;
            digitCount = 0;
            isColon = false;
            inputLocked = false;
            isTimerRunning = false;
        }

        if (digitCount == 4) {
            inputLocked = true;
        }

        
        if (digitCount == 2 && !isColon) {
            displayText += ":";
            isColon = true;  
        }
    }
}


static void updateTimer() {
    
    if (seconds > 0) {
        seconds--;
    }
    else if (minutes > 0) {
        minutes--;
        seconds = 59;  
    }
    else if (seconds == 0 && minutes == 0) {
        digitCount = 0;
        isColon = false;
        inputLocked = false;
        isTimerRunning = false;

        //kraj
    }

    std::string timeText = (minutes < 10 ? "0" : "") + std::to_string(minutes) + ":" + (seconds < 10 ? "0" : "") + std::to_string(seconds);
    displayText = timeText;
    
}






void RenderText(unsigned int& shader, std::string text, float x, float y, float scale, glm::vec3 color)
{
    // activate corresponding render state	
    glUseProgram(shader);
    glUniform3f(glGetUniformLocation(shader, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(letterVAO);

    // iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++)
    {
        Character ch = Characters[*c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;
        // update VBO for each character
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, letterVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // be sure to use glBufferSubData and not glBufferData

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}