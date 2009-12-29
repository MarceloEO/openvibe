#include "ovasCDriverBrainquiryQpet9.h"
#include "../ovasCConfigurationGlade.h"

#include <openvibe-toolkit/ovtk_all.h>

#include <system/Time.h>

#include <math.h>

#include <iostream>

using namespace OpenViBEAcquisitionServer;
using namespace OpenViBE;
using namespace std;

//___________________________________________________________________//
//                                                                   //

CDriverBrainquiryQpet9::CDriverBrainquiryQpet9(void)
	:m_pCallback(NULL)
	,m_bInitialized(false)
	,m_bStarted(false)
	,m_ui32SampleCountPerSentBlock(0)
	,m_pSample(NULL)
	,m_ui32TotalSampleCount(0)
	,m_ui32StartTime(0)
{
	m_oHeader.setSamplingFrequency(512);
	m_oHeader.setChannelCount(4);
}

void CDriverBrainquiryQpet9::release(void)
{
	delete this;
}

const char* CDriverBrainquiryQpet9::getName(void)
{
	return "Brainquiry Qpet-9";
}

//___________________________________________________________________//
//                                                                   //

boolean CDriverBrainquiryQpet9::initialize(
	const uint32 ui32SampleCountPerSentBlock,
	IDriverCallback& rCallback)
{
	if(m_bInitialized)
	{
		return false;
	}

	if(!m_oHeader.isChannelCountSet()
	 ||!m_oHeader.isSamplingFrequencySet())
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

	m_pCallback=&rCallback;
	m_bInitialized=true;
	m_ui32SampleCountPerSentBlock=ui32SampleCountPerSentBlock;
	m_ui32TotalSampleCount=0;

	m_ui32StartTime=System::Time::getTime();
	return true;
}

boolean CDriverBrainquiryQpet9::start(void)
{
	if(!m_bInitialized)
	{
		return false;
	}

	if(m_bStarted)
	{
		return false;
	}

	m_bStarted=true;
	return m_bStarted;
}

boolean CDriverBrainquiryQpet9::loop(void)
{
	if(!m_bInitialized)
	{
		return false;
	}

	if(!m_bStarted)
	{
		return false;
	}

	uint32 l_ui32CurrentTime=System::Time::getTime();

	if(l_ui32CurrentTime-m_ui32StartTime > (1000*m_ui32TotalSampleCount)/m_oHeader.getSamplingFrequency())
	{
		CStimulationSet l_oStimulationSet;
		l_oStimulationSet.setStimulationCount(1);
		l_oStimulationSet.setStimulationIdentifier(0, 0);
		l_oStimulationSet.setStimulationDate(0, 0);
		l_oStimulationSet.setStimulationDuration(0, 0);

		for(uint32 j=0; j<m_oHeader.getChannelCount(); j++)
		{
			for(uint32 i=0; i<m_ui32SampleCountPerSentBlock; i++)
			{
#if 1
				float64 l_f64Value=
					sin(((i+m_ui32TotalSampleCount)*(j+1)*12.3)/m_oHeader.getSamplingFrequency())+
					sin(((i+m_ui32TotalSampleCount)*(j+1)* 4.5)/m_oHeader.getSamplingFrequency())+
					sin(((i+m_ui32TotalSampleCount)*(j+1)*67.8)/m_oHeader.getSamplingFrequency());
				m_pSample[j*m_ui32SampleCountPerSentBlock+i]=(float32)l_f64Value;
#else
				m_pSample[j*m_ui32SampleCountPerSentBlock+i]=j;
#endif
			}
		}

		m_ui32TotalSampleCount+=m_ui32SampleCountPerSentBlock;
		m_pCallback->setSamples(m_pSample);
		m_pCallback->setStimulationSet(l_oStimulationSet);
	}

	return true;
}

boolean CDriverBrainquiryQpet9::stop(void)
{
	if(!m_bInitialized)
	{
		return false;
	}

	if(!m_bStarted)
	{
		return false;
	}

	m_bStarted=false;
	return !m_bStarted;
}

boolean CDriverBrainquiryQpet9::uninitialize(void)
{
	if(!m_bInitialized)
	{
		return false;
	}

	if(m_bStarted)
	{
		return false;
	}

	m_bInitialized=false;

	delete [] m_pSample;
	m_pSample=NULL;
	m_pCallback=NULL;

	return true;
}

//___________________________________________________________________//
//                                                                   //

boolean CDriverBrainquiryQpet9::isConfigurable(void)
{
	return true;
}

boolean CDriverBrainquiryQpet9::configure(void)
{
	CConfigurationGlade m_oConfiguration("../share/openvibe-applications/acquisition-server/interface-Generic-Oscillator.glade");
	return m_oConfiguration.configure(m_oHeader);
}
