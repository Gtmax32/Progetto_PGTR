#ifndef BULLETDEBUGDRAWER_H
#define BULLETDEBUGDRAWER_H

#include <iostream>

#include <utils/shader_v2.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <bullet/btBulletDynamicsCommon.h>
#include <bullet/LinearMath/btIDebugDraw.h>

class BulletDebugDrawer : public btIDebugDraw {
public:	
	BulletDebugDrawer();
	~BulletDebugDrawer();
	
	void SetMatrices(Shader &shader, glm::mat4 viewMatrix, glm::mat4 projectionMatrix);
	void drawLine(const btVector3& from, const btVector3& to, const btVector3& color);
	
	void drawContactPoint(const btVector3&, const btVector3&, btScalar, int, const btVector3&);
	void reportErrorWarning(const char *);
	void draw3dText(const btVector3 &, const char *);
	
	void setDebugMode(int p);
	int getDebugMode();
};

#endif // BULLETDEBUGDRAWER_H
