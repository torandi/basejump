#include <NuiApi.h>
#include <NuiSkeleton.h>
#include <NuiSensor.h>
#include "Controller.hpp"
#include <iostream>

class Kinect : public Controller
{
private:
	INuiSensor* m_pNuiSensor;
	HANDLE m_hNextSkeletonEvent;
protected:
	virtual void readWingNormals(/*btVector3 normals[2]*/);

public:
	Kinect();
	virtual ~Kinect();

	virtual void init();
};