#ifndef __OpenViBEApplication_COgreVRApplication_H__
#define __OpenViBEApplication_COgreVRApplication_H__

#include <Ogre.h>
#include <OIS/OIS.h>

namespace OpenViBEVRDemos {

	class COgreVRApplication : public Ogre::FrameListener ,OIS::KeyListener, OIS::MouseListener
	{
		public:

			/**
			* \brief Default constructor.
			*/
			COgreVRApplication();

			/**
			* \brief Destructor.
			*/
			virtual ~COgreVRApplication();
			
			/** 
			* \brief Launches the ogre loop
			*/
			virtual void go(void);

		private:
	
			/** 
			* \brief Main Ogre and custom setup steps
			* \return True if the setup is a success.
			*/
			virtual bool setup();

			/** 
			* \brief Configuration management
			* \return True if the configuration step is a success.
			*/
			virtual bool configure();

			/** 
			* \brief Parsing of the resource configuration file
			*/
			virtual void setupResources();

			// Main Ogre objects
			Ogre::Root* m_poRoot;					//!< Ogre root.
			Ogre::SceneManager* m_poSceneManager;	//!< The scene manager used.
			Ogre::RenderWindow* m_poWindow;			//!< The render window used.
			Ogre::Camera* m_poCamera;				//!< The camera used.
			Ogre::String m_sResourcePath;			//!< Path to the file resource.cfg for Ogre.

			bool m_bContinue;
			
			virtual bool initScene(void) { return true; }

			virtual bool initOIS(void); 

			/** 
			* \brief Frame started callback.
			*/
			bool frameStarted(const Ogre::FrameEvent& evt);
			
			/** 
			* \brief Frame ended callback.
			*/
			bool frameEnded(const Ogre::FrameEvent& evt) { return true; }	

			/** 
			* \brief Mouse moved callback, launched when the mouse is moved.
			*/
			bool mouseMoved(const OIS::MouseEvent& arg) { return true; }

			/** 
			* \brief Mouse pressed callback, launched when a mouse button is pressed.
			*/
			bool mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id) { return true; }
			
			/** 
			* \brief Mouse released callback, launched when a mouse button is released.
			*/
			bool mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id) { return true; }

			/** 
			* \brief Key pressed callback, launched when a key is pressed.
			*/
			bool keyPressed(const OIS::KeyEvent& evt);
			/** 
			* \brief Key released callback, launched when a key is released. 
			*/
			bool keyReleased(const OIS::KeyEvent& evt) { return true; }

			OIS::InputManager* m_poInputManager;				//!< The OIS input manager.
			OIS::Mouse* m_poMouse;								//!< The mouse.
			OIS::Keyboard* m_poKeyboard;						//!< The keyboard.
		

	};
};
#endif //__OpenViBEApplication_COgreVRApplication_H__