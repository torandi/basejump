#ifndef PROTAGONIST_CPP
#define PROTAGONIST_CPP

#include "movable_object.hpp"
#define _USE_MATH_DEFINES
#include <cmath>

#ifdef WIN32
#pragma managed(push,off)
#endif

#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>

#ifdef WIN32
#pragma managed(pop)
#endif

struct Wing
{
   const btScalar
		PHI_AIR, // air density :: kg/m^3
		AOA_MAX, // max angle of attack :: rad
		PLANFORM_AREA, // wing chord cross section area :: m^2
		DRAG_COEFFICIENT; 
	
	btScalar planformCoefficient; // wing spread, modulates planform area
	btVector3 m_offset, m_normal, m_lift, m_drag;	
	
	Wing(const btVector3 & v) :
		PHI_AIR(1.2f),
		AOA_MAX(.4f),
		PLANFORM_AREA(.3f),
		DRAG_COEFFICIENT(.7f),
		m_offset(v) {}

	
	const btVector3 & offset() const {
		return m_offset; }
	
	const btVector3 & normal() const {
		return m_normal; }
	
	const btVector3 & lift() const {
		return m_lift; }
	
	const btVector3 & drag() const {
		return m_drag; }
	
	
	void offset(const btScalar & x, const btScalar & y, const btScalar & z) {
		m_offset.setValue(x, y, z); }
	
	void normal(const btScalar & x, const btScalar & y, const btScalar & z) {
		m_normal.setValue(x, y, z); }
	
	
	/* update drag and lift
	 */
	void update(btScalar & speed, btScalar & aoa, btMatrix3x3 & rot, btVector3 & wind)
	{
		m_lift = scalarLift(speed, aoa) * rot.getColumn(0).cross(-wind.normalized());
		m_drag = scalarDrag(speed, aoa) * wind.normalized();

		// LT/RT determines coefficients
		planformCoefficient = .5f * (1.f + m_normal.z());
	}
	
protected:
	// thin air foil theory estimation of symmetric airfoil with infinite wingspan is 2*pi*aoa
	// in practise this gives an optimum aoa before stall (aoa max)
	// achieve a rough curve which peaks at aoa max
	inline const btScalar liftCoefficient(btScalar aoa) {
		const btScalar _aoa = btMax(.0f, AOA_MAX - std::abs(aoa-AOA_MAX));
		return planformCoefficient * 2 * M_PI * _aoa; }
	
	//		inline float liftCoefficient(float aoa) {
	//			return aoa > 0.82f || aoa < 0 ? .0f : (aoa+.14f) * (aoa-.22f) * (aoa-.6f) * (aoa-.96f) * 96 + 1.7f; }
	//
	//		inline float liftCoefficient(float aoa) {
	//			return 2.f * M_PI * aoa; }
	
	inline const btScalar dragCoefficient(btScalar & aoa) {
		return (.5f + .5f * planformCoefficient) * DRAG_COEFFICIENT; }
	
	// dynamic pressure of incompressible fluid (in our case air)
	inline const btScalar dynamicPressure(btScalar & speed) {
		return .5f * PHI_AIR * speed * speed; }
	
	inline const btScalar partialAerodynamicForce(btScalar & speed) {
		return PLANFORM_AREA * dynamicPressure(speed); }
	
	inline const btScalar scalarLift(btScalar & speed, btScalar & aoa) {
		return liftCoefficient(aoa) * partialAerodynamicForce(speed); }
	
	inline const btScalar scalarDrag(btScalar & speed, btScalar & aoa) {
		return dragCoefficient(aoa) * partialAerodynamicForce(speed); }
};




class Protagonist : public MovableObject
{
protected:
	btCollisionShape* shape;
	btDefaultMotionState* motionState;

	void initPhysics();

	void rotateTowardsTargetDirection();
	void applyAerodynamics();
	void applyWingAerodynamics(Wing &);


public:
	btRigidBody* rigidBody;


	Wing lWing;
	Wing rWing;
	
	btTransform trans_;
	btMatrix3x3 rot_;
	btVector3 pos_;
	
	btVector3 currentDirection;
	btVector3 targetDirection;
	
	btVector3 wind;
	btVector3 wind_local;
	btScalar speed;
	btScalar aoa;


	Protagonist();
	virtual ~Protagonist();

	void update();
	void draw();
	void syncTransform(MovableObject * obj);
};

#endif
