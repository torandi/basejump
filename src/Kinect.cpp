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

bool Kinect::init()
{
	bool gotKinect = false;
	m_pNuiSensor = NULL;
	m_hNextSkeletonEvent = NULL;
	HRESULT hr;
	// Initialize m_pNuiSensor
	hr = NuiCreateSensorByIndex(0, &m_pNuiSensor);//FindKinectSensor();
	  
	//std::cout << *number << std::endl;
	if (SUCCEEDED(hr))
	{
		// Initialize the Kinect and specify that we'll be using skeleton
		hr = m_pNuiSensor->NuiInitialize(NUI_INITIALIZE_FLAG_USES_SKELETON);
		if (SUCCEEDED(hr))
		{
			gotKinect = true;
			// Create an event that will be signaled when skeleton data is available
			m_hNextSkeletonEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

			// Open a skeleton stream to receive skeleton data
			hr = m_pNuiSensor->NuiSkeletonTrackingEnable(m_hNextSkeletonEvent, 0);
		}
	}

	active_ = gotKinect;
	return gotKinect;
}

void Kinect::readWingNormals()
{
	// Wait for 0ms, just quickly test if it is time to process a skeleton
    if ( WAIT_OBJECT_0 == WaitForSingleObject(m_hNextSkeletonEvent, 0) )
    {
		NUI_SKELETON_FRAME skeletonFrame = {0};

        // Get the skeleton frame that is ready
        if (SUCCEEDED(m_pNuiSensor->NuiSkeletonGetNextFrame(0, &skeletonFrame)))
        {
			// Process the skeleton frame
			//TODO: SkeletonFrameReady(&skeletonFrame);
			float Right_Hand_z = skeletonFrame.SkeletonData->SkeletonPositions[NUI_SKELETON_POSITION_HAND_RIGHT].z;
			std::cout << Right_Hand_z << std::endl;

        }
    }
}



