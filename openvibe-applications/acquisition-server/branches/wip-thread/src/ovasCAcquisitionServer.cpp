#include "ovasCAcquisitionServer.h"

#include <openvibe-toolkit/ovtk_all.h>

#include <system/Memory.h>
#include <system/Time.h>

#include <fstream>
#include <sstream>

#include <string>
#include <algorithm>
#include <functional>
#include <cctype>
#include <cstring>

#include <cassert>

#define boolean OpenViBE::boolean

namespace
{
	// because std::tolower has multiple signatures,
	// it can not be easily used in std::transform
	// this workaround is taken from http://www.gcek.net/ref/books/sw/cpp/ticppv2/
	template <class charT>
	charT to_lower(charT c)
	{
		return std::tolower(c);
	}
};

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBEAcquisitionServer;
using namespace std;

namespace OpenViBEAcquisitionServer
{
	class CDriverContext : public OpenViBEAcquisitionServer::IDriverContext
	{
	public:

		CDriverContext(const OpenViBE::Kernel::IKernelContext& rKernelContext, const OpenViBEAcquisitionServer::CAcquisitionServer& rAcquisitionServer)
			:m_rKernelContext(rKernelContext)
			,m_rAcquisitionServer(rAcquisitionServer)
		{
		}

		virtual ILogManager& getLogManager(void) const
		{
			return m_rKernelContext.getLogManager();
		}

		virtual IConfigurationManager& getConfigurationManager(void) const
		{
			return m_rKernelContext.getConfigurationManager();
		}

		virtual boolean isConnected(void) const
		{
			return m_rAcquisitionServer.isConnected();
		}

		virtual boolean isStarted(void) const
		{
			return m_rAcquisitionServer.isStarted();
		}

		virtual boolean updateImpedance(const uint32 ui32ChannelIndex, const float64 f64Impedance)
		{
			if(!this->isConnected()) return false;
			if(this->isStarted()) return false;
#if 0
			if(f64Impedance>=0)
			{
				float64 l_dFraction=(f64Impedance*.001/20);
				if(l_dFraction>1) l_dFraction=1;

				char l_sMessage[1024];
				char l_sLabel[1024];
				char l_sImpedance[1024];
				char l_sStatus[1024];

				if(::strcmp(m_pHeader->getChannelName(ui32ChannelIndex), ""))
				{
					::strcpy(l_sLabel, m_pHeader->getChannelName(ui32ChannelIndex));
				}
				else
				{
					::sprintf(l_sLabel, "Channel %i", ui32ChannelIndex+1);
				}

				if(l_dFraction==1)
				{
					::sprintf(l_sImpedance, "Too high !");
				}
				else
				{
					::sprintf(l_sImpedance, "%.2f kOhm", f64Impedance*.001);
				}

				::sprintf(l_sStatus, "%s", l_dFraction<.25?"Good !":"Bad...");
				::sprintf(l_sMessage, "%s\n%s\n\n%s", l_sLabel, l_sImpedance, l_sStatus);

				::gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(m_vLevelMesure[ui32ChannelIndex]), l_dFraction);
				::gtk_progress_bar_set_text(GTK_PROGRESS_BAR(m_vLevelMesure[ui32ChannelIndex]), l_sMessage);
			}
			else
			{
				::gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(m_vLevelMesure[ui32ChannelIndex]), 0);
				if(f64Impedance==-1)
				{
					::gtk_progress_bar_set_text(GTK_PROGRESS_BAR(m_vLevelMesure[ui32ChannelIndex]), "Measuring...");
				}
				else if (f64Impedance==-2)
				{
					::gtk_progress_bar_set_text(GTK_PROGRESS_BAR(m_vLevelMesure[ui32ChannelIndex]), "n/a");
				}
				else
				{
					::gtk_progress_bar_set_text(GTK_PROGRESS_BAR(m_vLevelMesure[ui32ChannelIndex]), "Unknown");
				}
			}

			::gtk_widget_show_all(m_pImpedanceWindow);
#endif
			return true;
		}

		virtual void onInitialize(const IHeader& rHeader)
		{
#if 0
			m_pHeader=&rHeader;
			::GtkWidget* l_pTable=gtk_table_new(1, rHeader.getChannelCount(), true);

			for(uint32 i=0; i<rHeader.getChannelCount(); i++)
			{
				::GtkWidget* l_pProgressBar=::gtk_progress_bar_new();
				::gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(l_pProgressBar), GTK_PROGRESS_BOTTOM_TO_TOP);
				::gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(l_pProgressBar), 0);
				::gtk_progress_bar_set_text(GTK_PROGRESS_BAR(l_pProgressBar), "n/a");
				::gtk_table_attach_defaults(GTK_TABLE(l_pTable), l_pProgressBar, i, i+1, 0, 1);
				m_vLevelMesure.push_back(l_pProgressBar);
			}

			m_pImpedanceWindow=gtk_window_new(GTK_WINDOW_TOPLEVEL);
			::gtk_window_set_title(GTK_WINDOW(m_pImpedanceWindow), "Impedance check");
			::gtk_container_add(GTK_CONTAINER(m_pImpedanceWindow), l_pTable);
#endif
		}

		virtual void onStart(const IHeader& rHeader)
		{
#if 0
			m_pHeader=&rHeader;
			::gtk_widget_hide(m_pImpedanceWindow);
#endif
		}

		virtual void onStop(const IHeader& rHeader)
		{
#if 0
			m_pHeader=&rHeader;
#endif
		}

		virtual void onUninitialize(const IHeader& rHeader)
		{
#if 0
			m_pHeader=&rHeader;
			::gtk_widget_destroy(m_pImpedanceWindow);
			m_vLevelMesure.clear();
#endif
		}

	protected:

		const IKernelContext& m_rKernelContext;
		const CAcquisitionServer& m_rAcquisitionServer;
		const IHeader* m_pHeader;
#if 0
		::GtkWidget* m_pImpedanceWindow;
		std::vector < ::GtkWidget* > m_vLevelMesure;
#endif
	};
}

//___________________________________________________________________//
//                                                                   //

CAcquisitionServer::CAcquisitionServer(const IKernelContext& rKernelContext)
	:m_rKernelContext(rKernelContext)
	,m_pDriverContext(NULL)
	,m_pDriver(NULL)
	,m_pAcquisitionStreamEncoder(NULL)
	,m_pExperimentInformationStreamEncoder(NULL)
	,m_pSignalStreamEncoder(NULL)
	,m_pStimulationStreamEncoder(NULL)
	,m_pChannelLocalisationStreamEncoder(NULL)
	,m_pConnectionServer(NULL)
	,m_bInitialized(false)
	,m_bStarted(false)
	,m_bGotData(false)
{
	m_pDriverContext=new CDriverContext(rKernelContext, *this);
	m_pAcquisitionStreamEncoder=&m_rKernelContext.getAlgorithmManager().getAlgorithm(m_rKernelContext.getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_AcquisitionStreamEncoder));
	m_pExperimentInformationStreamEncoder=&m_rKernelContext.getAlgorithmManager().getAlgorithm(m_rKernelContext.getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_ExperimentInformationStreamEncoder));
	m_pSignalStreamEncoder=&m_rKernelContext.getAlgorithmManager().getAlgorithm(m_rKernelContext.getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalStreamEncoder));
	m_pStimulationStreamEncoder=&m_rKernelContext.getAlgorithmManager().getAlgorithm(m_rKernelContext.getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationStreamEncoder));
	// m_pChannelLocalisationStreamEncoder=&m_rKernelContext.getAlgorithmManager().getAlgorithm(m_rKernelContext.getAlgorithmManager().createAlgorithm(/*TODO*/));

	m_pAcquisitionStreamEncoder->initialize();
	m_pExperimentInformationStreamEncoder->initialize();
	m_pSignalStreamEncoder->initialize();
	m_pStimulationStreamEncoder->initialize();
	// m_pChannelLocalisationStreamEncoder->initialize();

	op_pAcquisitionMemoryBuffer.initialize(m_pAcquisitionStreamEncoder->getOutputParameter(OVP_GD_Algorithm_AcquisitionStreamEncoder_OutputParameterId_EncodedMemoryBuffer));
	op_pExperimentInformationMemoryBuffer.initialize(m_pExperimentInformationStreamEncoder->getOutputParameter(OVP_GD_Algorithm_ExperimentInformationStreamEncoder_OutputParameterId_EncodedMemoryBuffer));
	op_pSignalMemoryBuffer.initialize(m_pSignalStreamEncoder->getOutputParameter(OVP_GD_Algorithm_SignalStreamEncoder_OutputParameterId_EncodedMemoryBuffer));
	op_pStimulationMemoryBuffer.initialize(m_pStimulationStreamEncoder->getOutputParameter(OVP_GD_Algorithm_StimulationStreamEncoder_OutputParameterId_EncodedMemoryBuffer));
	// op_pChannelLocalisationMemoryBuffer.initialize(m_pChannelLocalisationStreamEncoder->getOutputParameter(OVP_GD_Algorithm_ChannelLocalisationStreamEncoder_OutputParameterId_EncodedMemoryBuffer));

	TParameterHandler < IMemoryBuffer* > ip_pAcquisitionExperimentInformationMemoryBuffer(m_pAcquisitionStreamEncoder->getInputParameter(OVP_GD_Algorithm_AcquisitionStreamEncoder_InputParameterId_ExperimentInformationStream));
	TParameterHandler < IMemoryBuffer* > ip_pAcquisitionSignalMemoryBuffer(m_pAcquisitionStreamEncoder->getInputParameter(OVP_GD_Algorithm_AcquisitionStreamEncoder_InputParameterId_SignalStream));
	TParameterHandler < IMemoryBuffer* > ip_pAcquisitionStimulationMemoryBuffer(m_pAcquisitionStreamEncoder->getInputParameter(OVP_GD_Algorithm_AcquisitionStreamEncoder_InputParameterId_StimulationStream));
	// TParameterHandler < IMemoryBuffer* > ip_pAcquisitionChannelLocalisationMemoryBuffer(m_pAcquisitionStreamEncoder->getInputParameter(OVP_GD_Algorithm_AcquisitionStreamEncoder_InputParameterId_ChannelLocalisationStream));

	ip_pAcquisitionExperimentInformationMemoryBuffer.setReferenceTarget(op_pExperimentInformationMemoryBuffer);
	ip_pAcquisitionSignalMemoryBuffer.setReferenceTarget(op_pSignalMemoryBuffer);
	ip_pAcquisitionStimulationMemoryBuffer.setReferenceTarget(op_pStimulationMemoryBuffer);
	// ip_pAcquisitionChannelLocalisationMemoryBuffer.setReferenceTarget(op_pStimulationMemoryBuffer);
}

CAcquisitionServer::~CAcquisitionServer(void)
{
	if(m_bStarted)
	{
		m_pDriver->stop();
		m_pDriverContext->onStop(*m_pDriver->getHeader());
		m_bStarted=false;
	}

	if(m_bInitialized)
	{
		m_pDriver->uninitialize();
		m_pDriverContext->onUninitialize(*m_pDriver->getHeader());
		m_bInitialized=false;
	}

	if(m_pConnectionServer)
	{
		m_pConnectionServer->release();
		m_pConnectionServer=NULL;
	}

	list < pair < Socket::IConnection*, uint64 > >::iterator itConnection=m_vConnection.begin();
	while(itConnection!=m_vConnection.end())
	{
		itConnection->first->release();
		itConnection=m_vConnection.erase(itConnection);
	}

	// op_pChannelLocalisationMemoryBuffer.uninitialize();
	op_pStimulationMemoryBuffer.uninitialize();
	op_pSignalMemoryBuffer.uninitialize();
	op_pExperimentInformationMemoryBuffer.uninitialize();
	op_pAcquisitionMemoryBuffer.uninitialize();

	// m_pChannelLocalisationStreamEncoder->uninitialize();
	m_pStimulationStreamEncoder->uninitialize();
	m_pSignalStreamEncoder->uninitialize();
	m_pExperimentInformationStreamEncoder->uninitialize();
	m_pAcquisitionStreamEncoder->uninitialize();

	// m_rKernelContext.getAlgorithmManager().releaseAlgorithm(*m_pChannelLocalisationStreamEncoder);
	m_rKernelContext.getAlgorithmManager().releaseAlgorithm(*m_pStimulationStreamEncoder);
	m_rKernelContext.getAlgorithmManager().releaseAlgorithm(*m_pSignalStreamEncoder);
	m_rKernelContext.getAlgorithmManager().releaseAlgorithm(*m_pExperimentInformationStreamEncoder);
	m_rKernelContext.getAlgorithmManager().releaseAlgorithm(*m_pAcquisitionStreamEncoder);

	delete m_pDriverContext;
	m_pDriverContext=NULL;
}

IDriverContext& CAcquisitionServer::getDriverContext(void)
{
	return *m_pDriverContext;
}

//___________________________________________________________________//
//                                                                   //

boolean CAcquisitionServer::loop(void)
{
	// m_rKernelContext.getLogManager() << LogLevel_Debug << "idleCB\n";

	if(!m_pDriver->loop())
	{
		m_rKernelContext.getLogManager() << LogLevel_ImportantWarning << "Something bad happened in the loop callback, stoping the acquisition\n";
		return false;
	}

	boolean l_bLabelNeedsUpdate=false;

	// Searches for new connection(s)
	if(m_pConnectionServer)
	{
		if(m_pConnectionServer->isReadyToReceive())
		{

			// Accespts new client
			Socket::IConnection* l_pConnection=m_pConnectionServer->accept();
			if(l_pConnection!=NULL)
			{
				m_rKernelContext.getLogManager() << LogLevel_Trace << "Received new connection\n";

				m_vConnection.push_back(pair < Socket::IConnection*, uint64 >(l_pConnection, 0));

				l_bLabelNeedsUpdate=true;

				m_rKernelContext.getLogManager() << LogLevel_Debug << "Creating header\n";

				// op_pChannelLocalisationMemoryBuffer->setSize(0, true);
				op_pStimulationMemoryBuffer->setSize(0, true);
				op_pSignalMemoryBuffer->setSize(0, true);
				op_pExperimentInformationMemoryBuffer->setSize(0, true);
				op_pAcquisitionMemoryBuffer->setSize(0, true);

				// m_pChannelLocalisationStreamEncoder->process(OVP_GD_Algorithm_ChannelLocalisationStreamEncoder_InputTriggerId_EncodeHeader);
				m_pStimulationStreamEncoder->process(OVP_GD_Algorithm_StimulationStreamEncoder_InputTriggerId_EncodeHeader);
				m_pSignalStreamEncoder->process(OVP_GD_Algorithm_SignalStreamEncoder_InputTriggerId_EncodeHeader);
				m_pExperimentInformationStreamEncoder->process(OVP_GD_Algorithm_ExperimentInformationStreamEncoder_InputTriggerId_EncodeHeader);
				m_pAcquisitionStreamEncoder->process(OVP_GD_Algorithm_AcquisitionStreamEncoder_InputTriggerId_EncodeHeader);

				uint64 l_ui64MemoryBufferSize=op_pAcquisitionMemoryBuffer->getSize();
				l_pConnection->sendBufferBlocking(&l_ui64MemoryBufferSize, sizeof(l_ui64MemoryBufferSize));
				l_pConnection->sendBufferBlocking(op_pAcquisitionMemoryBuffer->getDirectPointer(), (uint32)op_pAcquisitionMemoryBuffer->getSize());
			}
		}
	}

	// Sends data to connected client(s)
	// and clean disconnected client(s)
	list < pair < Socket::IConnection*, uint64 > >::iterator itConnection=m_vConnection.begin();
	while(itConnection!=m_vConnection.end())
	{
		Socket::IConnection* l_pConnection=itConnection->first;

		if(!l_pConnection->isConnected())
		{
			l_pConnection->release();
			itConnection=m_vConnection.erase(itConnection);
			l_bLabelNeedsUpdate=true;
		}
		else
		{
			// Sends buffer
			if(m_bStarted && m_bGotData)
			{
				m_rKernelContext.getLogManager() << LogLevel_Debug << "Creating buffer\n";

				// op_pChannelLocalisationMemoryBuffer->setSize(0, true);
				op_pStimulationMemoryBuffer->setSize(0, true);
				op_pSignalMemoryBuffer->setSize(0, true);
				op_pExperimentInformationMemoryBuffer->setSize(0, true);
				op_pAcquisitionMemoryBuffer->setSize(0, true);

				uint64 l_ui64TimeOffset=((itConnection->second<<32)/m_pDriver->getHeader()->getSamplingFrequency());
				TParameterHandler < IStimulationSet* > ip_pStimulationSet(m_pStimulationStreamEncoder->getInputParameter(OVP_GD_Algorithm_StimulationStreamEncoder_InputParameterId_StimulationSet));
				OpenViBEToolkit::Tools::StimulationSet::copy(*ip_pStimulationSet, m_oStimulationSet, l_ui64TimeOffset);

				// m_pChannelLocalisationStreamEncoder->process(OVP_GD_Algorithm_ChannelLocalisationStreamEncoder_InputTriggerId_EncodeBuffer);
				m_pStimulationStreamEncoder->process(OVP_GD_Algorithm_StimulationStreamEncoder_InputTriggerId_EncodeBuffer);
				m_pSignalStreamEncoder->process(OVP_GD_Algorithm_SignalStreamEncoder_InputTriggerId_EncodeBuffer);
				m_pExperimentInformationStreamEncoder->process(OVP_GD_Algorithm_ExperimentInformationStreamEncoder_InputTriggerId_EncodeBuffer);
				m_pAcquisitionStreamEncoder->process(OVP_GD_Algorithm_AcquisitionStreamEncoder_InputTriggerId_EncodeBuffer);

				uint64 l_ui64MemoryBufferSize=op_pAcquisitionMemoryBuffer->getSize();
				l_pConnection->sendBufferBlocking(&l_ui64MemoryBufferSize, sizeof(l_ui64MemoryBufferSize));
				l_pConnection->sendBufferBlocking(op_pAcquisitionMemoryBuffer->getDirectPointer(), (uint32)op_pAcquisitionMemoryBuffer->getSize());

				itConnection->second+=m_ui32SampleCountPerSentBlock;
			}
			itConnection++;
		}
	}

	// Updates 'host count' label when needed
	if(l_bLabelNeedsUpdate)
	{
		char l_sLabel[1024];
		sprintf(l_sLabel, "%u host%s connected...", (unsigned int)m_vConnection.size(), m_vConnection.size()?"s":"");
		// gtk_label_set_label(GTK_LABEL(glade_xml_get_widget(m_pGladeInterface, "label_connected_host_count")), l_sLabel);
	}

	if(m_bGotData)
	{
		m_bGotData=false;
	}
	else
	{
		const IHeader& l_rHeader=*m_pDriver->getHeader();
		// System::Time::sleep((m_ui32SampleCountPerSentBlock*1000)/(16*l_rHeader.getSamplingFrequency()));
	}

	return true;
}

//___________________________________________________________________//
//                                                                   //

boolean CAcquisitionServer::connect(IDriver& rDriver, uint32 ui32SamplingCountPerSentBlock, uint32 ui32ConnectionPort)
{
	m_rKernelContext.getLogManager() << LogLevel_Debug << "connect\n";

	m_pDriver=&rDriver;
	m_ui32SampleCountPerSentBlock=ui32SamplingCountPerSentBlock;

	m_rKernelContext.getLogManager() << LogLevel_Info << "Connecting to device...\n";

	// Initializes driver
	if(!m_pDriver->initialize(m_ui32SampleCountPerSentBlock, *this))
	{
		m_rKernelContext.getLogManager() << LogLevel_Error << "Connection failed...\n";
		return false;
	}

	m_pDriverContext->onInitialize(*m_pDriver->getHeader());

	m_rKernelContext.getLogManager() << LogLevel_Info << "Connection succeeded !\n";

	m_pConnectionServer=Socket::createConnectionServer();
	if(m_pConnectionServer->listen(ui32ConnectionPort))
	{
		m_bGotData=false;
		m_bInitialized=true;
		m_eDriverLatencyLogLevel=LogLevel_Trace; // Until we solved the acquisition stuff, hide this message - ImportantWarning;
		m_ui64ToleranceDurationBeforeWarning=m_rKernelContext.getConfigurationManager().expandAsInteger("${AcquisitionServer_ToleranceDuration}", 100);

		m_rKernelContext.getLogManager() << LogLevel_Trace << "Driver monitoring tolerance set to " << m_ui64ToleranceDurationBeforeWarning << " milliseconds\n";

		TParameterHandler < uint64 > ip_ui64BuferDuration(m_pAcquisitionStreamEncoder->getInputParameter(OVP_GD_Algorithm_AcquisitionStreamEncoder_InputParameterId_BufferDuration));

		TParameterHandler < uint64 > ip_ui64ExperimentIdentifier(m_pExperimentInformationStreamEncoder->getInputParameter(OVP_GD_Algorithm_ExperimentInformationStreamEncoder_InputParameterId_ExperimentIdentifier));
		TParameterHandler < CString* > ip_pExperimentDate(m_pExperimentInformationStreamEncoder->getInputParameter(OVP_GD_Algorithm_ExperimentInformationStreamEncoder_InputParameterId_ExperimentDate));
		TParameterHandler < uint64 > ip_ui64SubjectIdentifier(m_pExperimentInformationStreamEncoder->getInputParameter(OVP_GD_Algorithm_ExperimentInformationStreamEncoder_InputParameterId_SubjectIdentifier));
		TParameterHandler < CString* > ip_pSubjectName(m_pExperimentInformationStreamEncoder->getInputParameter(OVP_GD_Algorithm_ExperimentInformationStreamEncoder_InputParameterId_SubjectName));
		TParameterHandler < uint64 > ip_ui64SubjectAge(m_pExperimentInformationStreamEncoder->getInputParameter(OVP_GD_Algorithm_ExperimentInformationStreamEncoder_InputParameterId_SubjectAge));
		TParameterHandler < uint64 > ip_ui64SubjectGender(m_pExperimentInformationStreamEncoder->getInputParameter(OVP_GD_Algorithm_ExperimentInformationStreamEncoder_InputParameterId_SubjectGender));
		TParameterHandler < uint64 > ip_ui64LaboratoryIdentifier(m_pExperimentInformationStreamEncoder->getInputParameter(OVP_GD_Algorithm_ExperimentInformationStreamEncoder_InputParameterId_LaboratoryIdentifier));
		TParameterHandler < CString* > ip_pLaboratoryName(m_pExperimentInformationStreamEncoder->getInputParameter(OVP_GD_Algorithm_ExperimentInformationStreamEncoder_InputParameterId_LaboratoryName));
		TParameterHandler < uint64 > ip_ui64TechnicianIdentifier(m_pExperimentInformationStreamEncoder->getInputParameter(OVP_GD_Algorithm_ExperimentInformationStreamEncoder_InputParameterId_TechnicianIdentifier));
		TParameterHandler < CString* > ip_pTechnicianName(m_pExperimentInformationStreamEncoder->getInputParameter(OVP_GD_Algorithm_ExperimentInformationStreamEncoder_InputParameterId_TechnicianName));

		TParameterHandler < IMatrix* > ip_pMatrix(m_pSignalStreamEncoder->getInputParameter(OVP_GD_Algorithm_SignalStreamEncoder_InputParameterId_Matrix));
		TParameterHandler < uint64 > ip_ui64SamplingRate(m_pSignalStreamEncoder->getInputParameter(OVP_GD_Algorithm_SignalStreamEncoder_InputParameterId_SamplingRate));

		TParameterHandler < IStimulationSet* > ip_pStimulationSet(m_pStimulationStreamEncoder->getInputParameter(OVP_GD_Algorithm_StimulationStreamEncoder_InputParameterId_StimulationSet));

		const IHeader& l_rHeader=*m_pDriver->getHeader();

		ip_ui64BuferDuration=(((uint64)m_ui32SampleCountPerSentBlock)<<32)/l_rHeader.getSamplingFrequency();

		ip_ui64ExperimentIdentifier=l_rHeader.getExperimentIdentifier();
		ip_ui64SubjectIdentifier=l_rHeader.getSubjectAge();
		ip_ui64SubjectGender=l_rHeader.getSubjectGender();

		ip_ui64SamplingRate=l_rHeader.getSamplingFrequency();
		ip_pMatrix->setDimensionCount(2);
		ip_pMatrix->setDimensionSize(0, l_rHeader.getChannelCount());
		ip_pMatrix->setDimensionSize(1, m_ui32SampleCountPerSentBlock);
		for(uint32 i=0; i<l_rHeader.getChannelCount(); i++)
		{
			string l_sChannelName=l_rHeader.getChannelName(i);
			if(l_sChannelName!="")
			{
				ip_pMatrix->setDimensionLabel(0, i, l_sChannelName.c_str());
			}
			else
			{
				std::stringstream ss;
				ss << "Channel " << i+1;
				ip_pMatrix->setDimensionLabel(0, i, ss.str().c_str());
			}
		}

		// TODO Gain is ignored
	}
	else
	{
		m_rKernelContext.getLogManager() << LogLevel_Error << "Could not listen on TCP port (firewall problem ?)\n";
		return false;
	}

	return true;
}

boolean CAcquisitionServer::start(void)
{
	m_rKernelContext.getLogManager() << LogLevel_Debug << "buttonStartPressedCB\n";

	m_rKernelContext.getLogManager() << LogLevel_Info << "Starting the acquisition...\n";

	// Starts driver
	if(!m_pDriver->start())
	{
		m_rKernelContext.getLogManager() << LogLevel_Error << "Starting failed !\n";
		return false;
	}
	m_pDriverContext->onStart(*m_pDriver->getHeader());

	m_rKernelContext.getLogManager() << LogLevel_Info << "Now acquiring...\n";

	m_ui64SampleCount=0;
	m_ui64StartTime=System::Time::zgetTime();

	m_bStarted=true;
	return true;
}

boolean CAcquisitionServer::stop(void)
{
	m_rKernelContext.getLogManager() << LogLevel_Debug << "buttonStopPressedCB\n";

	m_rKernelContext.getLogManager() << LogLevel_Info << "Stoping the acquisition.\n";

	// Stops driver
	m_pDriver->stop();
	m_pDriverContext->onStop(*m_pDriver->getHeader());

	m_bStarted=false;
	return true;
}

boolean CAcquisitionServer::disconnect(void)
{
	m_rKernelContext.getLogManager() << LogLevel_Info << "Disconnecting.\n";

	if(m_bInitialized)
	{
		m_pDriver->uninitialize();
		m_pDriverContext->onUninitialize(*m_pDriver->getHeader());
	}

	list < pair < Socket::IConnection*, uint64 > >::iterator itConnection=m_vConnection.begin();
	while(itConnection!=m_vConnection.end())
	{
		itConnection->first->release();
		itConnection=m_vConnection.erase(itConnection);
	}

	if(m_pConnectionServer)
	{
		m_pConnectionServer->release();
		m_pConnectionServer=NULL;
	}

	m_bInitialized=false;
	return true;
}

//___________________________________________________________________//
//                                                                   //

void CAcquisitionServer::setSamples(const float32* pSample)
{
	if(m_bStarted)
	{
		if(m_bGotData)
		{
			// some data will be dropped !
			m_rKernelContext.getLogManager() << LogLevel_ImportantWarning << "dropped data\n";
		}
		else
		{
			TParameterHandler < IMatrix* > ip_pMatrix(m_pSignalStreamEncoder->getInputParameter(OVP_GD_Algorithm_SignalStreamEncoder_InputParameterId_Matrix));
			for(uint32 i=0; i<ip_pMatrix->getBufferElementCount(); i++)
			{
				ip_pMatrix->getBuffer()[i]=pSample[i];
			}
			m_ui64SampleCount+=m_ui32SampleCountPerSentBlock;
		}
		m_bGotData=true;

		{
			uint64 l_ui64TheoricalSampleCount=(m_pDriver->getHeader()->getSamplingFrequency() * (System::Time::zgetTime()-m_ui64StartTime))>>32;
			uint64 l_ui64ToleranceSampleCount=(m_pDriver->getHeader()->getSamplingFrequency() * m_ui64ToleranceDurationBeforeWarning) / 1000;

			if(m_ui64SampleCount < l_ui64TheoricalSampleCount + l_ui64ToleranceSampleCount && l_ui64TheoricalSampleCount + l_ui64ToleranceSampleCount < m_ui64SampleCount + 2*l_ui64ToleranceSampleCount)
			{
			}
			else
			{
				m_rKernelContext.getLogManager() << m_eDriverLatencyLogLevel << "Theorical sample per seconds and real sample per seconds does not match.\n";
				m_rKernelContext.getLogManager() << m_eDriverLatencyLogLevel << "  Received : " << m_ui64SampleCount << " samples.\n";
				m_rKernelContext.getLogManager() << m_eDriverLatencyLogLevel << "  Should have received : " << l_ui64TheoricalSampleCount << " samples.\n";
				m_rKernelContext.getLogManager() << m_eDriverLatencyLogLevel << "  Difference is : " << (l_ui64TheoricalSampleCount>m_ui64SampleCount?l_ui64TheoricalSampleCount-m_ui64SampleCount:m_ui64SampleCount-l_ui64TheoricalSampleCount) << " samples.\n";
				m_rKernelContext.getLogManager() << m_eDriverLatencyLogLevel << "  Please submit a bug report for the driver you are using.\n";
				m_eDriverLatencyLogLevel=LogLevel_Trace;
			}
		}
	}
	else
	{
		m_rKernelContext.getLogManager() << LogLevel_Warning << "The acquisition is not started\n";
	}
}

void CAcquisitionServer::setStimulationSet(const IStimulationSet& rStimulationSet)
{
	if(m_bStarted)
	{
		OpenViBEToolkit::Tools::StimulationSet::copy(m_oStimulationSet, rStimulationSet);
	}
	else
	{
		m_rKernelContext.getLogManager() << LogLevel_Warning << "The acquisition is not started\n";
	}
}