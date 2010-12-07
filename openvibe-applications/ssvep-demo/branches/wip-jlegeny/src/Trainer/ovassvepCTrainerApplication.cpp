#include "ovassvepCTrainerApplication.h"

using namespace Ogre;
using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBESSVEP;

	CTrainerApplication::CTrainerApplication(std::string s_configFileName) : 
	CApplication(),
	m_bActive( false ),
	m_poInstructionsReady( NULL )
{
}

bool CTrainerApplication::setup(OpenViBE::Kernel::IKernelContext* poKernelContext)
{
	CApplication::setup(poKernelContext);

	(*m_poLogManager) << LogLevel_Debug << "  * CTrainerApplication::setup()\n";
	CTrainerTarget::initialize( m_poPainter, m_poSceneNode );

	// paint targets

	CTrainerTarget* l_poTarget;
	l_poTarget = CTrainerTarget::createTarget(0.0, 0.0, ColourValue(0, 0, 0), 30, 30);
	this->addTarget(l_poTarget);

	l_poTarget = CTrainerTarget::createTarget(0.0, 0.5, ColourValue(1, 0, 0), 2, 1);
	this->addTarget(l_poTarget);

	l_poTarget = CTrainerTarget::createTarget(-0.4, 0.0, ColourValue(1, 0, 0), 2, 2);
	this->addTarget(l_poTarget);

	l_poTarget = CTrainerTarget::createTarget(0.4, 0.0, ColourValue(1, 0, 0), 3, 2);
	this->addTarget(l_poTarget);

	// draw the initial text

	
	m_poInstructionsReady = m_poGUIWindowManager->createWindow("TaharezLook/StaticImage", "InstructionsReady");
	m_poInstructionsReady->setPosition(CEGUI::UVector2(cegui_reldim(0.0f), cegui_reldim(0.0f)) );
	m_poInstructionsReady->setSize(CEGUI::UVector2(CEGUI::UDim(0.0f, 640.f), CEGUI::UDim(0.0f, 32.f)));

	m_poSheet->addChildWindow(m_poInstructionsReady);

	CEGUI::ImagesetManager::getSingleton().createFromImageFile("InstructionsReady","InstructionText-Start.png");

	m_poInstructionsReady->setProperty("Image","set:InstructionsReady image:full_image");
	m_poInstructionsReady->setProperty("FrameEnabled","False");
	m_poInstructionsReady->setProperty("BackgroundEnabled","False");
	m_poInstructionsReady->setVisible(true);


	// initialize commands
	(*m_poLogManager) << LogLevel_Debug << "+ addCommand(new CBasicCommand(...)\n";
	this->addCommand(new CBasicCommand( this ));

	(*m_poLogManager) << LogLevel_Debug << "+ addCommand(new CControlCommand(...))\n";
	this->addCommand(new CControlCommand( this, "ControlButton", "localhost"));

	(*m_poLogManager) << LogLevel_Debug << "+ addCommand(new CGoalCommand(...))\n";
	this->addCommand(new CGoalCommand( this, "GoalButton", "localhost"));

	(*m_poLogManager) << LogLevel_Debug << "+ addCommand(new CStartCommand(...))\n";
	this->addCommand(new CStartCommand( this ));

	return true;
}


void CTrainerApplication::processFrame(OpenViBE::uint8 ui8CurrentFrame)
{
	if (not m_bActive)
	{
		return;
	}

	for (OpenViBE::uint8 i = 0; i < m_oTargets.size(); i++)
	{
		m_oTargets[i]->processFrame(ui8CurrentFrame);
	}
}

void CTrainerApplication::addTarget(CTrainerTarget* poTarget)
{
	m_oTargets.push_back(poTarget);
	poTarget->setVisible( true );
}

void CTrainerApplication::setGoal(OpenViBE::uint8 iGoal)
{
	OpenViBE::uint32 l_ui32CurrentTime = time(NULL) - m_ttStartTime;
	(*m_poLogManager) << LogLevel_Info << l_ui32CurrentTime << "    > Goal set to " << iGoal << "\n";

	for (OpenViBE::uint8 i = 0; i < m_oTargets.size(); i++)
	{
		m_oTargets[i]->setGoal( iGoal == i );
	}
}

void CTrainerApplication::startExperiment()
{
	CApplication::startExperiment();

	m_ttStartTime = time(NULL);

	this->stopFlickering();
	m_poInstructionsReady->setVisible( false );
}

void CTrainerApplication::startFlickering()
{
	OpenViBE::uint32 l_ui32CurrentTime = time(NULL) - m_ttStartTime;
	(*m_poLogManager) << LogLevel_Info << l_ui32CurrentTime << "    > Starting Visual Stimulation\n";
	m_bActive = true;
}

void CTrainerApplication::stopFlickering()
{
	OpenViBE::uint32 l_ui32CurrentTime = time(NULL) - m_ttStartTime;
	(*m_poLogManager) << LogLevel_Info << l_ui32CurrentTime << "    > Stopping Visual Stimulation\n";
	m_bActive = false;

	for (OpenViBE::uint8 i = 0; i < m_oTargets.size(); i++)
	{
		m_oTargets[i]->setVisible(true);
	}

	this->setGoal( -1 );
}
