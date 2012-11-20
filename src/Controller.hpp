

class Controller
{
protected:
	virtual void readWingNormals(/*btVector3 normals[2]*/) = 0; //TODO: Uncomment when bullet is in!

public:
	virtual ~Controller();
	void update();

};