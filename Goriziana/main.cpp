/* Codice ricavato dai tutorial della categoria Model Loading,	
 * partendo dal codice ottenuto dai tutorial precedenti.
 * Pagina web di riferimento: https://learnopengl.com/Model-Loading/Model
 */

#include <iostream>
#include <vector>
// THIS IS OPTIONAL AND NOT REQUIRED, ONLY USE THIS IF YOU DON'T WANT GLAD TO INCLUDE windows.h
// GLAD will include windows.h for APIENTRY if it was not previously defined.
// Make sure you have the correct definition for APIENTRY for platforms which define _WIN32 but don't use __stdcall
#ifdef _WIN32
    #define APIENTRY __stdcall
#endif

#include <glad/glad.h>

// GLFW
#include <glfw/glfw3.h>

// confirm that GLAD didn't include windows.h
#ifdef _WINDOWS_
    #error windows.h was included!
#endif

#include <utils/shader_v2.h>
#include <utils/camera_v2.h>
#include <utils/model_v3.h>
#include <utils/physics_v1.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

// Libreria per il caricamento delle immagini
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

// Libreria per la simulazione fisica
#include <bullet/btBulletDynamicsCommon.h>

#define NR_LIGHTS 3

// Dimensioni della finestra dell'applicazione
const GLuint SCR_WIDTH = 1280, SCR_HEIGHT = 720;

// Camera
Camera camera(glm::vec3(0.0f, 8.0f, 7.0f));

// Variabili utilizzate per implementare una Camera FPS
GLfloat lastX = (float)SCR_WIDTH / 2.0f;
GLfloat lastY = (float)SCR_HEIGHT / 2.0f;
GLfloat firstMouse = true;

// Deltatime per uniformare la velocità di movimento
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

//Dimensione della sfera
glm::vec3 sphereSize = glm::vec3(0.5f, 0.5f, 0.5f);

// Posizione del light cube nella scena
glm::vec3 lightPos(0.5f, 0.2f, 2.0f);

// posizioni delle mie pointlight
glm::vec3 lightPositions[] = {
    glm::vec3(-5.0f, 15.0f, 0.0f),
    glm::vec3(0.0f, 15.0f, 0.0f),
    glm::vec3(5.0f, 15.0f, 0.0f),
};  

// Uniform da passare agli shader 
glm::vec3 specularColor(1.0f, 1.0f, 1.0f);
glm::vec3 ambientColor(0.1f, 0.1f, 0.1f);
// pesi della componente diffusive, speculari e ambientali
GLfloat Kd = 0.8f;
GLfloat Ks = 0.5f;
GLfloat Ka = 0.1f;
// componente di shininess per shader Phong e Blinn-Phong
GLfloat shininess = 25.0f;

// parametri per l'attenuazione negli shader Phong e Blinn-Phong
GLfloat constant = 1.0f;
GLfloat linear = 0.09f;
GLfloat quadratic = 0.032f;

Physics poolSimulation;

// Registra gli eventi che modificano le dimensioni della finestra
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// Funzioni di utility
GLuint loadTexture(const char* path);
void draw_model_texture(Shader &shader, Model &plane, Model &table, Model &pin);
void draw_model_notexture(Shader &shader, Model &ball);

int main(){
	//INIZIALIZZO GLFW
	
	if (! glfwInit()){
		std::cout << "Errore nell'inizializzazione di GLFW!\n" << std::endl;
		return -1;
	}
	
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
	glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	
	//CREO LA FINESTRA
	
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH,SCR_HEIGHT,"Prima Finestra",nullptr,nullptr);
	
	if (!window){
		std::cout <<"Errore nella creazione della finestra!" << std::endl;
		glfwTerminate();
		
		return -1;
	}
	
	glfwMakeContextCurrent(window);
	
	//SETTO LE FUNZIONI DI CALLBACK CHE SI OCCUPA DI GESTIRE LE INTERAZIONI DELL'UTENTE
	
	glfwSetFramebufferSizeCallback(window,framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	
	// GLAD cerca di caricare il contesto impostato da GLFW
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Errore nell'inizializzazione del contesto OpenGL!" << std::endl;
        return -1;
    }
	
	//SETTO IL VIEWPORT	
	//glViewport(0,0,SCR_WIDTH,SCR_HEIGHT);
	
	//SETTO IL DEPTH TEST
	glEnable(GL_DEPTH_TEST);
	
	//UTILIZZO LA CLASSE SHADER CREATA PER COMPILARE IL VS ED IL FS, E LINKARLI NEL PS
	Shader shaderNoTexture("../shaderNoTexture.vert", "../shaderNoTexture.frag");
	Shader shaderTexture("../shaderTexture.vert","../shaderTexture.frag");
	
	//UTILIZZO LA CLASSE MODEL CREATA PER CARICARE E VISUALIZZARE IL MODELLO 3D
	//Model modelRoom("../../table/resource/Room.obj");
	Model modelTable("../../models/table/pooltable.obj");
	Model modelBall("../../models/ball/ball.obj");
	Model modelPin("../../models/pin/High_Poly.obj");
	Model modelPlane("../../models/plane/plane.obj");
	
	//CREO IL CORPO RIGIDO DA ASSEGNARE AL PIANO
	glm::vec3 bodyPlanePos = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 bodyPlaneSize = glm::vec3(50.0f, 1.0f, 50.0f);
	glm::vec3 bodyPlaneRotation = glm::vec3(0.0f, 0.0f, 0.0f);
	
	btRigidBody* bodyPlane = poolSimulation.createRigidBody(0, bodyPlanePos, bodyPlaneSize, bodyPlaneRotation, 0.0, 0.3, 0.3);
	
	//CREO IL CORPO RIGIDO DA ASSEGNARE ALLE PALLE
	glm::vec3 bodyBallRadius = sphereSize;
	glm::vec3 bodyBallRotation = glm::vec3(0.0f, 0.0f, 0.0f);
	
	// imposto il delta di tempo massimo per aggiornare la simulazione fisica
    GLfloat maxSecPerFrame = 1.0f / 60.0f;
	
	//AVVIO IL RENDER LOOP
	while(!glfwWindowShouldClose(window)){
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		
		lastFrame = currentFrame;
		
		processInput(window);
		
		glClearColor(0.31f, 0.76f, 0.92f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		draw_model_notexture(shaderNoTexture, modelBall);
		
		draw_model_texture(shaderTexture, modelPlane, modelTable, modelPin);		
		
		
		
		glfwSwapBuffers(window);
		glfwPollEvents();		
	}
	
	glfwTerminate();
	
	return 0;		
}

void processInput(GLFWwindow *window){
	if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	
	if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
		
	if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	
	if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
	
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height){
	glViewport(0,0,width,height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos){
	if(firstMouse){
		lastX = xpos;
		lastY = ypos;
		
		firstMouse = false;
	}
	
	GLfloat xOffset = xpos - lastX;
	GLfloat yOffset = lastY - ypos; // Inverto la sottrazione per l'asse è negativo in questo caso
	
	lastX = xpos;
	lastY = ypos;
	
	camera.ProcessMouseMovement(xOffset, yOffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
    camera.ProcessMouseScroll(yoffset);
}

// Carico immagine da disco e creo texture OpengGL
GLuint loadTexture(char const * path){
    GLuint textureID;
    glGenTextures(1, &textureID);

    GLint width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data){
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else{
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

void draw_model_notexture(Shader &shader, Model &ball){
	//CREO IL CORPO RIGIDO DA ASSEGNARE ALLE PALLE
	glm::vec3 bodyBallRadius = sphereSize;
	glm::vec3 bodyBallRotation = glm::vec3(0.0f, 0.0f, 0.0f);
	
	shader.Use();
		
	//RENDERIZZO LE PALLE DA BILIARDO
	//INIZIO DALLA BIANCA
	shader.setVec3("diffuseColor", 1.0f, 1.0f, 1.0f);
	shader.setVec3("ambientColor", ambientColor);
	shader.setVec3("specularColor", specularColor);
	
	// Per ogni luce nello shader, passo la posizione corrispondente
	for(GLuint i = 0; i < NR_LIGHTS; i++){
		string number = to_string(i);
		shader.setVec3(("lights[" + number + "]").c_str(), lightPositions[i]);
		//glUniform3fv(glGetUniformLocation(shaders[current_program].Program, ("lights[" + number + "]").c_str()), 1, glm::value_ptr(lightPositions[i]));
	}
	
	shader.setFloat("Kd",Kd);		
	shader.setFloat("Ka", Ka);
	shader.setFloat("Ks", Ks);
	
	shader.setFloat("constant",constant);
	shader.setFloat("linear", linear);
	shader.setFloat("quadratic", quadratic);
	shader.setFloat("shininess", shininess);
	
	glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 10000.0f);
	shader.setMat4("projectionMatrix", projection);
	
	glm::mat4 view = camera.GetViewMatrix();
	shader.setMat4("viewMatrix", view);
	
	glm::mat4 sphereModelMatrix;
	glm::mat3 sphereNormalMatrix;
	
	sphereModelMatrix = glm::translate(sphereModelMatrix, glm::vec3(-3.0f, 8.0f, 0.0f)); 
	//sphereModelMatrix = glm::rotate(sphereModelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	sphereModelMatrix = glm::scale(sphereModelMatrix, sphereSize);
	
	sphereNormalMatrix = glm::inverseTranspose(glm::mat3(view*sphereModelMatrix));
	
	shader.setMat4("modelMatrix", sphereModelMatrix);
	shader.setMat3("normalMatrix",sphereNormalMatrix);
	
	ball.Draw(shader);
	
	//RENDERIZZO LA PALLA ROSSA
	shader.setVec3("diffuseColor", 1.0f, 0.0f, 0.0f);
	
	sphereModelMatrix = glm::mat4();
	sphereNormalMatrix = glm::mat4();
	
	sphereModelMatrix = glm::translate(sphereModelMatrix, glm::vec3(0.0f, 8.0f, 0.0f)); 
	//sphereModelMatrix = glm::rotate(sphereModelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	sphereModelMatrix = glm::scale(sphereModelMatrix, sphereSize);
	// se casto a mat3 una mat4, in automatico estraggo la sottomatrice 3x3 superiore sinistra
	sphereNormalMatrix = glm::inverseTranspose(glm::mat3(view*sphereModelMatrix));
	
	shader.setMat4("modelMatrix", sphereModelMatrix);
	shader.setMat3("normalMatrix",sphereNormalMatrix);
	
	ball.Draw(shader);
	
	//RENDERIZZO LA PALLA GIALLA
	shader.setVec3("diffuseColor", 1.0f, 1.0f, 0.0f);
	
	sphereModelMatrix = glm::mat4();
	sphereNormalMatrix = glm::mat4();
	
	sphereModelMatrix = glm::translate(sphereModelMatrix, glm::vec3(3.0f, 8.0f, 0.0f)); 
	//sphereModelMatrix = glm::rotate(sphereModelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	sphereModelMatrix = glm::scale(sphereModelMatrix, sphereSize);
	// se casto a mat3 una mat4, in automatico estraggo la sottomatrice 3x3 superiore sinistra
	sphereNormalMatrix = glm::inverseTranspose(glm::mat3(view*sphereModelMatrix));
	
	shader.setMat4("modelMatrix", sphereModelMatrix);
	shader.setMat3("normalMatrix",sphereNormalMatrix);
	
	ball.Draw(shader);
}

// Imposto lo shader e renderizzo i modelli degli oggetti con texture
void draw_model_texture(Shader &shader, Model &plane, Model &table, Model &pin){
	GLint texture = loadTexture("../../textures/floor.jpg");
	
	//RENDERIZZO IL TAVOLO
	shader.Use();
	
	shader.setVec3("ambientColor", ambientColor);
	shader.setVec3("specularColor", specularColor);
	
	// Per ogni luce nello shader, passo la posizione corrispondente
	for(GLuint i = 0; i < NR_LIGHTS; i++){
		string number = to_string(i);
		shader.setVec3(("lights[" + number + "]").c_str(), lightPositions[i]);
		//glUniform3fv(glGetUniformLocation(shaders[current_program].Program, ("lights[" + number + "]").c_str()), 1, glm::value_ptr(lightPositions[i]));
	}
	
	shader.setFloat("Kd",Kd);		
	shader.setFloat("Ka", Ka);
	shader.setFloat("Ks", Ks);
	
	shader.setFloat("constant",constant);
	shader.setFloat("linear", linear);
	shader.setFloat("quadratic", quadratic);
	shader.setFloat("shininess", shininess);
	
	shader.setFloat("repeat", 1.0f);
	
	glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 10000.0f);
	shader.setMat4("projectionMatrix", projection);
	
	glm::mat4 view = camera.GetViewMatrix();
	shader.setMat4("viewMatrix", view);
	
	glm::mat4 sphereModelMatrix;
	glm::mat4 sphereNormalMatrix;
	
	sphereModelMatrix = glm::translate(sphereModelMatrix, glm::vec3(0.0f, 0.1f, 0.0f)); 
	//sphereModelMatrix = glm::rotate(sphereModelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	sphereModelMatrix = glm::scale(sphereModelMatrix, glm::vec3(25.0f, 25.0f, 25.0f));
	sphereNormalMatrix = glm::inverseTranspose(glm::mat3(view*sphereModelMatrix));
	
	shader.setMat4("projectionMatrix", projection);
	shader.setMat4("viewMatrix", view);
	shader.setMat4("modelMatrix", sphereModelMatrix);
	shader.setMat3("normalMatrix",sphereNormalMatrix);
	
	table.Draw(shader);
	
	//RENDERIZZO I BIRILLI
	sphereModelMatrix = glm::mat4();
	
	sphereModelMatrix = glm::translate(sphereModelMatrix, glm::vec3(0.0f, 11.0f, 0.0f)); 
	//sphereModelMatrix = glm::rotate(sphereModelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	sphereModelMatrix = glm::scale(sphereModelMatrix, glm::vec3(0.05f, 0.05f, 0.05f));
	
	shader.setMat4("modelMatrix", sphereModelMatrix);
	pin.Draw(shader);

	//RENDERIZZO IL PIANO
	shader.setInt("tex",0);
	shader.setFloat("repeat",80.0f);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	
	sphereModelMatrix = glm::mat4();
	
	sphereModelMatrix = glm::translate(sphereModelMatrix, glm::vec3(0.0f, 0.0f, 0.0f)); 
	//sphereModelMatrix = glm::rotate(sphereModelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	sphereModelMatrix = glm::scale(sphereModelMatrix, glm::vec3(50.0f, 1.0f, 50.0f));
	
	shader.setMat4("modelMatrix", sphereModelMatrix);
	plane.Draw(shader);
}

