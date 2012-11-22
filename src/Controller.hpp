

class Controller
{
protected:
	virtual void readWingNormals(/*btVector3 normals[2]*/) = 0; //TODO: Uncomment when bullet is in!

	bool active_;
public:
	Controller();
	virtual ~Controller();
	void update();
	/*
	 * Call init to active the controller
	 */
	virtual bool init() = 0;

	/*
	 * Returns true if the controller has been succesfully activated
	 */
	bool active() const { return active_; }
};
