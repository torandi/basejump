#include <NuiApi.h>
#include <NuiSkeleton.h>
#include <NuiSensor.h>
#include "Controller.hpp"
#include <iostream>

class Kinect : Controller
{
private:
	INuiSensor* m_pNuiSensor;
	HANDLE m_hNextSkeletonEvent;
protected:
	virtual void readWingNormals(/*btVector3 normals[2]*/);

public:
	virtual ~Kinect();

	virtual bool init();
};