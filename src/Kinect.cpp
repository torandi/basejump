#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "Kinect.hpp"

Kinect::~Kinect()
{
	if (m_pNuiSensor)
	{
		m_pNuiSensor->NuiShutdown();
	}
	if(m_pNuiSensor != NULL){
		m_pNuiSensor->Release();
	}
}

void Kinect::readWingNormals()
{

}



