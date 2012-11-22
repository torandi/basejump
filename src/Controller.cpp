#include "Controller.hpp"

Controller::Controller()
	: active_(false) 
{ }

Controller::~Controller() {}

void Controller::update(){
	readWingNormals(/*normals*/);
    //m_protagonist->leftWing(normals[0]);
    //m_protagonist->rightWing(normals[1]);
}
