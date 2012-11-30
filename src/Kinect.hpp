#include <NuiApi.h>
#include <NuiSkeleton.h>
#include <NuiSensor.h>
#include "Controller.hpp"
#include <iostream>
#include "Protagonist.hpp"

#pragma managed(push,off)
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#pragma managed(pop)


#ifdef WIN32
#pragma managed(push,off)
#endif

#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>

#ifdef WIN32
#pragma managed(pop)
#endif

class Kinect : public Controller
{
private:
	INuiSensor* m_pNuiSensor;
	HANDLE m_hNextSkeletonEvent;
	int tracked_skeleton;

	void init();
	void find_skeleton(NUI_SKELETON_FRAME &);
	void process_skeleton(NUI_SKELETON_FRAME &, Protagonist &, float dt);

	btVector3 vector4_to_btVector3(const Vector4 &) const;
	inline btScalar clamp(const btScalar &) const;

public:
	Kinect();
	virtual ~Kinect();

	virtual void update_object(Protagonist & p, float dt);
};