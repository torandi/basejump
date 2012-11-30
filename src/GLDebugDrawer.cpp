#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include <iostream>
#include "shader.hpp"

#include "GLDebugDrawer.hpp"



GLDebugDrawer::GLDebugDrawer() : m_debugMode(0), numVerts(0)
{
	shader_ = Shader::create_shader("/shaders/simple");
	initDrawLine();
}


GLDebugDrawer::~GLDebugDrawer()
{
	glDeleteVertexArrays(1, &vao);
}


void GLDebugDrawer::initDrawLine()
{
	// create and bind vao
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	
	// create and bind vbo
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
		
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}


void GLDebugDrawer::addVertex(const btVector3& v, const btVector3& c)
{
	int _n = ITEM_SIZE * numVerts;
	
	verts[_n] = v.x();
	verts[_n+1] = v.y();
	verts[_n+2] = v.z();

	verts[_n+3] = c.x();
	verts[_n+4] = c.y();
	verts[_n+5] = c.z();
	verts[_n+6] = 1.f;

	++numVerts;
}


void GLDebugDrawer::drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
{
	addVertex(from, color);
	//addVertex(to, color);
}


void GLDebugDrawer::setDebugMode(int debugMode) {
	m_debugMode = debugMode; }


int GLDebugDrawer::getDebugMode() const {
	 return m_debugMode; }


void GLDebugDrawer::draw3dText(const btVector3& location,const char* textString)
{
//		glRasterPos3f(location.x(),  location.y(),  location.z());
	//BMF_DrawString(BMF_GetFont(BMF_kHelvetica10),textString);
}


void GLDebugDrawer::reportErrorWarning(const char* warningString)
{
	std::cout << warningString << std::endl;
}


void GLDebugDrawer::drawContactPoint(const btVector3 & pointOnB,const btVector3 & normalOnB,btScalar distance,int lifeTime,const btVector3& color)
{
//			btVector3 to=pointOnB+normalOnB*1;//distance;
//			const btVector3&from = pointOnB;
//			glColor4f(color.getX(), color.getY(), color.getZ(),1.f);
//			//glColor4f(0,0,0,1.f);
//			glBegin(GL_LINES);
//			glVertex3d(from.getX(), from.getY(), from.getZ());
//			glVertex3d(to.getX(), to.getY(), to.getZ());
//			glEnd();


		//		glRasterPos3f(from.x(),  from.y(),  from.z());
		//		char buf[12];
		//		sprintf(buf," %d",lifeTime);
		//BMF_DrawString(BMF_GetFont(BMF_kHelvetica10),buf);
}


void GLDebugDrawer::commit()
{
	shader_->bind();
	Shader::upload_model_matrix(glm::mat4());
	
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, ITEM_STRIDE*numVerts, verts);
	
	Shader::push_vertex_attribs(2);
	
	glVertexAttribPointer(0, VERT_SIZE, GL_FLOAT, GL_FALSE, ITEM_STRIDE, 0);
	glVertexAttribPointer(1, CLR_SIZE, GL_FLOAT, GL_FALSE, ITEM_STRIDE, (void*)VERT_STRIDE);
	
	glBindVertexArray(vao);
	glDrawArrays(GL_LINES, 0, numVerts);
	glBindVertexArray(0);
	
	numVerts = 0;
	
	Shader::pop_vertex_attribs();
}