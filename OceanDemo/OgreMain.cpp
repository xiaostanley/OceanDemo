#include "stdafx.h"
#include "OgreMain.h"

using namespace Ogre;

#ifdef _USE_HYDRAX_
Hydrax::Hydrax* mHydrax = NULL;
#endif // _USE_HYDRAX_

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
	cameraLeftTurn(false),
	cameraRightTurn(false),
	cameraSpeed(40.0f),
	boundaryVisble(true)
#ifdef _USE_TERRAIN_LIQUID_
	, tliquid(NULL)
#endif
#ifdef _USE_OGRE_WATER_
	, mWater(NULL)
#endif

{
}

COgreMain::~COgreMain(void)
{
}

void COgreMain::run(void)
{
	//创建场景
	createScene();

#ifdef _LAYERED_RENDERING_

	mWindow->setActive(true);
	mWindow->setAutoUpdated(false);

	Ogre::Viewport* mViewport = mWindow->getViewport(0);
	mViewport->setAutoUpdated(false);

	while (!mWindow->isClosed())
	{
		mWindow->_beginUpdate();

		mainCameraView.getCamera()->setNearClipDistance(20.0f);
		mainCameraView.getCamera()->setFarClipDistance(100000.f);
		mViewport->setOverlaysEnabled(false);
		mWindow->_updateViewport(mViewport);
		mViewport->setOverlaysEnabled(true);

		mViewport->setClearEveryFrame(true, Ogre::FBT_DEPTH);
		mViewport->setSkiesEnabled(false);
		mainCameraView.getCamera()->setNearClipDistance(0.1f);
		mainCameraView.getCamera()->setFarClipDistance(20.f);
		//mainCameraView.getCamera()->setPolygonMode(PM_WIREFRAME);
		mWindow->_updateViewport(mViewport);
		mainCameraView.getCamera()->setPolygonMode(PM_SOLID);
		mViewport->setSkiesEnabled(true);
		mViewport->setClearEveryFrame(true, Ogre::FBT_COLOUR | Ogre::FBT_DEPTH);

		mWindow->_updateAutoUpdatedViewports();
		mWindow->_endUpdate();

		mWindow->swapBuffers();
		mRoot->renderOneFrame();

		Ogre::WindowEventUtilities::messagePump();
	}
#else
	//启动渲染
	mRoot->startRendering();
#endif

	//销毁场景
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
	//判断主窗口是否关闭
	if (mWindow->isClosed())
		return false;

	//获取键鼠输入
	mKeyboard->capture();
	mMouse->capture();

	if (mKeyboard->isKeyDown(OIS::KC_ESCAPE))
		return false;

	//视点控制
	if (cameraForward || cameraBackward || cameraLeft || cameraRight)
	{
		float heightCam = mainCameraView.getCameraNode()->_getDerivedPosition().y;
		cameraSpeed = 40.f + Ogre::Math::Abs(heightCam);

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
	if (cameraLeftTurn || cameraRightTurn)
	{
		float aglYaw = 0.f;
		if (cameraLeftTurn)		aglYaw = -1.1f;
		if (cameraRightTurn)	aglYaw = 1.1f;

		mainCameraView.getCameraNode()->yaw(Ogre::Degree(aglYaw), Ogre::Node::TS_WORLD);
	}

#ifdef _USE_OGRE_WATER_
	mWater->update(evt.timeSinceLastFrame);
#endif
#ifdef _USE_HYDRAX_
	mHydrax->update(evt.timeSinceLastFrame);
#endif // _USE_HYDRAX_


	// 显示帧率
	Ogre::OverlayElement* textArea = OverlayManager::getSingleton().getOverlayElement("CodeText");
	textArea->setCaption(Ogre::DisplayString(L"FPS: ") + Ogre::DisplayString(Ogre::StringConverter::toString(mWindow->getBestFPS())));

	return true;
}

bool COgreMain::frameStarted(const Ogre::FrameEvent & evt)
{
	//获取输入
	mKeyboard->capture();
	mMouse->capture();

	//按KC_ESCAPE键退出
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

#ifdef _GL_RENDER_SYSTEM_
	Ogre::RenderSystem* rsys = mRoot->getRenderSystemByName("OpenGL Rendering Subsystem");
#else
	Ogre::RenderSystem* rsys = mRoot->getRenderSystemByName("Direct3D9 Rendering Subsystem");
#endif
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
	inputManager->destroyInputObject(mMouse); 
	mMouse = NULL;
	inputManager->destroyInputObject(mKeyboard); 
	mKeyboard = NULL;
	OIS::InputManager::destroyInputSystem(inputManager); 
	inputManager = NULL;
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
	//场景管理器
	mSceneMgr = mRoot->createSceneManager(Ogre::ST_GENERIC);

	//平面管理器
	mOverlaySystem = new Ogre::OverlaySystem();
	mSceneMgr->addRenderQueueListener(mOverlaySystem);
}

void COgreMain::destroySceneManager(void)
{
	//删除平面管理器
	delete mOverlaySystem;
	mOverlaySystem = NULL;

	//删除场景管理器
	mRoot->destroySceneManager(mSceneMgr);
	mSceneMgr = NULL;
}

void COgreMain::createContent(void)
{
	// 显示坐标轴
	Ogre::Entity* axis = mSceneMgr->createEntity("axis", "Axis.mesh");
	Ogre::SceneNode* nodeAxis = mSceneMgr->getRootSceneNode()->createChildSceneNode("nodeAxis");
	nodeAxis->attachObject(axis);
	nodeAxis->setScale(50.f, 50.f, 50.f);

	// 创建摄像头
	mainCameraView.createCameraNode(mSceneMgr, "MainCamera", 10.f, 99999*6.0f);	// 10 10000

	// 设置主要摄像头
	Ogre::Vector3 mainCameraPos;
	mainCameraPos.x = 0.f;
	mainCameraPos.y = 0.f;
	mainCameraPos.z = 0.f;
	mainCameraView.getCameraNode()->setPosition(mainCameraPos);

	// 创建视口
	mainCameraView.setViewportRect(0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1, Ogre::ColourValue(0.0f, 0.0f, 0.0f), true);
	mainCameraView.createViewport(mWindow);

	///////////////////////////////////////////////////////////////////////////////////////////////

	// 加载字体
	Ogre::FontPtr font = Ogre::FontManager::getSingleton().create("SimHeiFont", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	font->setSource("simhei.ttf");
	font->setType(FT_TRUETYPE);
	font->setTrueTypeSize(32);
	font->setTrueTypeResolution(96);
	font->addCodePointRange(Ogre::Font::CodePointRange(33, 166));
	font->load();

	// 创建overlay
	mainOverlay = Ogre::OverlayManager::getSingleton().create("mainOverlay");
	mainOverlay->setZOrder(254);
	mainOverlay->show();

	// 创建OverlayContainer
	Ogre::OverlayContainer* ovContainer = static_cast<Ogre::OverlayContainer*>(Ogre::OverlayManager::getSingleton().createOverlayElement("BorderPanel", "Container"));
	ovContainer->setMetricsMode(GMM_RELATIVE);
	ovContainer->setPosition(0.f, 0.f);
	ovContainer->setWidth(0.2f);
	ovContainer->setHeight(0.07f);
	ovContainer->setMaterialName("Core/StatsBlockCenter");
	mainOverlay->add2D(ovContainer);

	// 创建文字显示区域
	Ogre::OverlayElement* textArea = OverlayManager::getSingleton().createOverlayElement("TextArea", "CodeText");
	textArea->setMetricsMode(GMM_RELATIVE);
	textArea->setPosition(0.01f, 0.01f);
	textArea->setWidth(0.2f);
	textArea->setHeight(0.1f);
	textArea->setParameter("font_name", "SimHeiFont");
	textArea->setParameter("char_height", Ogre::StringConverter::toString(Ogre::Real(0.05f)));
	textArea->setParameter("horz_align", "left");
	textArea->setColour(Ogre::ColourValue(1.f, 1.f, 1.f));
	textArea->setCaption(Ogre::DisplayString("FPS"));
	ovContainer->addChild(textArea);

	///////////////////////////////////////////////////////////////////////////////////////////////

	// 设置雾效
	//mSceneMgr->setFog(FOG_LINEAR, Ogre::ColourValue(0.9f, 0.9f, 0.9f), 0.0, 500.f, 5000.f);
	//mSceneMgr->setFog(FOG_EXP, Ogre::ColourValue(0.9f, 0.9f, 0.9f), 0.001f);

	//设置阴影及环境光
	mSceneMgr->setShadowTechnique(Ogre::SHADOWTYPE_STENCIL_ADDITIVE);
	mSceneMgr->setAmbientLight(Ogre::ColourValue(0.05f, 0.05f, 0.05f));

	//太阳光（平行光）
	Ogre::Light* sunlight = mSceneMgr->createLight("sunlight");
	sunlight->setType(Ogre::Light::LT_DIRECTIONAL);

	mSceneMgr->setSkyBox(true, "Examples/MorningSkyBox", 99999*3, true);

#ifdef _TERRAIN_SHOW_
	// 地形
	Entity* entTerra = mSceneMgr->createEntity("entTerra", "17002190PAN_2048.mesh");
	entTerra->getSubEntity(1)->setVisible(boundaryVisble);
	entTerra->getSubEntity(2)->setVisible(boundaryVisble);
	entTerra->getSubEntity(3)->setVisible(boundaryVisble);
	entTerra->getSubEntity(4)->setVisible(boundaryVisble);
	entTerra->getSubEntity(5)->setVisible(!boundaryVisble);
	entTerra->getSubEntity(6)->setVisible(!boundaryVisble);
	entTerra->getSubEntity(7)->setVisible(!boundaryVisble);
	entTerra->getSubEntity(8)->setVisible(!boundaryVisble);
	SceneNode* nodeTerraBase = mSceneMgr->createSceneNode("nodeTerraBase");
	nodeTerraBase->attachObject(entTerra);
	nodeTerraBase->setPosition(-entTerra->getBoundingBox().getCenter());
	SceneNode* nodeTerra = mSceneMgr->getRootSceneNode()->createChildSceneNode("nodeTerra");
	nodeTerra->addChild(nodeTerraBase);
	nodeTerra->setScale(0.06f, 0.12f, 0.06f);
#endif
	//nodeTerra->setVisible(false);

#ifdef _USE_TERRAIN_LIQUID_
	tliquid = new TerranLiquid;
	tliquid->setInputParas(mRoot, mSceneMgr, nodeTerra, "entTerra", -entTerra->getBoundingBox().getCenter());
	tliquid->setHeight(-15.f + 3.8f * 2);
	tliquid->setGridDensity(10.f);
	tliquid->initialize();
#endif

// 	SceneNode* nodeTerraLiq = mSceneMgr->createSceneNode("nodeTerraLiq");
// 	nodeTerraLiq->attachObject(tliquid);
// 	mSceneMgr->getRootSceneNode()->addChild(nodeTerraLiq);

#ifdef _USE_OGRE_WATER_
	mWater = new OgreWater::Water(mWindow, mSceneMgr, mainCameraView.getCamera());
	mWater->setWaterDustEnabled(true);
	mWater->init();
	mWater->setWaterHeight(300.0f);
#endif // _USE_OGRE_WATER_

#ifdef _USE_HYDRAX_
	mHydrax = new Hydrax::Hydrax(mSceneMgr, mainCameraView.getCamera(), mWindow->getViewport(0));
	Hydrax::Module::ProjectedGrid* mGrid = new Hydrax::Module::ProjectedGrid(mHydrax, new Hydrax::Noise::Perlin(),
		Ogre::Plane(Ogre::Vector3::UNIT_Y, Ogre::Vector3(0.f, -15.f, 0.f)), 
		Hydrax::MaterialManager::NM_VERTEX, Hydrax::Module::ProjectedGrid::Options());
// 	Hydrax::Module::RadialGrid* mGrid = new Hydrax::Module::RadialGrid(mHydrax, new Hydrax::Noise::Perlin(),
// 		Hydrax::MaterialManager::NM_VERTEX, Hydrax::Module::RadialGrid::Options());

	mHydrax->setModule(static_cast<Hydrax::Module::Module*>(mGrid));
	mHydrax->loadCfg("HydraxDemo.hdx");
	mHydrax->create();

#endif // _USE_HYDRAX_

#if 0
	// 创建海面网格
	{
		float* vertices = NULL;
		float* normals = NULL;
		float* weights = NULL;
		float* textures = NULL;
		int numVertices = 0;
		unsigned int* indices = NULL;
		int numFaces = 0;

		float xgrid = 1000.f, zgrid = 1000.f;
		size_t xseg = 50, zseg = 50;
		float xintv = xgrid / xseg;
		float zintv = zgrid / zseg;

		numVertices = (xseg + 1) * (zseg + 1);
		vertices = new float[3 * numVertices];
		normals = new float[3 * numVertices];
		textures = new float[2 * numVertices];
		weights = new float[3 * numVertices];
		for (size_t zi = 0; zi <= (size_t)zseg; zi++)
		{
			for (size_t xi = 0; xi <= (size_t)xseg; xi++)
			{
				size_t pidx = zi * (xseg + 1) + xi;
				vertices[3 * pidx] = xi * xintv - xgrid / 2.f;
				vertices[3 * pidx + 1] = 0.f;
				vertices[3 * pidx + 2] = zi * zintv - zgrid / 2.f;
				weights[3 * pidx] = xi * xintv / xgrid;
				weights[3 * pidx + 1] = weights[3 * pidx + 2] = 0.f;
				normals[3 * pidx] = normals[3 * pidx + 2] = 0.f;
				normals[3 * pidx + 1] = 1.f;
				textures[2 * pidx] = xi * xintv / xgrid;
				textures[2 * pidx + 1] = zi * zintv / zgrid;
			}
		}

		numFaces = 2 * xseg * zseg;
		indices = new unsigned int[3 * numFaces];
		size_t fidx = 0;
		for (size_t zi = 0; zi < (size_t)zseg; zi++)
		{
			for (size_t xi = 0; xi < (size_t)xseg; xi++)
			{
				size_t pidx = zi * (xseg + 1) + xi;
				indices[3 * fidx] = pidx;
				indices[3 * fidx + 1] = pidx + xseg + 1;
				indices[3 * fidx + 2] = pidx + xseg + 2;
				fidx++;
				indices[3 * fidx] = pidx;
				indices[3 * fidx + 1] = pidx + xseg + 2;
				indices[3 * fidx + 2] = pidx + 1;
				fidx++;
			}
		}

		Ogre::AxisAlignedBox box(-xgrid / 2.f, 0.f, -zgrid / 2.f, xgrid / 2.f, 0.f, zgrid / 2.f);

//		Helper::exportPlyModel("D:/Ganymede/DynamicOcean/OceanDemo/test.ply", vertices, numVertices, indices, numFaces);
		MeshGenerator::generateMesh(vertices, normals, weights, textures, NULL, numVertices,
			indices, numFaces, &box,"OceanTransition", 
			"D:/Libraries/OGRE/OgreSDK_vc14_v1_10_0/media/models/", "oceanplanew");
// 		MeshGenerator::generateMesh(vertices, normals, NULL, textures, NULL, numVertices,
// 			indices, numFaces, &box, "OceanShallow",
// 			"D:/Libraries/OGRE/OgreSDK_vc14_v1_10_0/media/models/", "oceanplane");

		delete[] vertices;
		delete[] normals;
		delete[] textures;
		delete[] weights;
		delete[] indices;
	}
#endif
#if 1
	Ogre::Entity* entOs1 = mSceneMgr->createEntity("OceanPlane1", "oceanplane.mesh");
	SceneNode* nodeOs1 = mSceneMgr->getRootSceneNode()->createChildSceneNode("nodeOceanPlane1");
	nodeOs1->attachObject(entOs1);
	nodeOs1->translate(Ogre::Vector3(0.f, -15.f, 0.f));

	Ogre::Entity* entOs2 = mSceneMgr->createEntity("OceanPlane2", "oceanplanew.mesh");
	SceneNode* nodeOs2 = mSceneMgr->getRootSceneNode()->createChildSceneNode("nodeOceanPlane2");
	nodeOs2->attachObject(entOs2);
	nodeOs2->translate(Ogre::Vector3(1000.f, -15.f, 0.f));

	Ogre::Entity* entOs3 = mSceneMgr->createEntity("OceanPlane3", "oceanplane.mesh");
	SceneNode* nodeOs3 = mSceneMgr->getRootSceneNode()->createChildSceneNode("nodeOceanPlane3");
	nodeOs3->attachObject(entOs3);
	entOs3->setMaterialName("OceanDeep");
	nodeOs3->translate(Ogre::Vector3(2000.f, -15.f, 0.f));
#endif

#if 0
	Ogre::Vector3 minVec = (entTerra->getBoundingBox().getMinimum() - entTerra->getBoundingBox().getCenter()) * nodeTerra->getScale().x;
	Ogre::Vector3 maxVec = (entTerra->getBoundingBox().getMaximum() - entTerra->getBoundingBox().getCenter()) * nodeTerra->getScale().z;

	float xgrid = Ogre::Math::Abs(maxVec.x - minVec.x);
	float ygrid = Ogre::Math::Abs(maxVec.z - minVec.z);
	xgrid = ygrid = 1000.f;
	
	Ogre::String oceanMatlName = "Ocean2_HLSL_GLSL";

	Ogre::Plane oceanSurface;
	oceanSurface.normal = Ogre::Vector3::UNIT_Y;
// 	oceanSurface.d = 15.f;
// 	Ogre::MeshManager::getSingleton().createPlane("OceanSurface", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
// 		oceanSurface, xgrid, ygrid, 50, 50, true, 1, 1, 1, Ogre::Vector3::UNIT_Z);
// 
// 	Entity* entOceanSurface = mSceneMgr->createEntity("entOceanSurface", "OceanSurface");
// 	SceneNode* nodeOceanSurface = mSceneMgr->getRootSceneNode()->createChildSceneNode("nodeOceanSurface");
// 	nodeOceanSurface->attachObject(entOceanSurface);
// 	entOceanSurface->setMaterialName(oceanMatlName);

	oceanSurface.d = 0.f;
	MeshPtr osptr = Ogre::MeshManager::getSingleton().createPlane("OceanSurface", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		oceanSurface, xgrid, ygrid, 50, 50, true, 1, 1, 1, Ogre::Vector3::UNIT_Z);
	osptr->getSubMesh(0)->setMaterialName(oceanMatlName);

	// 导出
	MeshSerializer ser;
	ser.exportMesh(osptr.getPointer(), "D:/Libraries/OGRE/OgreSDK_vc14_v1_10_0/media/models/oceanplane.mesh");

	Entity* entOceanSurface = mSceneMgr->createEntity("entOceanSurface", "oceanplane.mesh");
	SceneNode* nodeOceanSurface = mSceneMgr->getRootSceneNode()->createChildSceneNode("nodeOceanSurface");
	nodeOceanSurface->attachObject(entOceanSurface);
	nodeOceanSurface->showBoundingBox(true);

// 	Ogre::Plane os2;
// 	os2.normal = Ogre::Vector3::UNIT_Y;
// 	os2.d = oceanSurface.d;
// 	Ogre::MeshManager::getSingleton().createPlane("OceanSurface2", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
// 		os2, xgrid, ygrid, 100, 100, true, 1, 1, 1, Ogre::Vector3::UNIT_Z);
// 	Entity* entOS2 = mSceneMgr->createEntity("entOS2", "OceanSurface2");
// 	SceneNode* nodeOS2 = mSceneMgr->getRootSceneNode()->createChildSceneNode("nodeOS2");
// 	nodeOS2->attachObject(entOS2);
// 	nodeOS2->translate(Ogre::Vector3(xgrid, 0.f, 0.f));
// 	entOS2->setMaterialName(oceanMatlName);

#endif
}

void COgreMain::destroyContent(void)
{
	mainCameraView.destroyViewport(mWindow);
	mainCameraView.destroyCameraNode(mSceneMgr);

#ifdef _USE_OGRE_WATER_
	delete mWater;
#endif
#ifdef _USE_HYDRAX_
	delete mHydrax;
#endif // _USE_HYDRAX_

#ifdef _USE_TERRAIN_LIQUID_
	delete tliquid;
#endif

	//Ogre::OverlayManager::getSingleton().destroy(mainOverlay);
	//mainOverlay = NULL;
}

bool COgreMain::keyPressed(const OIS::KeyEvent & e)
{
	//渲染模式切换
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

	//视点控制
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
	else if (e.key == OIS::KC_LEFT)
	{
		cameraLeftTurn = true;
	}
	else if (e.key == OIS::KC_RIGHT)
	{
		cameraRightTurn = true;
	}

#ifdef _TERRAIN_SHOW_
	// 切换边界
	if (e.key == OIS::KC_P)
	{
		Entity* entTerra = mSceneMgr->getEntity("entTerra");
		boundaryVisble = !boundaryVisble;
		entTerra->getSubEntity(1)->setVisible(boundaryVisble);
		entTerra->getSubEntity(2)->setVisible(boundaryVisble);
		entTerra->getSubEntity(3)->setVisible(boundaryVisble);
		entTerra->getSubEntity(4)->setVisible(boundaryVisble);
		entTerra->getSubEntity(5)->setVisible(!boundaryVisble);
		entTerra->getSubEntity(6)->setVisible(!boundaryVisble);
		entTerra->getSubEntity(7)->setVisible(!boundaryVisble);
		entTerra->getSubEntity(8)->setVisible(!boundaryVisble);
	}
	// 不显示地形
	if (e.key == OIS::KC_I)
	{
		SceneNode* nodeTerra = mSceneMgr->getSceneNode("nodeTerra");
		nodeTerra->flipVisibility(true);
	}
	// Z-Fighting演示
// 	if (e.key == OIS::KC_U)
// 	{
// 		mainCameraView.getCamera()->setNearClipDistance(0.1f);
// 	}
// 	else if (e.key == OIS::KC_Y)
// 	{
// 		mainCameraView.getCamera()->setNearClipDistance(10.f);
// 	}
#endif

	return true;
}

bool COgreMain::keyReleased(const OIS::KeyEvent & e)
{
	//视点控制
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
	else if (e.key == OIS::KC_LEFT)
	{
		cameraLeftTurn = false;
	}
	else if (e.key == OIS::KC_RIGHT)
	{
		cameraRightTurn = false;
	}

	return true;
}

bool COgreMain::mouseMoved(const OIS::MouseEvent & e)
{
	//视点控制
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

