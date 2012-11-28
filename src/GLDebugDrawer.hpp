#ifndef GL_DEBUG_DRAWER_H
#define GL_DEBUG_DRAWER_H


#include <LinearMath/btIDebugDraw.h>



class GLDebugDrawer : public btIDebugDraw
{
    int m_debugMode;
    
    Shader * shader_;
    
    GLuint vao;
    GLuint vbo;
    
    static const GLuint VERT_SIZE = 3;
    static const GLuint CLR_SIZE = 4;
    static const GLuint ITEM_SIZE = VERT_SIZE + CLR_SIZE;

    static const GLsizei VERT_STRIDE = sizeof(GLfloat) * VERT_SIZE;
    static const GLsizei CLR_STRIDE = sizeof(GLfloat) * CLR_SIZE;
    static const GLsizei ITEM_STRIDE = VERT_STRIDE + CLR_STRIDE;
    
    static const int MAX_VERTS = 1e6;
    
    GLfloat verts[ITEM_SIZE * MAX_VERTS];
    GLuint numVerts;
    
    
    
    void initDrawLine();
    
    void addVertex(const btVector3 & vert, const btVector3 & color);
    
public:
    GLDebugDrawer();
    virtual ~GLDebugDrawer();
    
    virtual void drawLine(const btVector3 & from, const btVector3 & to, const btVector3 & color);

    virtual void setDebugMode(int debugMode);
    virtual int	getDebugMode() const;

    virtual void drawContactPoint(const btVector3 & PointOnB,const btVector3 & normalOnB,btScalar & distance,int lifeTime,const btVector3 & color);

    virtual void reportErrorWarning(const char* warningString);

    virtual void draw3dText(const btVector3 & location,const char * textString);

    virtual void commit();
};



#endif
