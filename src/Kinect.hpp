#include <NuiApi.h>
#include <NuiSkeleton.h>
#include <NuiSensor.h>
#include "Controller.hpp"

class Kinect : Controller
{
private:
	INuiSensor* m_pNuiSensor;

protected:
	virtual void readWingNormals(/*btVector3 normals[2]*/);

public:
	virtual ~Kinect();
};