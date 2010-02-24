#include "ovavrdCOgreVRApplication.h"

#include <iostream>

#include <vrpn_Tracker.h>
#include <vrpn_Button.h>
#include <vrpn_Analog.h>

using namespace OpenViBEVRDemos;
using namespace Ogre;


COgreVRApplication::COgreVRApplication()
{
	m_dClock = 0;
	m_sResourcePath = "./resources.cfg";
	m_bContinue=true;
}

COgreVRApplication::~COgreVRApplication()
{
	if(m_poVrpnPeripheral)
		delete m_poVrpnPeripheral;
	
	if(m_poCamera)
		delete m_poCamera;
	
	if(m_poWindow)
		delete m_poWindow;
	
	if( m_poInputManager ) {
        if( m_poMouse ) {
            m_poInputManager->destroyInputObject( m_poMouse );
            m_poMouse = NULL;
        }

        if( m_poKeyboard ) {
            m_poInputManager->destroyInputObject( m_poKeyboard );
            m_poKeyboard = NULL;
        }
    
        m_poInputManager->destroyInputSystem( m_poInputManager );    
	}

	/*
	if(m_poSceneManager)
		m_poRoot->destroySceneManager(m_poSceneManager);
	
	if(m_poRoot)
		delete m_poRoot;*/
}
			
void COgreVRApplication::go(void)
{
	if (!this->setup()) 
	{
		std::cerr<<"[FAILED] Setup failed, end of program."<< std::endl;
		return; 
	}

	this->initialise();

	std::cout<<std::endl<< "START RENDERING..."<<std::endl;


    m_poRoot->startRendering();
}

bool COgreVRApplication::setup()
{
	// Plugin config path setup
	Ogre::String pluginsPath;
#if defined OVA_OS_Windows
 #if defined OVA_BUILDTYPE_Debug
	pluginsPath = std::string(getenv("OGRE_HOME"))+std::string("/bin/debug/Plugins.cfg");
 #else
	pluginsPath = std::string(getenv("OGRE_HOME"))+std::string("/bin/release/Plugins.cfg");
 #endif
#elif defined OVA_OS_Linux
	printf("%p\n", getenv("OGRE_HOME"));
	pluginsPath = Ogre::String(getenv("OGRE_HOME"))+Ogre::String("/lib/OGRE/Plugins.cfg");
#else
	#error "failing text"
#endif

	// Root creation
	m_poRoot = new Ogre::Root(pluginsPath, "ogre.cfg","Ogre.log");

	// Resource handling
	this->setupResources();

	//Configuration from file or dialog window if needed
	if (!this->configure()) 
	{ 
		std::cerr<<"[FAILED] The configuration process ended unexpectedly."<< std::endl;
		return false; 
	}

	// load ressources
	Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups(); 

	// Scene graph and rendering initialisation
	m_poSceneManager = m_poRoot->createSceneManager("TerrainSceneManager", "DefaultSceneManager");

	//Camera
	m_poCamera = m_poSceneManager->createCamera("DefaultCamera");
	m_poCamera->setNearClipDistance(0.1f);
	m_poCamera->setFarClipDistance(300.0f);
	m_poCamera->setRenderingDistance(0.01f);

	// Create one viewport, entire window
    Ogre::Viewport* l_poViewPort = m_poWindow->addViewport(m_poCamera);
    l_poViewPort->setBackgroundColour(Ogre::ColourValue(0,0,0));
    // Alter the camera aspect ratio to match the viewport
    m_poCamera->setAspectRatio(Ogre::Real(l_poViewPort->getActualWidth()) / Ogre::Real(l_poViewPort->getActualHeight()));


	// Set default mipmap level (NB some APIs ignore this)
	Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);

	//Listening the frame rendering
	m_poRoot->addFrameListener(this);

	//OIS
	this->initOIS();

	//CEGUI
	this->initCEGUI();

	//VRPN
	m_poVrpnPeripheral = new CAbstractVrpnPeripheral("openvibe-vrpn@localhost");
	m_poVrpnPeripheral->init();

	return true;
}

void COgreVRApplication::setupResources()
{
	 // Load resource paths from config file
    Ogre::ConfigFile l_oConfigFile;
    l_oConfigFile.load(m_sResourcePath);
    // Go through all sections & settings in the file
    Ogre::ConfigFile::SectionIterator l_oSectionIterator = l_oConfigFile.getSectionIterator();

    Ogre::String l_sSecName, l_sTypeName, l_sArchName;
    while (l_oSectionIterator.hasMoreElements())
    {
        l_sSecName = l_oSectionIterator.peekNextKey();
		Ogre::ConfigFile::SettingsMultiMap *settings = l_oSectionIterator.getNext();
        Ogre::ConfigFile::SettingsMultiMap::iterator i;
        for (i = settings->begin(); i != settings->end(); ++i)
        {
            l_sTypeName = i->first;
            l_sArchName = i->second;
            Ogre::ResourceGroupManager::getSingleton().addResourceLocation(l_sArchName, l_sTypeName, l_sSecName);
        }
    }
}

bool COgreVRApplication::configure()
{
	if(! m_poRoot->restoreConfig())
	{
		if( ! m_poRoot->showConfigDialog() )
		{
			std::cerr<<"[FAILED] No configuration created from the dialog window."<< std::endl;
			return false;
		}
	}

	m_poWindow = m_poRoot->initialise(true,"VR Application - powered by OpenViBE");

	return true;
}




bool COgreVRApplication::initOIS() 
{
	OIS::ParamList l_oParamList;
	size_t windowHnd = 0;
	std::ostringstream windowHndStr;

	// Retrieve the rendering window
	RenderWindow* window = Ogre::Root::getSingleton().getAutoCreatedWindow();
	window->getCustomAttribute("WINDOW", &windowHnd);
	windowHndStr << windowHnd;
	l_oParamList.insert(make_pair(std::string("WINDOW"), windowHndStr.str()));
	l_oParamList.insert(std::make_pair(std::string("x11_mouse_grab"), std::string("false")));
    l_oParamList.insert(std::make_pair(std::string("x11_keyboard_grab"), std::string("false")));

	// Create the input manager
	m_poInputManager = OIS::InputManager::createInputSystem( l_oParamList );

	//Create all devices (TODO: create joystick)
	m_poKeyboard = static_cast<OIS::Keyboard*>(m_poInputManager->createInputObject( OIS::OISKeyboard, true ));
	m_poMouse = static_cast<OIS::Mouse*>(m_poInputManager->createInputObject( OIS::OISMouse, true ));

	m_poKeyboard->setEventCallback(this);
	m_poMouse->setEventCallback(this);

	std::cout<<"OIS initialised"<<std::endl;

	return true;
}

bool COgreVRApplication::initCEGUI() 
{
	RenderWindow* window = Root::getSingleton().getAutoCreatedWindow();
	
	m_poGUIRenderer = new CEGUI::OgreCEGUIRenderer(window, Ogre::RENDER_QUEUE_OVERLAY, false, 3000, m_poSceneManager);
    m_poGUISystem = new CEGUI::System(m_poGUIRenderer);

    CEGUI::SchemeManager::getSingleton().loadScheme((CEGUI::utf8*)"TaharezLookSkin.scheme");
    //CEGUI::MouseCursor::getSingleton().setImage("TaharezLook", "MouseArrow");
	
	m_poGUIWindowManager = CEGUI::WindowManager::getSingletonPtr();
	m_poSheet = m_poGUIWindowManager->createWindow("DefaultGUISheet", "Sheet");
		
	m_poGUISystem->setGUISheet(m_poSheet);

	return true;
}

//--------------------------------------------------------------

bool COgreVRApplication::keyPressed(const OIS::KeyEvent& evt)
{
	if(evt.key == OIS::KC_ESCAPE)
	{
		std::cout<<"[ESC] pressed, user termination."<<std::endl;
		m_bContinue = false;
	}

	return true;
}

bool COgreVRApplication::frameStarted(const FrameEvent& evt)
{
	m_dClock += evt.timeSinceLastFrame;
	if(m_dClock >= 1/MAX_FREQUENCY)
	{
		m_poKeyboard->capture();
		m_poMouse->capture();

		m_poVrpnPeripheral->loop();
		//the button states are added in the peripheric, but they have to be popped.
		//the basic class does not pop the states.
		
		this->process();
		m_dClock = 0;
	}

	if(!m_bContinue)
	{

	}

	return m_bContinue;
}
