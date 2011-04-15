#include "ovpCInputChannel.h"

#include <iostream>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::SignalProcessing;

namespace
{
	class _AutoCast_
	{
	public:
		_AutoCast_(IBox& rBox, IConfigurationManager& rConfigurationManager, const uint32 ui32Index) : m_rConfigurationManager(rConfigurationManager) { rBox.getSettingValue(ui32Index, m_sSettingValue); }
		operator uint64 (void) { return m_rConfigurationManager.expandAsUInteger(m_sSettingValue); }
		operator int64 (void) { return m_rConfigurationManager.expandAsInteger(m_sSettingValue); }
		operator float64 (void) { return m_rConfigurationManager.expandAsFloat(m_sSettingValue); }
		operator boolean (void) { return m_rConfigurationManager.expandAsBoolean(m_sSettingValue); }
		operator const CString (void) { return m_sSettingValue; }
	protected:
		IConfigurationManager& m_rConfigurationManager;
		CString m_sSettingValue;
	};
};

CInputChannel::~CInputChannel()
{ 
	if(m_oMatrixBuffer[0]) {delete m_oMatrixBuffer[0];}
	if(m_oMatrixBuffer[1]) {delete m_oMatrixBuffer[1];}
}

boolean CInputChannel::initialize(OpenViBEToolkit::TBoxAlgorithm < OpenViBE::Plugins::IBoxAlgorithm>* pTBoxAlgorithm)
{
	m_bInitialized                  = false;
	m_bFirstChunk                   = true;
	m_bHeaderProcessed              = false;
	m_oMatrixBuffer[0]              = NULL;
	m_oMatrixBuffer[1]              = NULL;
	m_oIStimulationSet              = NULL;
	m_pTBoxAlgorithm                = pTBoxAlgorithm;
	m_ui32LoopStimulationChunkIndex = 0;
	m_ui32LoopSignalChunkIndex      = 0;
	m_ui32SignalChannel             = 0;
	m_ui32StimulationChannel        = 1;
	m_ui64TimeStampStartSignal      = 0;
	m_ui64TimeStampEndSignal        = 0;
	m_ui64PtrMatrixIndex = 0;

	m_ui64StartStimulation    = m_pTBoxAlgorithm->getTypeManager().getEnumerationEntryValueFromName(OV_TypeId_Stimulation, _AutoCast_(m_pTBoxAlgorithm->getStaticBoxContext(), m_pTBoxAlgorithm->getConfigurationManager(), 0));

	m_pStreamDecoderSignal=&m_pTBoxAlgorithm->getAlgorithmManager().getAlgorithm(m_pTBoxAlgorithm->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalStreamDecoder));
	m_pStreamDecoderSignal->initialize();
	ip_pMemoryBufferSignal.initialize(m_pStreamDecoderSignal->getInputParameter(OVP_GD_Algorithm_SignalStreamDecoder_InputParameterId_MemoryBufferToDecode));
	op_pMatrixSignal.initialize(m_pStreamDecoderSignal->getOutputParameter(OVP_GD_Algorithm_SignalStreamDecoder_OutputParameterId_Matrix));
	op_ui64SamplingRateSignal.initialize(m_pStreamDecoderSignal->getOutputParameter(OVP_GD_Algorithm_SignalStreamDecoder_OutputParameterId_SamplingRate));

	m_pStreamDecoderStimulation=&m_pTBoxAlgorithm->getAlgorithmManager().getAlgorithm(m_pTBoxAlgorithm->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationStreamDecoder));
	m_pStreamDecoderStimulation->initialize();
	ip_pMemoryBufferStimulation.initialize(m_pStreamDecoderStimulation->getInputParameter(OVP_GD_Algorithm_StimulationStreamDecoder_InputParameterId_MemoryBufferToDecode));
	op_pStimulationSetStimulation.initialize(m_pStreamDecoderStimulation->getOutputParameter(OVP_GD_Algorithm_StimulationStreamDecoder_OutputParameterId_StimulationSet));

	return true;
}

boolean CInputChannel::uninitialize()
{
	op_pStimulationSetStimulation.uninitialize();
	ip_pMemoryBufferStimulation.uninitialize();
	m_pStreamDecoderStimulation->uninitialize();
	m_pTBoxAlgorithm->getAlgorithmManager().releaseAlgorithm(*m_pStreamDecoderStimulation);

	
	op_ui64SamplingRateSignal.uninitialize();
	op_pMatrixSignal.uninitialize();
	ip_pMemoryBufferSignal.uninitialize();
	m_pStreamDecoderSignal->uninitialize();
	m_pTBoxAlgorithm->getAlgorithmManager().releaseAlgorithm(*m_pStreamDecoderSignal);


	return true;
}

boolean CInputChannel::getStimulationStart()
{

	IBoxIO& l_rDynamicBoxContext=m_pTBoxAlgorithm->getDynamicBoxContext();

	for(uint32 i=0; !m_bInitialized && i<l_rDynamicBoxContext.getInputChunkCount(m_ui32StimulationChannel); i++) //Stimulation de l'input 1
	{
		ip_pMemoryBufferStimulation=l_rDynamicBoxContext.getInputChunk(m_ui32StimulationChannel, i);
		m_pStreamDecoderStimulation->process();
		m_oIStimulationSet=op_pStimulationSetStimulation;

		m_ui64TimeStampStartStimulation=l_rDynamicBoxContext.getInputChunkStartTime(m_ui32StimulationChannel, i);

		for(uint32 j=0; j<m_oIStimulationSet->getStimulationCount();j++)
		{
			if(m_oIStimulationSet->getStimulationIdentifier(j)!=0/*==m_ui64StartStimulation*/)
			  {
				if(!m_bInitialized) 
				  {
					m_ui64TimeStimulationPosition=m_oIStimulationSet->getStimulationDate(j);
					m_bInitialized=true;
					m_ui64TimeStampStartStimulation=m_ui64TimeStimulationPosition;
					m_ui64TimeStampEndStimulation=l_rDynamicBoxContext.getInputChunkEndTime(m_ui32StimulationChannel, i);
					std::cout<<"Get Synchronisation Stimulation at channel "<<m_ui32StimulationChannel<<std::endl;
					break;
				  }
			  }
		}
		l_rDynamicBoxContext.markInputAsDeprecated(m_ui32StimulationChannel, i);
	}

	return m_bInitialized;
}

OpenViBE::boolean CInputChannel::getStimulation()
{
	
	IBoxIO& l_rDynamicBoxContext=m_pTBoxAlgorithm->getDynamicBoxContext();

	if(m_ui32LoopStimulationChunkIndex<l_rDynamicBoxContext.getInputChunkCount(m_ui32StimulationChannel))
	  {
		ip_pMemoryBufferStimulation=l_rDynamicBoxContext.getInputChunk(m_ui32StimulationChannel, 0);
		m_pStreamDecoderStimulation->process();
		m_oIStimulationSet=op_pStimulationSetStimulation;

		m_ui64TimeStampStartStimulation=l_rDynamicBoxContext.getInputChunkStartTime(m_ui32StimulationChannel, 0);
		m_ui64TimeStampEndStimulation=l_rDynamicBoxContext.getInputChunkEndTime(m_ui32StimulationChannel, 0);
     
		l_rDynamicBoxContext.markInputAsDeprecated(m_ui32StimulationChannel, 0);

		m_ui32LoopStimulationChunkIndex++;

		return true;
	}

	return false;
}


void CInputChannel::flushInputStimulation()
{
	IBoxIO& l_rDynamicBoxContext=m_pTBoxAlgorithm->getDynamicBoxContext();

	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(m_ui32StimulationChannel); i++) //Stimulation de l'input 1
	  {
		l_rDynamicBoxContext.markInputAsDeprecated(m_ui32StimulationChannel, i);
	  }
}

void CInputChannel::flushLateSignal()
{
	IBoxIO& l_rDynamicBoxContext=m_pTBoxAlgorithm->getDynamicBoxContext();

	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(m_ui32SignalChannel); i++) //Stimulation de l'input 1
	  {
		uint64 l_ui64chunkEndTime = l_rDynamicBoxContext.getInputChunkEndTime(m_ui32SignalChannel, i);
		if((!m_bInitialized && (l_ui64chunkEndTime<m_ui64TimeStampStartStimulation))  || (m_bInitialized && (l_ui64chunkEndTime<m_ui64TimeStimulationPosition)) )
		  {
			  l_rDynamicBoxContext.markInputAsDeprecated(m_ui32SignalChannel, i);
			  std::cout << "markInputAsDeprecated : "<<m_ui32SignalChannel << " i = " << i << std::endl;
		  }
	  }
}

boolean CInputChannel::getHeaderParams()
{
	IBoxIO& l_rDynamicBoxContext=m_pTBoxAlgorithm->getDynamicBoxContext();

	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(m_ui32SignalChannel); i++)
	{
		ip_pMemoryBufferSignal=l_rDynamicBoxContext.getInputChunk(m_ui32SignalChannel, i);
		m_pStreamDecoderSignal->process();
		if(m_pStreamDecoderSignal->isOutputTriggerActive(OVP_GD_Algorithm_SignalStreamDecoder_OutputTriggerId_ReceivedHeader))
		{
			if(m_oMatrixBuffer[0]) {delete m_oMatrixBuffer[0];}
			m_oMatrixBuffer[0] = new CMatrix();
			if(m_oMatrixBuffer[1]) {delete m_oMatrixBuffer[1];}
			m_oMatrixBuffer[1] = new CMatrix();

			OpenViBEToolkit::Tools::Matrix::copyDescription(*m_oMatrixBuffer[0], *op_pMatrixSignal);
			OpenViBEToolkit::Tools::Matrix::copyDescription(*m_oMatrixBuffer[1], *op_pMatrixSignal);
			m_bHeaderProcessed = true;
			l_rDynamicBoxContext.markInputAsDeprecated(m_ui32SignalChannel, i);
			return true;
		}
	}
	return false;
}

OpenViBE::CMatrix* CInputChannel::getMatrixPtr()
{
	return m_oMatrixBuffer[m_ui64PtrMatrixIndex & 1];
}

OpenViBE::boolean CInputChannel::hasSignalChunk()
{
	IBoxIO& l_rDynamicBoxContext=m_pTBoxAlgorithm->getDynamicBoxContext();
	return (m_ui32LoopSignalChunkIndex<l_rDynamicBoxContext.getInputChunkCount(m_ui32SignalChannel));
}

OpenViBE::boolean CInputChannel::fillData()
{
	IBoxIO& l_rDynamicBoxContext=m_pTBoxAlgorithm->getDynamicBoxContext();
	ip_pMemoryBufferSignal=l_rDynamicBoxContext.getInputChunk(m_ui32SignalChannel, m_ui32LoopSignalChunkIndex);
	m_pStreamDecoderSignal->process();
	if(m_pStreamDecoderSignal->isOutputTriggerActive(OVP_GD_Algorithm_SignalStreamDecoder_OutputTriggerId_ReceivedBuffer))
	  {
		  if(m_bFirstChunk)
		    {
				m_ui64TimeStampStartSignal = l_rDynamicBoxContext.getInputChunkStartTime(m_ui32SignalChannel, m_ui32LoopSignalChunkIndex);
				m_ui64TimeStampEndSignal   = l_rDynamicBoxContext.getInputChunkEndTime(m_ui32SignalChannel, m_ui32LoopSignalChunkIndex);
				if(!calculateSampleOffset()) {m_ui32LoopSignalChunkIndex++; return false;}

				copyData(false, m_ui64PtrMatrixIndex);
		    }
		  else
		    {
				copyData(true, m_ui64PtrMatrixIndex);
				copyData(false, m_ui64PtrMatrixIndex + 1);
		    }
	  }
	l_rDynamicBoxContext.markInputAsDeprecated(m_ui32SignalChannel, m_ui32LoopSignalChunkIndex);

	m_ui32LoopSignalChunkIndex++;

	if(m_bFirstChunk)
	{
		m_bFirstChunk = false;
		return false;
	}

	return true;
}

OpenViBE::boolean CInputChannel::calculateSampleOffset()
{
	if(m_ui64TimeStampStartSignal>m_ui64TimeStimulationPosition || m_ui64TimeStimulationPosition>m_ui64TimeStampEndSignal)
	  {return false;}

	m_ui64NbChannels=m_oMatrixBuffer[0]->getDimensionSize(0);
	m_ui64NbSamples=m_oMatrixBuffer[0]->getDimensionSize(1);
	m_ui64OffsetBuffer=uint64(double(m_ui64NbSamples*(m_ui64TimeStimulationPosition-m_ui64TimeStampStartSignal))/double(m_ui64TimeStampEndSignal-m_ui64TimeStampStartSignal));
	std::cout<<"OffsetSample = "<<m_ui64OffsetBuffer<<std::endl;

	m_ui64OffsetRestBuffer=m_ui64NbSamples-m_ui64OffsetBuffer;
	std::cout<<"OffsetRestSample = "<<m_ui64OffsetRestBuffer<<std::endl;
	return true;
}

void CInputChannel::copyData(const OpenViBE::boolean startSrc, OpenViBE::uint64 matrixIndex)
{
	OpenViBE::CMatrix*&    l_pMatrixBuffer = m_oMatrixBuffer[matrixIndex & 1];

	OpenViBE::float64*     l_pSrcData = op_pMatrixSignal->getBuffer() + (startSrc ? 0 : m_ui64OffsetBuffer);
	OpenViBE::float64*     l_pDstData = l_pMatrixBuffer->getBuffer()  + (startSrc ? m_ui64OffsetBuffer : 0);
	OpenViBE::uint64       l_ui64Size = (startSrc ? m_ui64OffsetBuffer : m_ui64OffsetRestBuffer)*sizeof(OpenViBE::float64);

	for(OpenViBE::uint64 i=0; i < m_ui64NbChannels; i++, l_pSrcData+=m_ui64NbSamples, l_pDstData+=m_ui64NbSamples)
	{
		memcpy(l_pDstData, l_pSrcData, size_t (l_ui64Size));
	}
}

#if 0
	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(0); i++)
	{
		ip_pMemoryBufferSignal1=l_rDynamicBoxContext.getInputChunk(0, i);
		op_pMemoryBufferSignal1=l_rDynamicBoxContext.getOutputChunk(0);
		m_pStreamDecoderSignal1->process();
		if(m_pStreamDecoderSignal1->isOutputTriggerActive(OVP_GD_Algorithm_SignalStreamDecoder_OutputTriggerId_ReceivedHeader))
		{
			OpenViBEToolkit::Tools::Matrix::copyDescription(*ip_pMatrixSignal1, *op_pMatrixSignal1);
			m_pStreamEncoderSignal1->process(OVP_GD_Algorithm_SignalStreamEncoder_InputTriggerId_EncodeHeader);
		}
		if(m_pStreamDecoderSignal1->isOutputTriggerActive(OVP_GD_Algorithm_SignalStreamDecoder_OutputTriggerId_ReceivedBuffer))
		{
			OpenViBEToolkit::Tools::Matrix::copyContent(*ip_pMatrixSignal1, *op_pMatrixSignal1);
			m_pStreamEncoderSignal1->process(OVP_GD_Algorithm_SignalStreamEncoder_InputTriggerId_EncodeBuffer);
		}
		if(m_pStreamDecoderSignal1->isOutputTriggerActive(OVP_GD_Algorithm_SignalStreamDecoder_OutputTriggerId_ReceivedEnd))
		{
			m_pStreamEncoderSignal1->process(OVP_GD_Algorithm_SignalStreamEncoder_InputTriggerId_EncodeEnd);
		}

		l_rDynamicBoxContext.markOutputAsReadyToSend(0, l_rDynamicBoxContext.getInputChunkStartTime(0, i), l_rDynamicBoxContext.getInputChunkEndTime(0, i));
		l_rDynamicBoxContext.markInputAsDeprecated(0, i);
	}
#endif