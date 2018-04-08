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

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

// Libreria per il caricamento delle immagini
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#define NR_LIGHTS 3

// Dimensioni della finestra dell'applicazione
const GLuint SCR_WIDTH = 1280, SCR_HEIGHT = 720;

// Camera
Camera camera(glm::vec3(0.0f, 0.0f, 4.0f));

// Variabili utilizzate per implementare una Camera FPS
GLfloat lastX = (float)SCR_WIDTH / 2.0f;
GLfloat lastY = (float)SCR_HEIGHT / 2.0f;
GLfloat firstMouse = true;

// Deltatime per uniformare la velocità di movimento
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// Posizione del light cube nella scena
glm::vec3 lightPos(0.5f, 0.2f, 2.0f);

// posizioni delle mie pointlight
glm::vec3 lightPositions[] = {
    glm::vec3(-5.0f, 10.0f, 10.0f),
    glm::vec3(0.0f, 10.0f, 10.0f),
    glm::vec3(5.0f, 10.0f, 10.0f),
};  

// Uniform da passare agli shader 
//glm::vec3 diffuseColor(1.0f, 0.0f, 0.0f);
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

// Registra gli eventi che modificano le dimensioni della finestra
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

GLuint loadTexture(const char* path);

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
	
	//DEFINISCO I COLORI DELLE PALLE DA BILIARDO UTILIZZATE NEL GIOCO
	glm::vec3 colorWhite(1.0f, 1.0f, 1.0f);
	glm::vec3 colorRed(1.0f, 0.0f, 0.0f);
	glm::vec3 colorYellow(1.0f, 1.0f, 0.0f);
	
	//DEFINISCO LE POSIZIONI DELLE PALLE DA GIOCO
	glm::vec3 positionWhite(-3.0f, 0.0f, 0.0f);
	glm::vec3 positionRed(0.0f, 0.0f, 0.0f);
	glm::vec3 positionYellow(3.0f, 0.0f, 0.0f);
	
	//UTILIZZO LA CLASSE SHADER CREATA PER COMPILARE IL VS ED IL FS, E LINKARLI NEL PS
	Shader shaderNoTexture("../13_phong.vert", "../13b_blinnphong.frag");
	Shader shaderTexture("../18_phong_tex_multiplelights.vert","../18_blinnphong_tex_multiplelights.frag");
	
	//UTILIZZO LA CLASSE MODEL CREATA PER CARICARE E VISUALIZZARE IL MODELLO 3D
	//Model model3D("../../../models/cube.obj");
	//Model modelBall("../../table/resource/Ball3.obj");
	//Model modelRoom("../../table/resource/Room.obj");
	Model modelTable("../../models/table/pooltable.obj");
	Model modelBallWhite("../../models/ball/ball.obj"),
		  modelBallRed("../../models/ball/ball.obj"),
		  modelBallYellow("../../models/ball/ball.obj");
	Model modelPin("../../models/pin/High_Poly.obj");
	Model modelPlane("../../models/plane/plane.obj");
	
	//CARICO LA TEXTURE
	GLint texturePlane = loadTexture("../../textures/floor.jpg");
	
	//AVVIO IL RENDER LOOP
	while(!glfwWindowShouldClose(window)){
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		
		lastFrame = currentFrame;
		
		processInput(window);
		
		glClearColor(0.31f, 0.76f, 0.92f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		shaderNoTexture.Use();
		
		//RENDERIZZO LE PALLE DA BILIARDO
		//INIZIO DALLA BIANCA
		shaderNoTexture.setVec3("diffuseColor", colorWhite);
		shaderNoTexture.setVec3("ambientColor", ambientColor);
		shaderNoTexture.setVec3("specularColor", specularColor);
		
		// Per ogni luce nello shader, passo la posizione corrispondente
		for(GLuint i = 0; i < NR_LIGHTS; i++){
			string number = to_string(i);
			shaderNoTexture.setVec3(("lights[" + number + "]").c_str(), lightPositions[i]);
			//glUniform3fv(glGetUniformLocation(shaders[current_program].Program, ("lights[" + number + "]").c_str()), 1, glm::value_ptr(lightPositions[i]));
		}
		
		shaderNoTexture.setFloat("Kd",Kd);		
		shaderNoTexture.setFloat("Ka", Ka);
		shaderNoTexture.setFloat("Ks", Ks);
		
		shaderNoTexture.setFloat("constant",constant);
		shaderNoTexture.setFloat("linear", linear);
		shaderNoTexture.setFloat("quadratic", quadratic);
		shaderNoTexture.setFloat("shininess", shininess);
		
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 10000.0f);
		shaderNoTexture.setMat4("projectionMatrix", projection);
		
		glm::mat4 view = camera.GetViewMatrix();
		shaderNoTexture.setMat4("viewMatrix", view);
		
		/* Crea la matrice delle trasformazioni tramite la definizione delle 3 trasformazioni
		   NB) l'ultima definita è la prima applicata
		   Crea anche la matrice di trasformazione delle normali, che è l'inversa della trasposta della sottomatrice 3x3 (sup sinistra) della modelview.
		   Tolgo la 4a colonna per non considerare le traslazioni. Vedere la spiegazione seguente per capire perchè serve l'inversa della trasposta:
		
		   Two column vectors X and Y are perpendicular if and only if XT.Y=0. If We're going to transform X by a matrix M, we need to transform Y 
		   by some matrix N so that (M.X)T.(N.Y)=0. Using the identity (A.B)T=BT.AT, this becomes (XT.MT).(N.Y)=0 => XT.(MT.N).Y=0. If MT.N is the 
		   identity matrix then this reduces to XT.Y=0. And MT.N is the identity matrix if and only if N=(MT)-1, i.e. N is the inverse of the transpose of M.*/
		glm::mat4 sphereModelMatrix;
		glm::mat3 sphereNormalMatrix;
		sphereModelMatrix = glm::translate(sphereModelMatrix, positionWhite); 
		//sphereModelMatrix = glm::rotate(sphereModelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		sphereModelMatrix = glm::scale(sphereModelMatrix, glm::vec3(0.5f, 0.5f, 0.5f));
		// se casto a mat3 una mat4, in automatico estraggo la sottomatrice 3x3 superiore sinistra
		sphereNormalMatrix = glm::inverseTranspose(glm::mat3(view*sphereModelMatrix));
		
		shaderNoTexture.setMat4("modelMatrix", sphereModelMatrix);
		shaderNoTexture.setMat3("normalMatrix",sphereNormalMatrix);
		
		modelBallWhite.Draw(shaderNoTexture);
		
		//RENDERIZZO LA PALLA ROSSA
		shaderNoTexture.setVec3("diffuseColor", colorRed);
		
		sphereModelMatrix = glm::mat4();
		sphereNormalMatrix = glm::mat4();
		
		sphereModelMatrix = glm::translate(sphereModelMatrix, positionRed); 
		//sphereModelMatrix = glm::rotate(sphereModelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		sphereModelMatrix = glm::scale(sphereModelMatrix, glm::vec3(0.5f, 0.5f, 0.5f));
		// se casto a mat3 una mat4, in automatico estraggo la sottomatrice 3x3 superiore sinistra
		sphereNormalMatrix = glm::inverseTranspose(glm::mat3(view*sphereModelMatrix));
		
		shaderNoTexture.setMat4("modelMatrix", sphereModelMatrix);
		shaderNoTexture.setMat3("normalMatrix",sphereNormalMatrix);
		
		modelBallRed.Draw(shaderNoTexture);
		
		//RENDERIZZO LA PALLA ROSSA
		shaderNoTexture.setVec3("diffuseColor", colorYellow);
		
		sphereModelMatrix = glm::mat4();
		sphereNormalMatrix = glm::mat4();
		
		sphereModelMatrix = glm::translate(sphereModelMatrix, positionYellow); 
		//sphereModelMatrix = glm::rotate(sphereModelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		sphereModelMatrix = glm::scale(sphereModelMatrix, glm::vec3(0.5f, 0.5f, 0.5f));
		// se casto a mat3 una mat4, in automatico estraggo la sottomatrice 3x3 superiore sinistra
		sphereNormalMatrix = glm::inverseTranspose(glm::mat3(view*sphereModelMatrix));
		
		shaderNoTexture.setMat4("modelMatrix", sphereModelMatrix);
		shaderNoTexture.setMat3("normalMatrix",sphereNormalMatrix);
		
		modelBallYellow.Draw(shaderNoTexture);
		
		//RENDERIZZO IL TAVOLO
		shaderTexture.Use();
		
		shaderTexture.setVec3("diffuseColor", colorWhite);
		shaderTexture.setVec3("ambientColor", ambientColor);
		shaderTexture.setVec3("specularColor", specularColor);
		
		// Per ogni luce nello shader, passo la posizione corrispondente
		for(GLuint i = 0; i < NR_LIGHTS; i++){
			string number = to_string(i);
			shaderTexture.setVec3(("lights[" + number + "]").c_str(), lightPositions[i]);
			//glUniform3fv(glGetUniformLocation(shaders[current_program].Program, ("lights[" + number + "]").c_str()), 1, glm::value_ptr(lightPositions[i]));
		}
		
		shaderTexture.setFloat("Kd",Kd);		
		shaderTexture.setFloat("Ka", Ka);
		shaderTexture.setFloat("Ks", Ks);
		
		shaderTexture.setFloat("constant",constant);
		shaderTexture.setFloat("linear", linear);
		shaderTexture.setFloat("quadratic", quadratic);
		shaderTexture.setFloat("shininess", shininess);
		
		shaderTexture.setFloat("repeat", 1.0f);
		
		sphereModelMatrix = glm::mat4();
		sphereNormalMatrix = glm::mat4();
		
		sphereModelMatrix = glm::translate(sphereModelMatrix, glm::vec3(0.0f, -8.0f, 0.0f)); 
		//sphereModelMatrix = glm::rotate(sphereModelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		sphereModelMatrix = glm::scale(sphereModelMatrix, glm::vec3(25.0f, 25.0f, 25.0f));
		sphereNormalMatrix = glm::inverseTranspose(glm::mat3(view*sphereModelMatrix));
		
		shaderTexture.setMat4("projectionMatrix", projection);
		shaderTexture.setMat4("viewMatrix", view);
		shaderTexture.setMat4("modelMatrix", sphereModelMatrix);
		shaderTexture.setMat3("normalMatrix",sphereNormalMatrix);
		
		modelTable.Draw(shaderTexture);
		
		//RENDERIZZO I BIRILLI
		sphereModelMatrix = glm::mat4();
		
		sphereModelMatrix = glm::translate(sphereModelMatrix, glm::vec3(0.0f, 3.0f, 0.0f)); 
		//sphereModelMatrix = glm::rotate(sphereModelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		sphereModelMatrix = glm::scale(sphereModelMatrix, glm::vec3(0.05f, 0.05f, 0.05f));
		
		shaderTexture.setMat4("modelMatrix", sphereModelMatrix);
		modelPin.Draw(shaderTexture);

		//RENDERIZZO IL PIANO
		shaderTexture.setInt("tex",0);
		shaderTexture.setFloat("repeat",80.0f);
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texturePlane);
		
		sphereModelMatrix = glm::mat4();
		
		sphereModelMatrix = glm::translate(sphereModelMatrix, glm::vec3(0.0f, -8.01f, 0.0f)); 
		//sphereModelMatrix = glm::rotate(sphereModelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		sphereModelMatrix = glm::scale(sphereModelMatrix, glm::vec3(50.0f, 1.0f, 50.0f));
		
		shaderTexture.setMat4("modelMatrix", sphereModelMatrix);
		modelPlane.Draw(shaderTexture);
		
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