#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "Kinect.hpp"

Kinect::Kinect() : tracked_skeleton(-1)
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

			
			// We have lost the tracked person, or not started tracking a person.
			// Choose the first skeleton we find.
			if(tracked_skeleton == -1){				
				for (int i = 0 ; i < NUI_SKELETON_COUNT; ++i)
				{
					if(skeletonFrame.SkeletonData[i].eTrackingState == NUI_SKELETON_TRACKED){
						tracked_skeleton = i;
						break;
					}
				}
			}
			// Process the skeleton frame
			if(tracked_skeleton != -1){
				if(skeletonFrame.SkeletonData[tracked_skeleton].eTrackingState == NUI_SKELETON_TRACKED){
					float Right_Hand_z = skeletonFrame.SkeletonData[tracked_skeleton].SkeletonPositions[NUI_SKELETON_POSITION_HAND_RIGHT].z;
					if(Right_Hand_z != 0)
						std::cout << Right_Hand_z << std::endl;	
				} else{ // We have lost the tracked skeleton
					tracked_skeleton = -1;
				}
			}
        }
    }
}



