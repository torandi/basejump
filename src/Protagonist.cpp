#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "Protagonist.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define RENDER_DEBUG 0

#if RENDER_DEBUG

#include "debug_mesh.hpp"

	static DebugMesh * triangle;
#endif

/*
	Left and right wings.
	 
	Left and right stick horizontal axis interacts with ROLL_FACTOR and YAW_FACTOR.
	Left and right stick vertical axis interacts with PITCH_FACTOR.
	Left and right trigger interacts with PITCH_FACTOR, ROLL_FACTOR, liftCoefficient and dragCoefficient.
	 
	Steer wings with left and right stick.
	Control wing areas with left and right trigger.
	This way you can rotate with horizontal axes but also with triggers if you have lift force.
	 
	__WING_OFFSET: rotation factor if one wing is pulled in. Greater offset means stronger rotation.
	TARGET_ROTATION_FACTOR: Greater value makes auto target and stabilization stronger.
	ANGULAR_VELOCITY_FACTOR: dampens previous angular velocity. Greater value makes auto stabilization weaker.
	ROLL_FACTOR: greater value increases rolling force
	YAW_FACTOR: greater value increases yaw factor
	PITCH_FACTOR: greater value increases pitch factor
*/
	
static const btScalar MASS = 80.f;// kg
	
static const btScalar OFFSET_X = .2f;
static const btVector3 L_WING_OFFSET(-OFFSET_X, 0, 0);// m
static const btVector3 R_WING_OFFSET(OFFSET_X, 0, 0);// m
	
// warn: TARGET_ROTATION_FACTOR and ANGULAR_VELOCITY_FACTOR dampens angular velocity, if factors are too high we will get inverse damping
static const btScalar TARGET_ROTATION_FACTOR = .2f;// strength of auto rotation towards target direction
static const btScalar ANGULAR_VELOCITY_FACTOR = .7f;// dampens current angular velocity
	
static const btScalar ROLL_FACTOR = 3.0f;// controls roll force magnitude
static const btScalar YAW_FACTOR = .3f;
static const btScalar PITCH_FACTOR = 2.f;



Protagonist::Protagonist(const glm::vec3 &position) : MovableObject(position), lWing(L_WING_OFFSET), rWing(R_WING_OFFSET)
{
	initPhysics();
#if RENDER_DEBUG
	triangle = new DebugMesh(GL_TRIANGLES);
	static std::vector<DebugMesh::vertex_t> vertices;
	DebugMesh::vertex_t v;
	v.color = glm::vec4(1.f, 0.f, 0.f, 1.f);
	v.pos = glm::vec3(-0.5f, 0.f, -0.5f);
	vertices.push_back(v);

	v.color = glm::vec4(0.f, 0.f, 1.f, 1.f);
	v.pos = glm::vec3(0.0f, 0.f, 0.5f);
	vertices.push_back(v);

	v.color = glm::vec4(1.f, 0.f, 0.f, 1.f);
	v.pos = glm::vec3(0.5f, 0.f, -0.5f);
	vertices.push_back(v);

	v.color = glm::vec4(0.f, 1.f, 0.f, 1.f);
	v.pos = glm::vec3(0.f, 0.25f, -0.5f);
	vertices.push_back(v);


	triangle->set_vertices(vertices);
	static const unsigned int indices[] = { 0, 1, 2, 0, 2, 3};
	triangle->set_indices(indices, 6);
#endif
}


Protagonist::~Protagonist()
{
	delete rigidBody->getMotionState();
	delete rigidBody;
	delete shape;

#if RENDER_DEBUG
	delete triangle;
#endif
}


void Protagonist::initPhysics()
{
	shape = new btSphereShape(.5f);//new btBoxShape(btVector3(.5f,.5f,.5f));
	motionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0),btVector3(position_.x, position_.y, position_.z)));
		
	btScalar mass = MASS;
	btVector3 inertia(0,0,0);
	shape->calculateLocalInertia(mass, inertia);
	btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(mass, motionState, shape, inertia);
		
	rigidBody = new btRigidBody(rigidBodyCI);
}


void Protagonist::update()
{
	rigidBody->getMotionState()->getWorldTransform(trans_);
	rot_ = trans_.getBasis();
	pos_ = trans_.getOrigin();

	rotateTowardsTargetDirection();
	applyAerodynamics();
	syncTransform(this);
}


void Protagonist::rotateTowardsTargetDirection()
{
	currentDirection = rot_.getColumn(2).normalized();
	targetDirection = rigidBody->getLinearVelocity().normalized();
		
	// displace targetDirection based on wing normals
	targetDirection += rot_ * btVector3(
		YAW_FACTOR*lWing.normal().x(),
		(PITCH_FACTOR - lWing.normal().z()) * lWing.normal().y(), 0.f);
	targetDirection += rot_ * btVector3(
		YAW_FACTOR*rWing.normal().x(),
		(PITCH_FACTOR - rWing.normal().z()) * rWing.normal().y(), 0.f);
		
	// apply stabilization and direction auto targeting
	if (currentDirection.angle(targetDirection) > .1f)
	{
		// scale angularVelocity1 to smaller than angularVelocity0 otherwise we get inverse damping effect
		btVector3 angleTowardsTarget = TARGET_ROTATION_FACTOR * targetDirection.cross(currentDirection);
		btVector3 angularVelocity0 = ANGULAR_VELOCITY_FACTOR * rigidBody->getAngularVelocity();
		btVector3 angularVelocity1 = angularVelocity0 + angleTowardsTarget;
		rigidBody->setAngularVelocity(angularVelocity1);
	}
		
	// apply rolling, involves MASS since its an applied force
	rigidBody->applyTorque(rot_ * btVector3(0, 0,
		-(ROLL_FACTOR - lWing.normal().z()) * MASS * lWing.normal().x()));
	rigidBody->applyTorque(rot_ * btVector3(0, 0,
		-(ROLL_FACTOR - rWing.normal().z()) * MASS * rWing.normal().x()));
}


void Protagonist::applyAerodynamics()
{
	// both wings get same speed and aoa :: lift and drag modulated by individual wing coefficients
	wind = -rigidBody->getLinearVelocity();
	wind_local = rot_.inverse() * wind;
		
	speed = wind.length();
	aoa = btAtan2(wind_local.y(), wind_local.z());
		
	lWing.update(speed, aoa, rot_, wind);
	rWing.update(speed, aoa, rot_, wind);
		
	applyWingAerodynamics(lWing);
	applyWingAerodynamics(rWing);
}


void Protagonist::applyWingAerodynamics(Wing & wing)
{
	rigidBody->applyForce(wing.lift(), rot_ * wing.offset());
	rigidBody->applyForce(wing.drag(), rot_ * wing.offset());
		
//		std::cout << lWinglift.length() / lWing.drag.length() << std::endl;
//		std::cout << speed << std::endl;
//		std::cout << -rigidBody->getLinearVelocity().y() << std::endl;
}


void Protagonist::syncTransform(MovableObject * obj)
{
	float m[16];
	trans_.getOpenGLMatrix(m);
	obj->set_matrix(glm::make_mat4(m));
	obj->relative_rotate(glm::vec3(0,1,0), M_PI);
}

void Protagonist::draw() {
#if RENDER_DEBUG
	triangle->render(matrix());
#endif
}

void Protagonist::applyThrust() {
	//TODO
}
