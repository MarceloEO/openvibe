#include "ovasCDriverOpenEEGModularEEG.h"
#include "ovasCConfigurationOpenEEGModularEEG.h"

#include <openvibe-toolkit/ovtk_all.h>

#include <system/Time.h>
#include <system/Memory.h>
#include <cmath>
#include <iostream>

#if defined OVAS_OS_Windows
 #include <windows.h>
 #include <winbase.h>
 #include <cstdio>
 #include <cstdlib>
 #include <commctrl.h>
 #define TERM_SPEED 57600
#elif defined OVAS_OS_Linux
 #include <cstdio>
 #include <unistd.h>
 #include <fcntl.h>
 #include <termios.h>
 #include <sys/select.h>
 #define TERM_SPEED B57600
#else
#endif

#define boolean OpenViBE::boolean
using namespace OpenViBEAcquisitionServer;
using namespace OpenViBE;
using namespace OpenViBE::Kernel;

//___________________________________________________________________//
//                                                                   //

CDriverOpenEEGModularEEG::CDriverOpenEEGModularEEG(IDriverContext& rDriverContext)
	:IDriver(rDriverContext)
	,m_pCallback(NULL)
	,m_ui32SampleCountPerSentBlock(0)
	,m_ui32DeviceIdentifier(uint32(-1))
	,m_pSample(NULL)
{
	m_oHeader.setSamplingFrequency(256);
	m_oHeader.setChannelCount(6);
}

void CDriverOpenEEGModularEEG::release(void)
{
	delete this;
}

const char* CDriverOpenEEGModularEEG::getName(void)
{
	return "OpenEEG Modular EEG P2";
}

//___________________________________________________________________//
//                                                                   //

boolean CDriverOpenEEGModularEEG::initialize(
	const uint32 ui32SampleCountPerSentBlock,
	IDriverCallback& rCallback)
{
	if(m_rDriverContext.isConnected()) { return false; }

	if(!m_oHeader.isChannelCountSet()
	 ||!m_oHeader.isSamplingFrequencySet())
	{
		return false;
	}

	m_ui16Readstate=0;
	m_ui16ExtractPosition=0;

	if(!this->initTTY(&m_i32FileDescriptor, m_ui32DeviceIdentifier!=uint32(-1)?m_ui32DeviceIdentifier:1))
	{
		return false;
	}

	m_pSample=new float32[m_oHeader.getChannelCount()*ui32SampleCountPerSentBlock];
	if(!m_pSample)
	{
		delete [] m_pSample;
		m_pSample=NULL;
		return false;
	}
	m_vChannelBuffer2.resize(6);

	m_pCallback=&rCallback;
	m_ui32SampleCountPerSentBlock=ui32SampleCountPerSentBlock;
	m_ui8LastPacketNumber=0;

	m_rDriverContext.getLogManager() << LogLevel_Debug << "ModulerEEG Device Driver initialized.\n";
	return true;
}

boolean CDriverOpenEEGModularEEG::start(void)
{
	if(!m_rDriverContext.isConnected()) { return false; }
	if(m_rDriverContext.isStarted()) { return false; }
	m_rDriverContext.getLogManager() << LogLevel_Debug << "ModulerEEG Device Driver started.\n";
	return true;
}

boolean CDriverOpenEEGModularEEG::loop(void)
{
	// printf("enter loop\n");
	if(!m_rDriverContext.isConnected()) { return false; }
	// printf("connection ok\n");

	uint32 l_ui32ReceivedSamples=0;
	uint32 l_ui32StartTime=System::Time::getTime();
	while(l_ui32ReceivedSamples < m_ui32SampleCountPerSentBlock && System::Time::getTime()-l_ui32StartTime < 1000)
	{
		boolean l_bHasSamplesPending=(m_vChannelBuffer.size()!=0);

		// printf("enter while at counter= %d\n", counter);
		if(!l_bHasSamplesPending)
		{
			int32 l_i32PacketCount=this->readPacketFromTTY(m_i32FileDescriptor);
			switch(l_i32PacketCount)
			{
				case 0: // No Packet finished
					// printf("switch 0: No Packet finished\n");
					break;

				default:
				case 1: // Packet with Samples arrived
					break;

				case -1: // Timeout, could inidicate communication error
					m_rDriverContext.getLogManager() << LogLevel_ImportantWarning << "Could not receive data from " << m_sTTYName << "\n";
					return false;
			}
		}
		else
		{
			uint32 i, j;
			uint32 l_ui32PacketCountToKeep = m_vChannelBuffer.size();

			if(l_ui32PacketCountToKeep + l_ui32ReceivedSamples > m_ui32SampleCountPerSentBlock)
			{
				l_ui32PacketCountToKeep = m_ui32SampleCountPerSentBlock - l_ui32ReceivedSamples;
			}

			m_ui8LastPacketNumber=m_ui8PacketNumber;
			// printf("Will appended %i packet(s) !\n", l_i32PacketCountToKeep);
			for(j=0; j<m_oHeader.getChannelCount(); j++)
			{
				for(i=0; i<l_ui32PacketCountToKeep; i++)
				{
					m_pSample[j*m_ui32SampleCountPerSentBlock+l_ui32ReceivedSamples+i]=(float32)m_vChannelBuffer[i][j]-512.f;
				}
			}
			m_vChannelBuffer.erase(m_vChannelBuffer.begin(), m_vChannelBuffer.begin()+l_ui32PacketCountToKeep);
			if(m_vChannelBuffer.size())
			{
				// printf("Still %i packet(s) pending...\n", m_vChannelBuffer.size());
			}
			l_ui32ReceivedSamples+=l_ui32PacketCountToKeep;
		}
	}

	if(l_ui32ReceivedSamples!=m_ui32SampleCountPerSentBlock)
	{
		// Timeout
		// printf("  l_i32ReceivedSamples!=m_ui32SampleCountPerSentBlock => Timeout\n");
		return false;
	}

	if(m_rDriverContext.isStarted())
	{
		m_pCallback->setSamples(m_pSample);
		m_rDriverContext.correctJitterSampleCount(m_rDriverContext.getSuggestedJitterCorrectionSampleCount());
	}

	// printf("end loop\n");
	return true;
}

boolean CDriverOpenEEGModularEEG::stop(void)
{
	if(!m_rDriverContext.isConnected()) { return false; }
	if(!m_rDriverContext.isStarted()) { return false; }
	m_rDriverContext.getLogManager() << LogLevel_Debug << "ModulerEEG Device Driver stopped.\n";
	return true;
}

boolean CDriverOpenEEGModularEEG::uninitialize(void)
{
	if(!m_rDriverContext.isConnected()) { return false; }
	if(m_rDriverContext.isStarted()) { return false; }

	this->closeTTY(m_i32FileDescriptor);

	m_rDriverContext.getLogManager() << LogLevel_Debug << "ModulerEEG Device Driver closed.\n";

	delete [] m_pSample;
	m_pSample=NULL;
	m_pCallback=NULL;

	return true;
}

//___________________________________________________________________//
//                                                                   //

boolean CDriverOpenEEGModularEEG::isConfigurable(void)
{
	return true;
}

boolean CDriverOpenEEGModularEEG::configure(void)
{
	CConfigurationOpenEEGModularEEG m_oConfiguration("../share/openvibe-applications/acquisition-server/interface-OpenEEG-ModularEEG.ui", m_ui32DeviceIdentifier);
	return m_oConfiguration.configure(m_oHeader);
}

//___________________________________________________________________//
//                                                                   //

boolean CDriverOpenEEGModularEEG::parseByteP2(uint8 ui8Actbyte)
{
	uint32 m_ui32ChannelCount=m_oHeader.getChannelCount();
	switch(m_ui16Readstate)
	{
		case 0:
			if(ui8Actbyte==165)
			{
				m_ui16Readstate++;
			}
			break;

		case 1:
			if(ui8Actbyte==90)
			{
				m_ui16Readstate++;
			}
			else
			{
				m_ui16Readstate=0;
			}
			break;

		case 2:
			m_ui16Readstate++;
			break;

		case 3:
			m_ui8PacketNumber=ui8Actbyte;
			m_ui16ExtractPosition=0;
			m_ui16Readstate++;
			break;

		case 4:
			if(m_ui16ExtractPosition < 12)
			{
				if((m_ui16ExtractPosition & 1) == 0)
				{
					if((uint32)(m_ui16ExtractPosition>>1)<m_ui32ChannelCount)
					{
						m_vChannelBuffer2[m_ui16ExtractPosition>>1]=((int32)ui8Actbyte)<<8;
					}
				}
				else
				{
					if((uint32)(m_ui16ExtractPosition>>1)<m_ui32ChannelCount)
					{
						m_vChannelBuffer2[m_ui16ExtractPosition>>1]+=ui8Actbyte;
					}
				}
				m_ui16ExtractPosition++;
			}
			else
			{
				m_vChannelBuffer.push_back(m_vChannelBuffer2);
				m_ui16Switches=ui8Actbyte;
				m_ui16Readstate=0;
				return true;
			}
			break;

		default:
			m_ui16Readstate=0;
			break;
	}
	return false;
}

boolean CDriverOpenEEGModularEEG::initTTY(::FD_TYPE* pFileDescriptor, uint32 ui32TTYNumber)
{
	char l_sTTYName[1024];

#if defined OVAS_OS_Windows

	::sprintf(l_sTTYName, "\\\\.\\COM%d", ui32TTYNumber);
	DCB dcb = {0};
	*pFileDescriptor=::CreateFile(
		(LPCSTR)l_sTTYName,
		GENERIC_READ|GENERIC_WRITE,
		0,
		0,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		0);

	if(*pFileDescriptor == INVALID_HANDLE_VALUE)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "Could not open Communication Port " << CString(l_sTTYName) << " for ModulerEEG Driver.\n";
		return false;
	}

	if(!::GetCommState(*pFileDescriptor, &dcb))
	{
		return false;
	}

	// update DCB rate, byte size, parity, and stop bits size
	dcb.DCBlength = sizeof(dcb);
	dcb.BaudRate  = CBR_56000;
	dcb.ByteSize  = 8;
	dcb.Parity    = NOPARITY;
	dcb.StopBits  = ONESTOPBIT;
	dcb.EvtChar   = '\0';

	// update flow control settings
	dcb.fDtrControl       = DTR_CONTROL_ENABLE;
	dcb.fRtsControl       = RTS_CONTROL_ENABLE;
	dcb.fOutxCtsFlow      = FALSE;
	dcb.fOutxDsrFlow      = FALSE;
	dcb.fDsrSensitivity   = FALSE;;
	dcb.fOutX             = FALSE;
	dcb.fInX              = FALSE;
	dcb.fTXContinueOnXoff = FALSE;
	dcb.XonChar           = 0;
	dcb.XoffChar          = 0;
	dcb.XonLim            = 0;
	dcb.XoffLim           = 0;
	dcb.fParity           = FALSE;

	::SetCommState(*pFileDescriptor, &dcb);
	::SetupComm(*pFileDescriptor, 1024, 1024);
	::EscapeCommFunction(*pFileDescriptor, SETDTR);
	::SetCommMask (*pFileDescriptor, EV_RXCHAR | EV_CTS | EV_DSR | EV_RLSD | EV_RING);

#elif defined OVAS_OS_Linux

	struct termios l_oTerminalAttributes;

	// open ttyS<i> for i < 10, else open ttyUSB<i-10>
	if(ui32TTYNumber<10)
	{
		::sprintf(l_sTTYName, "/dev/ttyS%d", ui32TTYNumber);
	}
	else
	{
		::sprintf(l_sTTYName, "/dev/ttyUSB%d", ui32TTYNumber-10);
	}

	if((*pFileDescriptor=::open(l_sTTYName, O_RDWR))==-1)
	{
		m_rDriverContext.getLogManager() << LogLevel_Error << "Could not open Communication Port " << CString(l_sTTYName) << " for ModulerEEG Driver.\n";
		return false;
	}

	if(tcgetattr(*pFileDescriptor, &l_oTerminalAttributes)!=0)
	{
		::close(*pFileDescriptor);
		*pFileDescriptor=-1;
		m_rDriverContext.getLogManager() << LogLevel_Error << "terminal: tcgetattr() failed\n";
		return false;
	}

	/* l_oTerminalAttributes.c_cflag = TERM_SPEED | CS8 | CRTSCTS | CLOCAL | CREAD; */
	l_oTerminalAttributes.c_cflag = TERM_SPEED | CS8 | CLOCAL | CREAD;
	l_oTerminalAttributes.c_iflag = 0;
	l_oTerminalAttributes.c_oflag = OPOST | ONLCR;
	l_oTerminalAttributes.c_lflag = 0;
	if(::tcsetattr(*pFileDescriptor, TCSAFLUSH, &l_oTerminalAttributes)!=0)
	{
		::close(*pFileDescriptor);
		*pFileDescriptor=-1;
		m_rDriverContext.getLogManager() << LogLevel_Error << "terminal: tcsetattr() failed\n";
		return false;
	}

#else

	return false;

#endif

	m_sTTYName = l_sTTYName;

	return true;
 }

void CDriverOpenEEGModularEEG::closeTTY(::FD_TYPE i32FileDescriptor)
{
#if defined OVAS_OS_Windows
	::CloseHandle(i32FileDescriptor);
#elif defined OVAS_OS_Linux
	::close(i32FileDescriptor);
#else
#endif
}

int32 CDriverOpenEEGModularEEG::readPacketFromTTY(::FD_TYPE i32FileDescriptor)
{
	m_rDriverContext.getLogManager() << LogLevel_Debug << "Enters readPacketFromTTY\n";

	uint8  l_ui8ReadBuffer[100];
	uint32 l_ui32BytesProcessed=0;
	int32  l_i32PacketsProcessed=0;

#if defined OVAS_OS_Windows

	uint32 l_ui32ReadLength=0;
	uint32 l_ui32ReadOk=0;
	struct _COMSTAT l_oStatus;
	::DWORD l_dwState;

	if(::ClearCommError(i32FileDescriptor, &l_dwState, &l_oStatus))
	{
		l_ui32ReadLength=l_oStatus.cbInQue;
	}
	if(l_ui32ReadLength)
	{
		// printf("Read length = %i\n", l_ui32ReadLength);
	}
	else
	{
		// printf(".");
	}

	for(l_ui32BytesProcessed=0; l_ui32BytesProcessed<l_ui32ReadLength; l_ui32BytesProcessed++)
	{
		// printf("readPacketFromTTY : for : %d\n", l_ui32BytesProcessed);
		// Read the data from the serial port.
		::ReadFile(i32FileDescriptor, l_ui8ReadBuffer, 1, (LPDWORD)&l_ui32ReadOk, 0);
		if(l_ui32ReadOk==1)
		{
			// printf("readPacketFromTTY : l_ui32ReadOk==1 \n");
			if(this->parseByteP2(l_ui8ReadBuffer[0]))
			{
				// printf("readPacketFromTTY : parseByteP2(l_ui8ReadBuffer[0]) = true => l_i32PacketsProcessed++\n");
				l_i32PacketsProcessed++;
			}
		}
	}

#elif defined OVAS_OS_Linux

	fd_set  l_inputFileDescriptorSet;
	struct timeval l_timeout;
	ssize_t l_ui32ReadLength=0;

	l_timeout.tv_sec=1;
	l_timeout.tv_usec=0;

	FD_ZERO(&l_inputFileDescriptorSet);
	FD_SET(i32FileDescriptor, &l_inputFileDescriptorSet);

	switch(::select(i32FileDescriptor+1, &l_inputFileDescriptorSet, NULL, NULL, &l_timeout))
	{
		case -1: // error or timeout
		case  0:
			return (-1);

		default:
			if(FD_ISSET(i32FileDescriptor, &l_inputFileDescriptorSet))
			{
				if((l_ui32ReadLength=::read(i32FileDescriptor, l_ui8ReadBuffer, 1)) > 0)
				{
					for(l_ui32BytesProcessed=0; l_ui32BytesProcessed<l_ui32ReadLength; l_ui32BytesProcessed++)
					{
						if(this->parseByteP2(l_ui8ReadBuffer[l_ui32BytesProcessed]))
						{
							l_i32PacketsProcessed++;
						}
					}
				}
			}
			break;
	}

#else

#endif
	// printf("l_i32PacketsProcessed=%d\n",l_i32PacketsProcessed);
	m_rDriverContext.getLogManager() << LogLevel_Debug << "Leaves readPacketFromTTY\n";
	return l_i32PacketsProcessed;
 }


