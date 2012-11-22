#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "Kinect.hpp"

Kinect::Kinect()
{
	init();
}

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

void Kinect::init()
{
	bool gotKinect = false;
	m_pNuiSensor = NULL;
	m_hNextSkeletonEvent = NULL;
	HRESULT hr;
	std::cout << "Activating Kinect!" << std::endl;
	
	// Initialize m_pNuiSensor
	hr = NuiCreateSensorByIndex(0, &m_pNuiSensor);
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
			// smooth out the skeleton data
			m_pNuiSensor->NuiTransformSmooth(&skeletonFrame, NULL);

			// Process the skeleton frame
			/*
			 * We might need some smart code to choose which body to track.	
			 */
			for (int i = 0 ; i < NUI_SKELETON_COUNT; ++i)
			{
				//Notify if we found more than one skeleton
				NUI_SKELETON_TRACKING_STATE trackingState = skeletonFrame.SkeletonData[i].eTrackingState;
				float Right_Hand_z = skeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_HAND_RIGHT].z;
				if(Right_Hand_z != 0)
					std::cout << Right_Hand_z << std::endl;
			}			
        }
    }
}



