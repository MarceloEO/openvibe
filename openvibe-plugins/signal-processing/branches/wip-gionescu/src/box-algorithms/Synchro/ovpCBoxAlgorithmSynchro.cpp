#include "ovpCBoxAlgorithmSynchro.h"

#include <iostream>

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBE::Plugins;

using namespace OpenViBEPlugins;
using namespace OpenViBEPlugins::SignalProcessing;


boolean CBoxAlgorithmSynchro::initialize(void)
{
	m_oCInputChannel[0].initialize(this, 0);
	m_oCInputChannel[1].initialize(this, 1);
	
	m_oCOutputChannel[0].initialize(this, 0);
	m_oCOutputChannel[1].initialize(this, 1);

	//
	m_bStimulationReceivedStart=false;

	return true;
}

boolean CBoxAlgorithmSynchro::uninitialize(void)
{
	m_oCInputChannel[0].uninitialize();
	m_oCInputChannel[1].uninitialize();

	m_oCOutputChannel[0].uninitialize();
	m_oCOutputChannel[1].uninitialize();

	return true;
}

boolean CBoxAlgorithmSynchro::processInput(uint32 ui32InputIndex)
{
	this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
	return true;
}

boolean CBoxAlgorithmSynchro::process(void)
{
	IBoxIO& l_rDynamicBoxContext=this->getDynamicBoxContext();

	for(uint32 iInput = 0; iInput<2; iInput++)
	  {
		if(!m_oCInputChannel[iInput].isInitialized())
		  {
			m_oCInputChannel[iInput].getStimulationStart();
		  }
		else
		  {
			  m_oCInputChannel[iInput].flushInputStimulation();
		  }
	}

	for(uint32 iInput = 0; iInput<2; iInput++)
	  {
		  if(!m_oCInputChannel[iInput].hasProcessedHeader()) {m_oCInputChannel[iInput].getHeaderParams();}
	  }

	for(uint32 iInput = 0; iInput<2; iInput++)
	  {
		  if(m_oCInputChannel[iInput].hasProcessedHeader() && !m_oCOutputChannel[iInput].hasProcessedHeader()) 
		    {
			  m_oCOutputChannel[iInput].setMatrixPtr(m_oCInputChannel[iInput].getMatrixPtr());
			  m_oCOutputChannel[iInput].setSamplingRate(m_oCInputChannel[iInput].getSamplingRate());
			  m_oCOutputChannel[iInput].setChunkTimeStamps(m_oCInputChannel[iInput].getTimeStampStart(),m_oCInputChannel[iInput].getTimeStampEnd());
			  m_oCOutputChannel[iInput].sendHeader();
			}
	  }

	for(uint32 iInput = 0; iInput<2; iInput++)
	  {
		  m_oCInputChannel[iInput].flushLateSignal();
	  }


#if 0
	for(uint32 i=0; i<l_rDynamicBoxContext.getInputChunkCount(1); i++) //Stimulation de l'input 1
	{
		ip_pMemoryBufferStimulation1=l_rDynamicBoxContext.getInputChunk(1, i);
		m_pStreamDecoderStimulation1->process();
		IStimulationSet* l_StimSet=op_pStimulationSetStimulation1;

		if(l_StimSet->getStimulationCount()!=0) {std::cout<<"SOME STIMULATION"<<std::endl;}
		for(uint32 j=0; !m_bStimulationReceivedStart && j<l_StimSet->getStimulationCount();j++)
		{
			if(l_StimSet->getStimulationIdentifier(j)!=0)
			{
				m_bStimulationReceivedStart=true;
				std::cout<<"Stimulation received"<<std::endl;
			}
		}

		//ip_pStimulationSetStimulation=op_pStimulationSetStimulation1;
		op_pMemoryBufferStimulation=l_rDynamicBoxContext.getOutputChunk(2);
		if(m_pStreamDecoderStimulation1->isOutputTriggerActive(OVP_GD_Algorithm_StimulationStreamDecoder_OutputTriggerId_ReceivedHeader))
		{
			std::cout<<"Header"<<std::endl;
			m_pStreamEncoderStimulation->process(OVP_GD_Algorithm_StimulationStreamEncoder_InputTriggerId_EncodeHeader);
		}
		if(m_pStreamDecoderStimulation1->isOutputTriggerActive(OVP_GD_Algorithm_StimulationStreamDecoder_OutputTriggerId_ReceivedBuffer))
		{
			std::cout<<"Buffer"<<std::endl;
			m_pStreamEncoderStimulation->process(OVP_GD_Algorithm_StimulationStreamEncoder_InputTriggerId_EncodeBuffer);
		}
		if(m_pStreamDecoderStimulation1->isOutputTriggerActive(OVP_GD_Algorithm_StimulationStreamDecoder_OutputTriggerId_ReceivedEnd))
		{
			std::cout<<"Footer"<<std::endl;
			m_pStreamEncoderStimulation->process(OVP_GD_Algorithm_StimulationStreamEncoder_InputTriggerId_EncodeEnd);
		}
		l_rDynamicBoxContext.markOutputAsReadyToSend(2, l_rDynamicBoxContext.getInputChunkStartTime(1, i), l_rDynamicBoxContext.getInputChunkEndTime(1, i));
		l_rDynamicBoxContext.markInputAsDeprecated(1, i);
	}


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

	return true;
}
