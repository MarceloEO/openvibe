#if defined TARGET_HAS_ThirdPartyThinkGearAPI

#include "ovasCDriverNeuroskyMindset.h"
#include "ovasCConfigurationNeuroskyMindset.h"

#include "thinkgear.h"

#include <sstream>

using namespace OpenViBEAcquisitionServer;
using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace std;


//___________________________________________________________________//
//                                                                   //

CDriverNeuroskyMindset::CDriverNeuroskyMindset(IDriverContext& rDriverContext)
	:IDriver(rDriverContext)
	,m_pCallback(NULL)
	,m_ui32SampleCountPerSentBlock(0)
	,m_ui32TotalSampleCount(0)
	,m_pSample(NULL)
{
	/*
	11 channels
	-----
	eSense values:
	TG_DATA_ATTENTION
	TG_DATA_MEDITATION
	
	Raw EEG data:
	TG_DATA_RAW     (512 Hz sampling frequency)
	
	Power in defined frequency bands:
	TG_DATA_DELTA   (0.5 - 2.75 Hz)
	TG_DATA_THETA   (3.5 - 6.75 Hz)
	TG_DATA_ALPHA1  (7.5 - 9.25 Hz)
	TG_DATA_ALPHA2  (10 - 11.75 Hz)
	TG_DATA_BETA1   (13 - 16.75 Hz)
	TG_DATA_BETA2   (18 - 29.75 Hz)
	TG_DATA_GAMMA1  (31 - 39.75 Hz)
	TG_DATA_GAMMA2  (41 - 49.75 Hz)
	*/

	// this token allows user to use all data coming from headset. 
	// Raw EEG is sampled at 512Hz, other data are sampled around 1Hz 
	// so the driver sends these data at 512 Hz, changing value every seconds (we obtain a square signal)
	if(m_rDriverContext.getConfigurationManager().expandAsBoolean("${AcquisitionServer_NeuroskyMindset_FullData}", false))
	{
		m_oHeader.setChannelCount(11);
		m_oHeader.setChannelName(1,"Attention");
		m_oHeader.setChannelName(2,"Meditation");
		m_oHeader.setChannelName(3,"Delta");
		m_oHeader.setChannelName(4,"Theta");
		m_oHeader.setChannelName(5,"Low Alpha");
		m_oHeader.setChannelName(6,"High Alpha");
		m_oHeader.setChannelName(7,"Low Beta");
		m_oHeader.setChannelName(8,"High Beta");
		m_oHeader.setChannelName(9,"Low Gamma");
		m_oHeader.setChannelName(10,"Mid Gamma");
	}
	else
	{
		m_oHeader.setChannelCount(1); // one channel on the forhead
		m_oHeader.setChannelName(0,"Forehead");
	}

	m_oHeader.setSamplingFrequency(512); // raw signal sampling frequency, from the official documentation.

	m_i32ConnectionID = -1;

	m_ui32ComPort = OVAS_MINDSET_INVALID_COM_PORT;

}

CDriverNeuroskyMindset::~CDriverNeuroskyMindset(void)
{
}

const char* CDriverNeuroskyMindset::getName(void)
{
	return "NeuroSky MindSet";
}

//___________________________________________________________________//
//                                                                   //

boolean CDriverNeuroskyMindset::initialize(
	const uint32 ui32SampleCountPerSentBlock,
	IDriverCallback& rCallback)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "Mindset Driver: INIT called.\n";
	if(m_rDriverContext.isConnected())
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "[INIT] Mindset Driver: Driver already initialized.\n";
		return false;
	}

	if(!m_oHeader.isChannelCountSet()
	 ||!m_oHeader.isSamplingFrequencySet())
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "[INIT] Mindset Driver: Channel count or frequency not set.\n";
		return false;
	}

	// Builds up a buffer to store acquired samples. This buffer will be sent to the acquisition server later.
	m_pSample=new float32[m_oHeader.getChannelCount()*ui32SampleCountPerSentBlock];
	if(!m_pSample)
	{
		delete [] m_pSample;
		m_pSample=NULL;
		m_rDriverContext.getLogManager() << LogLevel_Error << "[INIT] Mindset Driver: Samples allocation failed.\n";
		return false;
	}


	int   l_iDllVersion = 0;
	int   l_iConnectionId = 0;
	int   l_iErrCode = 0;

	/* Print driver version number */
	l_iDllVersion = TG_GetDriverVersion();
	m_rDriverContext.getLogManager() << LogLevel_Info << "ThinkGear DLL version: "<< l_iDllVersion <<"\n";

	/* Get a connection ID handle to ThinkGear */
	l_iConnectionId = TG_GetNewConnectionId();
	if( l_iConnectionId < 0 ) 
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "TG_GetNewConnectionId() returned "<< l_iConnectionId <<".\n";
		return false;
	}

	m_rDriverContext.getLogManager() << LogLevel_Info << "ThinkGear Device ID is: "<< l_iConnectionId <<".\n";
	m_rDriverContext.getLogManager() << LogLevel_Info << "ThinkGear Communication Driver Ready.\n";
	
	m_i32ConnectionID = l_iConnectionId;

	// if no com port was selected with the property dialog window
	if(m_ui32ComPort == OVAS_MINDSET_INVALID_COM_PORT)
	{
		m_rDriverContext.getLogManager() << LogLevel_Info << "Scanning COM ports 1 to 16...\n";
		bool l_bComPortFound = false;
		// try the com ports
		for(uint32 i=1; i<16 && !l_bComPortFound; i++)
		{
			/* Attempt to connect the connection ID handle to serial port */
			stringstream l_ssComPortName;
			l_ssComPortName << "\\\\.\\COM" << i;
			int l_iErrCode = TG_Connect(l_iConnectionId,l_ssComPortName.str().c_str(),TG_BAUD_9600,TG_STREAM_PACKETS );
			if( l_iErrCode >= 0 ) 
			{
				m_rDriverContext.getLogManager() << LogLevel_Info << "Connection available on port COM"<< i <<" -- STATUS: ";
		
				//we try to read one packet, to check the connection.
				l_iErrCode = TG_ReadPackets(l_iConnectionId,1);
				if(l_iErrCode >= 0)
				{	
					m_ui32ComPort = i;
					l_bComPortFound = true;
				}
				else
				{
					if(l_iErrCode == -1) printf("FAILED (Invalid connection ID)\n");
					if(l_iErrCode == -2) printf("FAILED (0 bytes on the stream)\n");
					if(l_iErrCode == -3) printf("FAILED (I/O error occured)\n");
				}
				TG_Disconnect(l_iConnectionId);		
			}
		}

		if(!l_bComPortFound)
		{
			m_rDriverContext.getLogManager() << LogLevel_Error << "The driver was unable to find any valid device on serial port COM1 to COM16.\n";
			return false;
		}
	}

	//__________________________________
	// Saves parameters
	m_pCallback=&rCallback;
	m_ui32SampleCountPerSentBlock=ui32SampleCountPerSentBlock;

	return true;
}

boolean CDriverNeuroskyMindset::start(void)
{
	m_rDriverContext.getLogManager() << LogLevel_Trace << "Mindset Driver: START called.\n";
	if(!m_rDriverContext.isConnected())
	{
		return false;
	}

	if(m_rDriverContext.isStarted())
	{
		return false;
	}

	if(m_ui32ComPort == OVAS_MINDSET_INVALID_COM_PORT)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "No valid Serial COM port detected.\n";
		return false;
	}

	int   l_iErrCode = 0;

	/* Attempt to connect the connection ID handle to serial port */
	stringstream l_ssComPortName;
	l_ssComPortName << "\\\\.\\COM" << m_ui32ComPort;
	m_rDriverContext.getLogManager() << LogLevel_Info << "Trying to connect to Serial Port COM"<< m_ui32ComPort <<".\n";
	l_iErrCode = TG_Connect(m_i32ConnectionID,l_ssComPortName.str().c_str(),TG_BAUD_9600,TG_STREAM_PACKETS );
	if( l_iErrCode < 0 ) 
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "TG_Connect() returned "<< l_iErrCode <<".\n";
		return false;
	}

	return true;

}

boolean CDriverNeuroskyMindset::loop(void)
{
	if(!m_rDriverContext.isConnected())
	{
		return false;
	}


	if(m_rDriverContext.isStarted())
	{
		
		uint32 l_i32ReceivedSamples=0;
		int32 l_i32ErrorCode;

		while(l_i32ReceivedSamples < m_ui32SampleCountPerSentBlock)
		{
			/* Attempt to read 1 Packet of data from the connection (numPackets = -1 means all packets) */
			l_i32ErrorCode = TG_ReadPackets( m_i32ConnectionID, 1 );
			
			if(l_i32ErrorCode == 1)// we asked for 1 packet and received 1
			{
				/* If raw value has been updated by TG_ReadPackets()... */
				if( TG_GetValueStatus(m_i32ConnectionID, TG_DATA_RAW ) != 0 )
				{
					float32 raw_value = (float32) TG_GetValue(m_i32ConnectionID, TG_DATA_RAW);
					m_pSample[l_i32ReceivedSamples] = raw_value;
					l_i32ReceivedSamples++;
     			}

				if(m_rDriverContext.getConfigurationManager().expandAsBoolean("${AcquisitionServer_NeuroskyMindset_FullData}", false))
				{
					float32 l_f32Value;
					uint32 l_ui32ChannelIndex = 1;
					// we don't check if the value has changed, we construct a square signal (1Hz --> 512Hz)
					
					l_f32Value = (float32) TG_GetValue(m_i32ConnectionID, TG_DATA_ATTENTION);
					m_pSample[l_ui32ChannelIndex * m_ui32SampleCountPerSentBlock + l_i32ReceivedSamples-1] = l_f32Value;
					l_ui32ChannelIndex++;

					l_f32Value = (float32) TG_GetValue(m_i32ConnectionID, TG_DATA_MEDITATION);
					m_pSample[l_ui32ChannelIndex * m_ui32SampleCountPerSentBlock + l_i32ReceivedSamples-1] = l_f32Value;
					l_ui32ChannelIndex++;

					l_f32Value = (float32) TG_GetValue(m_i32ConnectionID, TG_DATA_DELTA);
					m_pSample[l_ui32ChannelIndex * m_ui32SampleCountPerSentBlock + l_i32ReceivedSamples-1] = l_f32Value;
					l_ui32ChannelIndex++;

					l_f32Value = (float32) TG_GetValue(m_i32ConnectionID, TG_DATA_THETA);
					m_pSample[l_ui32ChannelIndex * m_ui32SampleCountPerSentBlock + l_i32ReceivedSamples-1] = l_f32Value;
					l_ui32ChannelIndex++;

					l_f32Value = (float32) TG_GetValue(m_i32ConnectionID, TG_DATA_ALPHA1);
					m_pSample[l_ui32ChannelIndex * m_ui32SampleCountPerSentBlock + l_i32ReceivedSamples-1] = l_f32Value;
					l_ui32ChannelIndex++;

					l_f32Value = (float32) TG_GetValue(m_i32ConnectionID, TG_DATA_ALPHA2);
					m_pSample[l_ui32ChannelIndex * m_ui32SampleCountPerSentBlock + l_i32ReceivedSamples-1] = l_f32Value;
					l_ui32ChannelIndex++;

					l_f32Value = (float32) TG_GetValue(m_i32ConnectionID, TG_DATA_BETA1);
					m_pSample[l_ui32ChannelIndex * m_ui32SampleCountPerSentBlock + l_i32ReceivedSamples-1] = l_f32Value;
					l_ui32ChannelIndex++;

					l_f32Value = (float32) TG_GetValue(m_i32ConnectionID, TG_DATA_BETA2);
					m_pSample[l_ui32ChannelIndex * m_ui32SampleCountPerSentBlock + l_i32ReceivedSamples-1] = l_f32Value;
					l_ui32ChannelIndex++;

					l_f32Value = (float32) TG_GetValue(m_i32ConnectionID, TG_DATA_GAMMA1);
					m_pSample[l_ui32ChannelIndex * m_ui32SampleCountPerSentBlock + l_i32ReceivedSamples-1] = l_f32Value;
					l_ui32ChannelIndex++;

					l_f32Value = (float32) TG_GetValue(m_i32ConnectionID, TG_DATA_GAMMA2);
					m_pSample[l_ui32ChannelIndex * m_ui32SampleCountPerSentBlock + l_i32ReceivedSamples-1] = l_f32Value;
				}
			}
		}

		// no stimulations received from hardware, the set is empty
		CStimulationSet l_oStimulationSet;

		m_pCallback->setSamples(m_pSample);

		// Jitter correction (preliminary tests showed a 1 sec jitter for 3 minutes of measurement)
		if(m_rDriverContext.getJitterSampleCount() > m_rDriverContext.getJitterToleranceSampleCount()
			|| m_rDriverContext.getJitterSampleCount() < - m_rDriverContext.getJitterToleranceSampleCount())
		{
			m_rDriverContext.getLogManager() << LogLevel_Trace << "Jitter detected: "<< m_rDriverContext.getJitterSampleCount() <<" samples.\n";
			m_rDriverContext.getLogManager() << LogLevel_Trace << "Suggested correction: "<< m_rDriverContext.getSuggestedJitterCorrectionSampleCount() <<" samples.\n";

			if(! m_rDriverContext.correctJitterSampleCount(m_rDriverContext.getSuggestedJitterCorrectionSampleCount()))
			{
				m_rDriverContext.getLogManager() << LogLevel_Error << "ERROR while correcting a jitter.\n";
			}
		}
		m_pCallback->setStimulationSet(l_oStimulationSet);
	}

	return true;
}

boolean CDriverNeuroskyMindset::stop(void)
{

	m_rDriverContext.getLogManager() << LogLevel_Trace << "Mindset Driver: STOP called.\n";
	if(!m_rDriverContext.isConnected())
	{
		return false;
	}

	if(!m_rDriverContext.isStarted())
	{
		return false;
	}

	//STOP CODE HERE !
	TG_Disconnect(m_i32ConnectionID);

	return true;
}

boolean CDriverNeuroskyMindset::uninitialize(void)
{
	if(!m_rDriverContext.isConnected())
	{
		return false;
	}

	if(m_rDriverContext.isStarted())
	{
		return false;
	}

	// UNINIT HERE !
	TG_FreeConnection(m_i32ConnectionID);

	delete [] m_pSample;
	m_pSample=NULL;
	m_pCallback=NULL;

	return true;
}

//___________________________________________________________________//
//                                                                   //

boolean CDriverNeuroskyMindset::isConfigurable(void)
{
	return true;
}

boolean CDriverNeuroskyMindset::configure(void)
{
	CConfigurationNeuroskyMindset m_oConfiguration(m_rDriverContext, "../share/openvibe-applications/acquisition-server/interface-Neurosky-Mindset.glade",m_ui32ComPort);

	if(!m_oConfiguration.configure(m_oHeader)) // the basic configure will use the basic header
	{
		return false;
	}


	return true;
}

#endif // TARGET_HAS_ThirdPartyThinkGearAPI
