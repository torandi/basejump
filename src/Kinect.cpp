#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include "Kinect.hpp"
#include "Protagonist.hpp"



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

	std::cout << "Got kinect: " << active_ << std::endl;
}


void Kinect::update_object(Protagonist & protagonist, float dt)
{
	// Wait for 0ms, just quickly test if it is time to process a skeleton
	if(WAIT_OBJECT_0 != WaitForSingleObject(m_hNextSkeletonEvent, 0)) {
		return;
	}


	NUI_SKELETON_FRAME skeletonFrame = {0};

   // Get the skeleton frame that is ready
   if(!SUCCEEDED(m_pNuiSensor->NuiSkeletonGetNextFrame(0, &skeletonFrame))) {
	   return;
   }


	// smooth out the skeleton data
	m_pNuiSensor->NuiTransformSmooth(&skeletonFrame, NULL);

	// We have lost the tracked person, or not started tracking a person.
	// Choose the first skeleton we find.
	if(tracked_skeleton == -1) {
		find_skeleton(skeletonFrame);

		if (tracked_skeleton > -1)
			std::cout << "skeleton found" << std::endl; 
	}

	// Process the skeleton frame
	if(tracked_skeleton == -1) {
		return;
	}


	if(skeletonFrame.SkeletonData[tracked_skeleton].eTrackingState != NUI_SKELETON_TRACKED) {
		// We have lost the tracked skeleton
		std::cout << "skeleton lost" << std::endl;
		tracked_skeleton = -1;
		return;
	}

	process_skeleton(skeletonFrame, protagonist, dt);
}


void Kinect::find_skeleton(NUI_SKELETON_FRAME & skeletonFrame)
{
	for (int i = 0 ; i < NUI_SKELETON_COUNT; ++i) {
		if(skeletonFrame.SkeletonData[i].eTrackingState == NUI_SKELETON_TRACKED) {
			tracked_skeleton = i;
			break;
		}
	}
}


btVector3 Kinect::vector4_to_btVector3(const Vector4 & v) const
{
	return btVector3(v.x, v.y, v.z);
}


inline btScalar Kinect::clamp(const btScalar & x) const
{
	return std::max(-1.f, std::min(1.f, x));
}

/*
	turn left X 0 => -0.7
	turn right X 0 => +0.7

	bend forward Y 0 => +0.5
	bend back Y 0 => -0.5

	angles 35 => 75 deg
*/
void Kinect::process_skeleton(NUI_SKELETON_FRAME & skeletonFrame, Protagonist & protagonist, float dt)
{
	Vector4 (& skeletonPositions)[20] = skeletonFrame.SkeletonData[tracked_skeleton].SkeletonPositions;

	btVector3 lShoulder = vector4_to_btVector3(skeletonPositions[NUI_SKELETON_POSITION_SHOULDER_LEFT]);
	btVector3 lHand = vector4_to_btVector3(skeletonPositions[NUI_SKELETON_POSITION_HAND_LEFT]);
	btVector3 lHip = vector4_to_btVector3(skeletonPositions[NUI_SKELETON_POSITION_HIP_LEFT]);

	btVector3 rShoulder = vector4_to_btVector3(skeletonPositions[NUI_SKELETON_POSITION_SHOULDER_RIGHT]);
	btVector3 rHand = vector4_to_btVector3(skeletonPositions[NUI_SKELETON_POSITION_HAND_RIGHT]);
	btVector3 rHip = vector4_to_btVector3(skeletonPositions[NUI_SKELETON_POSITION_HIP_RIGHT]);
	
	btVector3 lArm = lHand - lShoulder;
	btVector3 lBody = lHip - lShoulder;
	btVector3 lNormal = lArm.cross(lBody).normalized();
	btScalar lAngle = lArm.angle(lBody);

	btVector3 rArm = rHand - rShoulder;
	btVector3 rBody = rHip - rShoulder;
	btVector3 rNormal = rBody.cross(rArm).normalized();
	btScalar rAngle = rBody.angle(rArm);

	protagonist.lWing.normal(
		-clamp(lNormal.x()/.7f),
		-clamp(lNormal.y()/.5f),
		clamp((lAngle-.95f) / .35f));

	protagonist.lWing.normal(
		-clamp(rNormal.x()/.7f),
		-clamp(rNormal.y()/.5f),
		clamp((rAngle-.95f) / .35f));

	std::cout << "rAngle: " << clamp((lAngle-.95f) / .35f) << std::endl;

}
