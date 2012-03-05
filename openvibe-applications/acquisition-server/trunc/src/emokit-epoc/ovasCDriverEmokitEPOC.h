#ifndef __OpenViBE_AcquisitionServer_CDriverEmokitEPOC_H__
#define __OpenViBE_AcquisitionServer_CDriverEmokitEPOC_H__

#if defined TARGET_HAS_ThirdPartyEmokitAPI

#include "../ovasIDriver.h"
#include "../ovasCHeader.h"

#include <vector>

namespace OpenViBEAcquisitionServer
{
	/**
	 * \class CDriverEmokitEPOC
	 * \author Kyle Machulis
	 * \date 4 march 2010
	 * \brief The CDriverEmokitEPOC allows the acquisition server to acquire data from a Emotiv EPOC Amplifier. All of them. Really.
	 *
	 */
	class CDriverEmokitEPOC : public OpenViBEAcquisitionServer::IDriver
	{
	public:

		CDriverEmokitEPOC(OpenViBEAcquisitionServer::IDriverContext& rDriverContext);
		virtual ~CDriverEmokitEPOC(void);
		virtual const char* getName(void);

		virtual OpenViBE::boolean initialize(
			const OpenViBE::uint32 ui32SampleCountPerSentBlock,
			OpenViBEAcquisitionServer::IDriverCallback& rCallback);
		virtual OpenViBE::boolean uninitialize(void);

		virtual OpenViBE::boolean start(void);
		virtual OpenViBE::boolean stop(void);
		virtual OpenViBE::boolean loop(void);

		virtual OpenViBE::boolean isConfigurable(void);
		virtual OpenViBE::boolean configure(void);
		virtual const OpenViBEAcquisitionServer::IHeader* getHeader(void) { return &m_oHeader; }

	protected:

		OpenViBEAcquisitionServer::IDriverCallback* m_pCallback;

		OpenViBEAcquisitionServer::CHeader m_oHeader;

		OpenViBE::uint32 m_ui32SampleCountPerSentBlock;
		OpenViBE::uint32 m_ui32TotalSampleCount;
		OpenViBE::float32* m_pSample;
		//OpenViBE::float64* m_pBuffer;

	private:

		OpenViBE::uint32 m_ui32_LastErrorCode;

		// TODO: Replace with Emokit device
		//EmoEngineEventHandle m_tEEEventHandle;
		OpenViBE::uint32 m_ui32UserID;
		OpenViBE::boolean m_bReadyToCollect;

		// TODO: What's a data handle?
		//DataHandle m_tDataHandle;
		OpenViBE::boolean m_bFirstStart;

		OpenViBE::boolean m_bUseGyroscope;
	};
};

#endif // TARGET_HAS_ThirdPartyEmokitAPI

#endif // __OpenViBE_AcquisitionServer_CDriverEmokitEPOC_H__
