#ifndef CONTROLLER_CPP
#define CONTROLLER_CPP



class Controller
{
protected:
	//virtual void readWingNormals(/*btVector3 normals[2]*/) = 0; //TODO: Uncomment when bullet is in!

	bool active_;
public:
	Controller();
	virtual ~Controller();

	virtual void update_object(Protagonist &, float dt) = 0;


	/*
	 * Call init to active the controller
	 */
	//virtual void init() = 0;

	/*
	 * Returns true if the controller has been succesfully activated
	 */
	bool active() const { return active_; }
};

#endif 
