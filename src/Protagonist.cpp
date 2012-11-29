#include "Protagonist.hpp"

#include <glm/glm.hpp>

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
	
static const btScalar ROLL_FACTOR = 1.5f;// controls roll force magnitude
static const btScalar YAW_FACTOR = .25f;
static const btScalar PITCH_FACTOR = 2.f;



Protagonist::Protagonist() : lWing(L_WING_OFFSET), rWing(R_WING_OFFSET)
{
	initPhysics();
}


Protagonist::~Protagonist()
{
	delete rigidBody->getMotionState();
	delete rigidBody;
	delete shape;
}


void Protagonist::initPhysics()
{
	shape = new btSphereShape(.5f);//new btBoxShape(btVector3(.5f,.5f,.5f));
	motionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(100,1200,100)));
		
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
	//float angle = glm::degrees(trans_.getRotation().getAngle());
	btVector3 axis = trans_.getRotation().getAxis();
	btQuaternion rot = trans_.getRotation();
	obj->set_position(glm::vec3(pos_.x(), pos_.y(), pos_.z()));
	obj->set_orientation(glm::fquat(glm::degrees(rot.getAngle()), glm::vec3(axis.x(), axis.y(), axis.z())));
	//obj->set_rotation(glm::vec3(axis.x(), axis.y(), axis.z()), angle);	
}
