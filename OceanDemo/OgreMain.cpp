#include "stdafx.h"
#include "OgreMain.h"

using namespace Ogre;

COgreMain::COgreMain(void)
	: mRoot(NULL),
	mWindow(NULL),
	mFSLayer(NULL),
	mSceneMgr(NULL),
	mOverlaySystem(NULL),
	inputManager(NULL),
	mKeyboard(NULL),
	mMouse(NULL),
	cameraForward(false),
	cameraBackward(false),
	cameraLeft(false),
	cameraRight(false),
	cameraSpeed(40.0f),
	visibility(true)
{
}

COgreMain::~COgreMain(void)
{
}

void COgreMain::run(void)
{
	//��������
	createScene();

	//������Ⱦ
	mRoot->startRendering();

	//���ٳ���
	destroyScene();
}

void COgreMain::createScene(void)
{
	mFSLayer = OGRE_NEW_T(Ogre::FileSystemLayer, Ogre::MEMCATEGORY_GENERAL)(OGRE_VERSION_NAME);

	createRoot();
	createWindow();
	createInput();
	loadResources();

	createSceneManager();
	createContent();
}

void COgreMain::destroyScene(void)
{
	destroyContent();
	destroySceneManager();

	unloadResources();
	destroyInput();
	destroyWindow();
	destroyRoot();

	OGRE_DELETE_T(mFSLayer, FileSystemLayer, Ogre::MEMCATEGORY_GENERAL);
}

bool COgreMain::frameRenderingQueued(const Ogre::FrameEvent & evt)
{
	//�ж��������Ƿ�ر�
	if (mWindow->isClosed())
		return false;

	//��ȡ��������
	mKeyboard->capture();
	mMouse->capture();

	if (mKeyboard->isKeyDown(OIS::KC_ESCAPE))
		return false;

	//�ӵ����
	if (cameraForward || cameraBackward || cameraLeft || cameraRight)
	{
		//float cameraSpeed = 2.0f;// 2.0f
		float moveX = 0.0f;
		float moveY = 0.0f;
		float moveZ = 0.0f;
		if (cameraForward) moveZ = -cameraSpeed;
		if (cameraBackward) moveZ = cameraSpeed;
		if (cameraLeft) moveX = -cameraSpeed;
		if (cameraRight) moveX = cameraSpeed;

		mainCameraView.getCameraNode()->translate(moveX*evt.timeSinceLastFrame, 0.0f, moveZ*evt.timeSinceLastFrame, Ogre::Node::TS_LOCAL);
	}

	return true;;
}

bool COgreMain::frameStarted(const Ogre::FrameEvent & evt)
{
	//��ȡ����
	mKeyboard->capture();
	mMouse->capture();

	//��KC_ESCAPE���˳�
	if (mKeyboard->isKeyDown(OIS::KC_ESCAPE))
		return false;

	return true;
}

bool COgreMain::frameEnded(const Ogre::FrameEvent & evt)
{
	return true;
}

void COgreMain::createRoot(void)
{
	Ogre::String pluginsPath = "";
	pluginsPath = mFSLayer->getConfigFilePath("plugins.cfg");

	mRoot = NULL;
	mRoot = OGRE_NEW Ogre::Root(pluginsPath, "", "ogre.log");

	Ogre::RenderSystem* rsys = mRoot->getRenderSystemByName("Direct3D9 Rendering Subsystem");
	mRoot->setRenderSystem(rsys);

	mRoot->initialise(false);
}

void COgreMain::destroyRoot(void)
{
	if (mRoot)
	{
		mRoot->saveConfig();

		OGRE_DELETE mRoot;
		mRoot = NULL;
	}
}

void COgreMain::createWindow(void)
{
	//create RenderWindow
	Ogre::NameValuePairList params;
	params["Colour Depth"] = "32";
	params["gamma"] = "Yes";
	params["FSAA"] = "4";
	params["vsync"] = "true";

	mWindow = mRoot->createRenderWindow("OgreMain", 1280, 760, false, &params);
	mRoot->addFrameListener(this);
}

void COgreMain::createInput(void)
{
	OIS::ParamList pl;
	size_t windowHnd = 0;
	std::ostringstream windowHndStr;

	//tell OIS about the Ogre window
	mWindow->getCustomAttribute("WINDOW", &windowHnd);
	windowHndStr << windowHnd;
	pl.insert(std::make_pair(std::string("WINDOW"), windowHndStr.str()));

	//setup the manager, keyboard and mouse to handle input
	inputManager = OIS::InputManager::createInputSystem(pl);
	mKeyboard = static_cast<OIS::Keyboard*>(inputManager->createInputObject(OIS::OISKeyboard, true));
	mMouse = static_cast<OIS::Mouse*>(inputManager->createInputObject(OIS::OISMouse, true));

	//tell OIS about the window's dimensions
	unsigned int width, height, depth;
	int top, left;
	mWindow->getMetrics(width, height, depth, left, top);
	const OIS::MouseState &ms = mMouse->getMouseState();
	ms.width = width;
	ms.height = height;

	//input events
	mKeyboard->setEventCallback(this);
	mMouse->setEventCallback(this);
}

void COgreMain::destroyInput(void)
{
	inputManager->destroyInputObject(mMouse); mMouse = 0;
	inputManager->destroyInputObject(mKeyboard); mKeyboard = 0;
	OIS::InputManager::destroyInputSystem(inputManager); 
	inputManager = 0;
}

void COgreMain::destroyWindow(void)
{
	mWindow = NULL;
}

void COgreMain::loadResources(void)
{
	Ogre::String resourcePath = "";
	resourcePath = mFSLayer->getConfigFilePath("resources.cfg");

	Ogre::ConfigFile config;
	config.load(resourcePath);

	Ogre::ConfigFile::SectionIterator seci = config.getSectionIterator();
	Ogre::String secName, typeName, archName;
	while (seci.hasMoreElements())
	{
		secName = seci.peekNextKey();
		Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();
		Ogre::ConfigFile::SettingsMultiMap::iterator i;
		for (i = settings->begin(); i != settings->end(); ++i)
		{
			typeName = i->first;
			archName = i->second;
			Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
				archName, typeName, secName);
		}
	}

	Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);
	Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
}

void COgreMain::unloadResources(void)
{
	Ogre::ResourceGroupManager::ResourceManagerIterator seci =
		Ogre::ResourceGroupManager::getSingleton().getResourceManagerIterator();
	while (seci.hasMoreElements())
	{
		seci.getNext()->unloadUnreferencedResources();
	}
}

void COgreMain::createSceneManager(void)
{
	//����������
	mSceneMgr = mRoot->createSceneManager(Ogre::ST_GENERIC);

	//ƽ�������
	mOverlaySystem = new Ogre::OverlaySystem();
	mSceneMgr->addRenderQueueListener(mOverlaySystem);
}

void COgreMain::destroySceneManager(void)
{
	//ɾ��ƽ�������
	delete mOverlaySystem;
	mOverlaySystem = NULL;

	//ɾ������������
	mRoot->destroySceneManager(mSceneMgr);
	mSceneMgr = NULL;
}

void COgreMain::createContent(void)
{
	// ��ʾ������
	Ogre::Entity* axis = mSceneMgr->createEntity("axis", "Axis.mesh");
	Ogre::SceneNode* nodeAxis = mSceneMgr->getRootSceneNode()->createChildSceneNode("nodeAxis");
	nodeAxis->attachObject(axis);
	nodeAxis->setScale(1.f, 1.f, 1.f);

	// ��������ͷ
	mainCameraView.createCameraNode(mSceneMgr, "MainCamera", 0.05f, 50000.0f);

	// ������Ҫ����ͷ
	Ogre::Vector3 mainCameraPos;
	mainCameraPos.x = 0.f;
	mainCameraPos.y = 0.f;
	mainCameraPos.z = 0.f;
	mainCameraView.getCameraNode()->setPosition(mainCameraPos);

	// �����ӿ�
	mainCameraView.setViewportRect(0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1, Ogre::ColourValue(0.0f, 0.0f, 0.0f), true);
	mainCameraView.createViewport(mWindow);

	//mainOverlay = Ogre::OverlayManager::getSingleton().create("mainOverlay");
	//mainOverlay->show();

	//������Ӱ��������
	mSceneMgr->setShadowTechnique(Ogre::SHADOWTYPE_STENCIL_ADDITIVE);
	mSceneMgr->setAmbientLight(Ogre::ColourValue(0.05f, 0.05f, 0.05f));

	//̫���⣨ƽ�й⣩
	Ogre::Light* sunlight = mSceneMgr->createLight("sunlight");
	sunlight->setType(Ogre::Light::LT_DIRECTIONAL);

	// ����
	Entity* entTerra = mSceneMgr->createEntity("entTerra", "17002190PAN_2048.mesh");
	entTerra->getSubEntity(1)->setVisible(visibility);
	entTerra->getSubEntity(2)->setVisible(visibility);
	entTerra->getSubEntity(3)->setVisible(visibility);
	entTerra->getSubEntity(4)->setVisible(visibility);
	entTerra->getSubEntity(5)->setVisible(!visibility);
	entTerra->getSubEntity(6)->setVisible(!visibility);
	entTerra->getSubEntity(7)->setVisible(!visibility);
	entTerra->getSubEntity(8)->setVisible(!visibility);
	SceneNode* nodeTerraBase = mSceneMgr->createSceneNode("nodeTerraBase");
	nodeTerraBase->attachObject(entTerra);
	nodeTerraBase->setPosition(-entTerra->getBoundingBox().getCenter());
	SceneNode* nodeTerra = mSceneMgr->getRootSceneNode()->createChildSceneNode("nodeTerra");
	nodeTerra->addChild(nodeTerraBase);
	nodeTerra->setScale(0.06f, 0.12f, 0.06f);
}

void COgreMain::destroyContent(void)
{
	mainCameraView.destroyViewport(mWindow);
	mainCameraView.destroyCameraNode(mSceneMgr);

	//Ogre::OverlayManager::getSingleton().destroy(mainOverlay);
	//mainOverlay = NULL;
}

bool COgreMain::keyPressed(const OIS::KeyEvent & e)
{
	//��Ⱦģʽ�л�
	if (e.key == OIS::KC_R)
	{
		Ogre::PolygonMode pm;
		pm = mainCameraView.getCamera()->getPolygonMode();

		if (pm == Ogre::PM_SOLID)
			mainCameraView.getCamera()->setPolygonMode(Ogre::PM_WIREFRAME);
		//else if (pm == Ogre::PM_WIREFRAME)
		//	mainCameraView.getCamera()->setPolygonMode(Ogre::PM_POINTS);
		else
			mainCameraView.getCamera()->setPolygonMode(Ogre::PM_SOLID);
	}

	//�ӵ����
	if (e.key == OIS::KC_W)
	{
		cameraForward = true;
	}
	else if (e.key == OIS::KC_S)
	{
		cameraBackward = true;
	}
	else if (e.key == OIS::KC_A)
	{
		cameraLeft = true;
	}
	else if (e.key == OIS::KC_D)
	{
		cameraRight = true;
	}

	// �л��߽�
	if (e.key == OIS::KC_P)
	{
		Entity* entTerra = mSceneMgr->getEntity("entTerra");
		visibility = !visibility;
		entTerra->getSubEntity(1)->setVisible(visibility);
		entTerra->getSubEntity(2)->setVisible(visibility);
		entTerra->getSubEntity(3)->setVisible(visibility);
		entTerra->getSubEntity(4)->setVisible(visibility);
		entTerra->getSubEntity(5)->setVisible(!visibility);
		entTerra->getSubEntity(6)->setVisible(!visibility);
		entTerra->getSubEntity(7)->setVisible(!visibility);
		entTerra->getSubEntity(8)->setVisible(!visibility);
	}

	return true;
}

bool COgreMain::keyReleased(const OIS::KeyEvent & e)
{
	//�ӵ����
	if (e.key == OIS::KC_W)
	{
		cameraForward = false;
	}
	else if (e.key == OIS::KC_S)
	{
		cameraBackward = false;
	}
	else if (e.key == OIS::KC_A)
	{
		cameraLeft = false;
	}
	else if (e.key == OIS::KC_D)
	{
		cameraRight = false;
	}

	return true;
}

bool COgreMain::mouseMoved(const OIS::MouseEvent & e)
{
	//�ӵ����
	mainCameraView.getCameraNode()->yaw(Ogre::Degree(-1.0f * (float)e.state.X.rel), Ogre::Node::TS_WORLD);
	mainCameraView.getCameraNode()->pitch(Ogre::Degree(-1.0f * (float)e.state.Y.rel), Ogre::Node::TS_LOCAL);

	return true;
}

bool COgreMain::mousePressed(const OIS::MouseEvent & e, OIS::MouseButtonID id)
{
	return true;
}

bool COgreMain::mouseReleased(const OIS::MouseEvent & e, OIS::MouseButtonID id)
{
	return true;
}
