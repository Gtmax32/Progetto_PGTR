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
#include "BulletDebugDrawer.h"

#define NR_LIGHTS 3

// Dimensioni della finestra dell'applicazione
const GLuint SCR_WIDTH = 1280, SCR_HEIGHT = 720;

// Camera
Camera camera(glm::vec3(0.0f, 9.0f, 15.0f));

// Variabili utilizzate per implementare una Camera FPS
GLfloat lastX = (float)SCR_WIDTH / 2.0f;
GLfloat lastY = (float)SCR_HEIGHT / 2.0f;
GLfloat firstMouse = true;
double mouseX, mouseY;

// Deltatime per uniformare la velocità di movimento
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

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

//Dimensione della sfera
glm::vec3 sphereSize = glm::vec3(0.5f, 0.5f, 0.5f);
//Posizione delle palle del gioco
glm::vec3 poolBallPos[] = {
	glm::vec3(-3.0f, 14.0f, 0.0f), // palla bianca
	glm::vec3(0.0f, 14.0f, 0.0f), // palla rossa
	glm::vec3(3.0f, 14.0f, 0.0f) // palla gialla
};

glm::vec3 poolPlanePos = glm::vec3(0.0f, 0.0f, 0.0f);

// Registra gli eventi che modificano le dimensioni della finestra
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// Funzioni di utility
GLuint loadTexture(const char* path);
void draw_model_texture(Shader &shader, Model &plane, btRigidBody* bodyPlane, Model &table, Model &pin);
void draw_model_notexture(Shader &shader, Model &ball, btRigidBody* bodyWhite, btRigidBody* bodyRed, btRigidBody* bodyYellow);

// Funzioni per gestire il gioco
void throw_ball(btRigidBody* ball);

Physics poolSimulation;
BulletDebugDrawer debugger;
btRigidBody* bodyBallWhite;

glm::mat4 projection(1.0f);
glm::mat4 view(1.0f);
glm::mat4 model(1.0f);
glm::mat3 normal(1.0f);

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
	
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH,SCR_HEIGHT,"Goriziana",nullptr,nullptr);
	
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
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	
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
	Shader shaderDebugger("../shaderDebug.vert", "../shaderDebug.frag");
	
	//UTILIZZO LA CLASSE MODEL CREATA PER CARICARE E VISUALIZZARE IL MODELLO 3D
	//Model modelRoom("../../table/resource/Room.obj");
	Model modelTable("../../models/table/pooltable.obj");
	Model modelBall("../../models/ball/ball.obj");
	Model modelPin("../../models/pin/High_Poly.obj");
	Model modelPlane("../../models/plane/plane.obj");
	
	//CREO IL CORPO RIGIDO DA ASSEGNARE AL PIANO
	glm::vec3 bodyPlaneSize = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::vec3 bodyPlaneRotation = glm::vec3(0.0f, 0.0f, 0.0f);
	
	btRigidBody* bodyPlane = poolSimulation.createRigidBody(0, poolPlanePos, bodyPlaneSize, bodyPlaneRotation, 0.0, 0.3, 0.3);
	
	//CREO IL CORPO RIGIDO DA ASSEGNARE AL TAVOLO
	glm::vec3 bodyTablePos = glm::vec3(0.0f, 6.48f, 0.0f);
	glm::vec3 bodyTableSize = glm::vec3(12.0f, 0.1f, 5.2f);
	glm::vec3 bodyTableRotation = glm::vec3(0.0f, 0.0f, 0.0f);
	
	btRigidBody* bodyTable = poolSimulation.createRigidBody(0, bodyTablePos, bodyTableSize, bodyTableRotation, 0.0, 0.3, 0.3);
	
	//CREO I BORDI DEL TAVOLO
	//LATO LUNGO POSTERIORE
	glm::vec3 bodyTableLSPos = glm::vec3(0.0f, 6.8f, -5.1f);
	glm::vec3 bodyTableLSSize = glm::vec3(12.0f, 1.0f, 0.1f);
	glm::vec3 bodyTableLSRotation = glm::vec3(0.0f, 0.0f, 0.0f);
	
	bodyTable = poolSimulation.createRigidBody(0, bodyTableLSPos, bodyTableLSSize, bodyTableLSRotation, 0.0, 0.3, 0.3);
	
	//LATO LUNGO ANTERIORE
	bodyTableLSPos = glm::vec3(0.0f, 6.8f, 5.1f);	
	
	bodyTable = poolSimulation.createRigidBody(0, bodyTableLSPos, bodyTableLSSize, bodyTableLSRotation, 0.0, 0.3, 0.3);
	
	//LATO CORTO SINISTRO
	glm::vec3 bodyTableSSPos = glm::vec3(-11.7f, 6.8f, 0.0f);
	glm::vec3 bodyTableSSSize = glm::vec3(0.1f, 1.0f, 5.2f);
	glm::vec3 bodyTableSSRotation = glm::vec3(0.0f, 0.0f, 0.0f);
	
	bodyTable = poolSimulation.createRigidBody(0, bodyTableSSPos, bodyTableSSSize, bodyTableSSRotation, 0.0, 0.3, 0.3);
	
	//LATO CORTO DESTRO
	bodyTableSSPos = glm::vec3(11.8f, 6.8f, 0.0f);
	
	bodyTable = poolSimulation.createRigidBody(0, bodyTableSSPos, bodyTableSSSize, bodyTableSSRotation, 0.0, 0.3, 0.3);
	
	//CREO IL CORPO RIGIDO DA ASSEGNARE ALLE PALLE
	glm::vec3 bodyBallRadius = sphereSize;
	glm::vec3 bodyBallRotation = glm::vec3(0.0f, 0.0f, 0.0f);
	
	bodyBallWhite = poolSimulation.createRigidBody(1, poolBallPos[0], bodyBallRadius, bodyBallRotation, 1.0, 0.3, 0.3);
	btRigidBody* bodyBallRed = poolSimulation.createRigidBody(1, poolBallPos[1], bodyBallRadius, bodyBallRotation, 1.0, 0.3, 0.3);
	btRigidBody* bodyBallYellow = poolSimulation.createRigidBody(1, poolBallPos[2], bodyBallRadius, bodyBallRotation, 1.0, 0.3, 0.3);	
	
	// imposto il delta di tempo massimo per aggiornare la simulazione fisica
    GLfloat maxSecPerFrame = 1.0f / 60.0f;
	
	debugger.setDebugMode(btIDebugDraw::DBG_DrawWireframe);
	poolSimulation.dynamicsWorld->setDebugDrawer(&debugger);
	
	//AVVIO IL RENDER LOOP
	while(!glfwWindowShouldClose(window)){
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;		
		lastFrame = currentFrame;
		
		glClearColor(0.31f, 0.76f, 0.92f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		projection = glm::perspective(45.0f, (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 10000.0f);
		view = camera.GetViewMatrix();
		
		debugger.SetMatrices(shaderDebugger, projection, view);
		poolSimulation.dynamicsWorld->debugDrawWorld();

		poolSimulation.dynamicsWorld->stepSimulation((deltaTime < maxSecPerFrame ? deltaTime : maxSecPerFrame), 10);
		
		draw_model_notexture(shaderNoTexture, modelBall, bodyBallWhite, bodyBallRed, bodyBallYellow);
		
		draw_model_texture(shaderTexture, modelPlane, bodyPlane, modelTable, modelPin);	
		
		processInput(window);
		
		glfwSwapBuffers(window);
		glfwPollEvents();		
	}
	
	shaderTexture.Delete();
	shaderNoTexture.Delete();
	shaderDebugger.Delete();
	
	poolSimulation.Clear();
	
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
	
	if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		throw_ball(bodyBallWhite);
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
	
	mouseX = xpos;
	mouseY = ypos;
	
	GLfloat xOffset = xpos - lastX;
	GLfloat yOffset = lastY - ypos; // Inverto la sottrazione per l'asse è negativo in questo caso
	
	lastX = xpos;
	lastY = ypos;
	
	camera.ProcessMouseMovement(xOffset, yOffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
    camera.ProcessMouseScroll(yoffset);
}

void throw_ball(btRigidBody* ball){
	//std::cout << "Space pressed..." << std::endl;
	GLfloat shootInitialSpeed = 1.0f;
	
	GLfloat matrix[16];
	
	btVector3 impulse, origin;
	btTransform transform;
	
	glm::vec3 ballPos;
	
	ball->getMotionState()->getWorldTransform(transform);
	transform.getOpenGLMatrix(matrix);
	
	origin = transform.getOrigin();
	
	ballPos = glm::vec3(origin.getX(), origin.getY(), origin.getZ());
	
	glm::vec3 mousePos = glm::vec3(mouseX, mouseY, 1.0f);
	
	glm::vec3 direction = glm::normalize(mousePos - ballPos) * shootInitialSpeed;
		
	impulse = btVector3(direction.x, direction.y, direction.z);
	ball->applyCentralImpulse(impulse);
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

// Imposto lo shader e renderizzo i modelli degli oggetti senza texture
void draw_model_notexture(Shader &shader, Model &ball, btRigidBody* bodyWhite, btRigidBody* bodyRed, btRigidBody* bodyYellow){	
	GLfloat matrix[16];
	btTransform transform;
	
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
	}
	
	shader.setFloat("Kd",Kd);		
	shader.setFloat("Ka", Ka);
	shader.setFloat("Ks", Ks);
	
	shader.setFloat("constant",constant);
	shader.setFloat("linear", linear);
	shader.setFloat("quadratic", quadratic);
	shader.setFloat("shininess", shininess);
	
	shader.setMat4("projectionMatrix", projection);
	
	shader.setMat4("viewMatrix", view);
	
	model = glm::mat4(1.0f);
	normal = glm::mat3(1.0f);
	
	bodyWhite->getMotionState()->getWorldTransform(transform);
	transform.getOpenGLMatrix(matrix);
	
	//model = glm::translate(model, poolBallPos[0]);
	//model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::make_mat4(matrix) * glm::scale(model, sphereSize);
	//model = glm::make_mat4(matrix) * model;
	
	normal = glm::inverseTranspose(glm::mat3(view*model));
	
	shader.setMat4("modelMatrix", model);
	shader.setMat3("normalMatrix",normal);
	
	ball.Draw(shader);
	
	//RENDERIZZO LA PALLA ROSSA
	shader.setVec3("diffuseColor", 1.0f, 0.0f, 0.0f);
	
	model = glm::mat4(1.0f);
	normal = glm::mat3(1.0f);
	
	bodyRed->getMotionState()->getWorldTransform(transform);
	transform.getOpenGLMatrix(matrix);
	
	//model = glm::translate(model, poolBallPos[1]); 
	//model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::make_mat4(matrix) * glm::scale(model, sphereSize);
	// se casto a mat3 una mat4, in automatico estraggo la sottomatrice 3x3 superiore sinistra
	normal = glm::inverseTranspose(glm::mat3(view*model));
	
	shader.setMat4("modelMatrix", model);
	shader.setMat3("normalMatrix",normal);
	
	ball.Draw(shader);
	
	//RENDERIZZO LA PALLA GIALLA
	shader.setVec3("diffuseColor", 1.0f, 1.0f, 0.0f);
	
	model = glm::mat4(1.0f);
	normal = glm::mat3(1.0f);
	
	bodyYellow->getMotionState()->getWorldTransform(transform);
	transform.getOpenGLMatrix(matrix);
	
	//model = glm::translate(model, poolBallPos[2]); 
	//model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::make_mat4(matrix) * glm::scale(model, sphereSize);
	// se casto a mat3 una mat4, in automatico estraggo la sottomatrice 3x3 superiore sinistra
	normal = glm::inverseTranspose(glm::mat3(view*model));
	
	shader.setMat4("modelMatrix", model);
	shader.setMat3("normalMatrix",normal);
	
	ball.Draw(shader);
}

// Imposto lo shader e renderizzo i modelli degli oggetti con texture
void draw_model_texture(Shader &shader, Model &plane, btRigidBody* bodyPlane, Model &table, Model &pin){
	btTransform transform;
	GLfloat matrix[16];
	
	GLint texture = loadTexture("../../textures/floor.jpg");
	
	//RENDERIZZO IL TAVOLO
	shader.Use();
	
	shader.setVec3("ambientColor", ambientColor);
	shader.setVec3("specularColor", specularColor);
	
	// Per ogni luce nello shader, passo la posizione corrispondente
	for(GLuint i = 0; i < NR_LIGHTS; i++){
		string number = to_string(i);
		shader.setVec3(("lights[" + number + "]").c_str(), lightPositions[i]);
	}
	
	shader.setFloat("Kd",Kd);		
	shader.setFloat("Ka", Ka);
	shader.setFloat("Ks", Ks);
	
	shader.setFloat("constant",constant);
	shader.setFloat("linear", linear);
	shader.setFloat("quadratic", quadratic);
	shader.setFloat("shininess", shininess);
	
	shader.setFloat("repeat", 1.0f);
	
	shader.setMat4("projectionMatrix", projection);
	
	shader.setMat4("viewMatrix", view);
	
	model = glm::mat4(1.0f);
	normal = glm::mat3(1.0f);
	
	model = glm::translate(model, glm::vec3(0.0f, 0.1f, -0.15f)); 
	//model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
	normal = glm::inverseTranspose(glm::mat3(view*model));
	
	shader.setMat4("modelMatrix", model);
	shader.setMat3("normalMatrix",normal);
	
	table.Draw(shader);
	
	//RENDERIZZO I BIRILLI
	/*model = glm::mat4();
	normal = glm::mat4();
	
	model = glm::translate(model, glm::vec3(0.0f, 11.0f, 0.0f)); 
	//model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
	normal = glm::inverseTranspose(glm::mat3(view*model));
	
	shader.setMat4("modelMatrix", model);
	shader.setMat3("normalMatrix",normal);
	
	pin.Draw(shader);*/

	//RENDERIZZO IL PIANO
	bodyPlane->getMotionState()->getWorldTransform(transform);
	transform.getOpenGLMatrix(matrix);
	
	shader.setInt("tex",0);
	shader.setFloat("repeat",2.0f);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	
	model = glm::mat4(1.0f);
	normal = glm::mat4(1.0f);
	
	//model = glm::translate(model, poolPlanePos); 
	//model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
	normal = glm::inverseTranspose(glm::mat3(view*model));
	
	shader.setMat4("modelMatrix", model);
	shader.setMat3("normalMatrix",normal);
	
	plane.Draw(shader);
}

